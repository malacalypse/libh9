//
//  h9.h
//  max-external
//
//  Created by Studio DC on 2020-06-24.
//

#ifndef h9_h
#define h9_h

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define NUM_KNOBS 10
#define MAX_NAME_LEN 17 // Fixme(DC): Determine correct max for both pedal name and preset name. Don't forget the terminating null.
#define SYSEX_EVENTIDE 0x1C
#define SYSEX_H9 0x70

typedef struct h9_algorithm {
    uint8_t id;
    uint8_t module;
    char *name;
    char *label_knob1;
    char *label_knob2;
    char *label_knob3;
    char *label_knob4;
    char *label_knob5;
    char *label_knob6;
    char *label_knob7;
    char *label_knob8;
    char *label_knob9;
    char *label_knob10;
    char *label_psw;
} h9_algorithm;

typedef struct h9_module {
    char *name;
    uint8_t sysex_num;
    uint8_t psw_mode;
    h9_algorithm *algorithms;
    size_t num_algorithms;
} h9_module;

typedef enum h9_status {
    kH9_UNKNOWN = 0U,
    kH9_OK,
    kH9_SYSEX_PREAMBLE_INCORRECT,
    kH9_SYSEX_CHECKSUM_INVALID,
} h9_status;

typedef enum h9_message_code {
    kH9_GET_CONFIG = 0x2e, // 46
    kH9_DUMP_ALL = 0x48,   // 72
    kH9_DUMP_ONE = 0x4e,   // 78
    kH9_PRESET = 0x4f,     // 79, used both when receiving and sending
} h9_message_code;

typedef uint8_t knob_id;
typedef float knob_value; // 0.00 to 1.00 always.

typedef struct h9_knob {
    knob_value default_value; // Value to set the knob to when loading the preset
    knob_value current_value; // Value the knob has been manually or via CC set to after loading
    knob_value display_value; // After adjustment by, e.g. exp or psw operation
    float mknob_value; // Still no clue what this is exactly, seems some translated display value of the knob.
    knob_value exp_min;
    knob_value exp_max;
    knob_value psw;
    bool exp_mapped;
    bool psw_mapped;
} h9_knob;

typedef struct h9_preset {
    char name[MAX_NAME_LEN];
    h9_module* module;
    h9_algorithm* algorithm;
    h9_knob knobs[NUM_KNOBS];
} h9_preset;

// We'll work on callbacks later
// typedef (void) knob_update(uint8_t knob_id, float value, void *ctx);
// typedef (void) data_callback(uint32_t len, char *data);

// The core H9 model
typedef struct h9 {
    // Device info
    char name[MAX_NAME_LEN];

    // MIDI and communications settings
    uint8_t sysex_id;
    uint8_t midi_channel;
    // ... other things like cc maps go here, once we support CC parsing and configuration detection

    // Current loaded preset
    h9_preset *preset;
    bool preset_dirty; // true if changes have been made (e.g. knobs twiddled, exp map changed) after last load or save
    bool synched; // true if we've spit out the preset to the device
    bool sync_dirty; // true if we've made changes that have not been synched (sysex-only changes, not knob values that can be sent as CC)

    // Current physical control states
    float expression; // 0.00 to 1.00
    bool expr_changed; // true if the expression pedal has been moved after loading the preset
    bool psw; // switch, on (true) or off (false)
} h9;

// H9 API

// Object lifecycle
// Create
h9* h9_new(void); // Allocates and returns a pointer to a new H9 instance
// Free / Delete
void h9_free(h9* h9);

// Common H9 operations
h9_status h9_load(h9* h9, uint8_t* sysex, size_t len); // can include or skip the leading and trailing 0xF0/0xF7, doesn't matter
size_t h9_dump(h9* h9, uint8_t* sysex, size_t max_len); // will dump WITH the 0xF0/0xF7 and correct preamble.

// Knob, Expr, and PSW operations
void h9_setKnob(h9* h9, knob_id knob_num, knob_value value);
void h9_setKnobMap(h9* h9, knob_id knob_num, knob_value exp_min, knob_value exp_max, knob_value psw);
knob_value h9_getKnob(h9* h9, knob_id knob_num);
knob_value h9_getKnobDisplay(h9* h9, knob_id knob_num);
void h9_getKnobMap(h9* h9, knob_id knob_num, knob_value* exp_min, knob_value* exp_max, knob_value* psw);
void h9_setExpr(h9* h9, knob_value value);
knob_value h9_getExpr(h9* h9);
void h9_setPsw(h9* h9, bool psw_on);
bool h9_getPsw(h9* h9);

// Preset Operations
void h9_switchAlgorithm(h9* h9, uint8_t module_sysex_id, uint8_t algorithm_sysex_id);

// (S)Setters and getters for preset name, module (by name or number), and algorithm (by name or number).
size_t h9_numSupportedModules(h9* h9);
size_t h9_numSupportedAlgorithms(h9* h9, uint8_t module_sysex_id);
uint8_t h9_currentModule(h9* h9);
uint8_t h9_currentAlgorithm(h9* h9);
char* h9_moduleName(h9* h9, uint8_t module_sysex_id);
char* h9_currentModuleName(h9* h9);
char* h9_algorithmName(h9* h9, uint8_t algorithm_sysex_id);
char* h9_currentAlgorithmName(h9* h9);

// Syncing
size_t h9_sysexGenRequestCurrentPreset(h9* h9, uint8_t* sysex, size_t max_len);
size_t h9_sysexGenRequestSystemConfig(h9* h9, uint8_t* sysex, size_t max_len);

#endif /* h9_h */
