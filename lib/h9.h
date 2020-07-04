//
//  h9.h
//  libh9 - Sysex and state manipulation toolkit for remotely controlling
//          the Eventide H9 multi-effect pedal.
//
//  Created by Studio DC on 2020-06-24.
//

#ifndef h9_h
#define h9_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define H9_NUM_MODULES    5
#define H9_MAX_ALGORITHMS 12
#define H9_NUM_KNOBS      10
#define H9_MAX_NAME_LEN   17
#define H9_SYSEX_EVENTIDE 0x1C
#define H9_SYSEX_H9       0x70
#define H9_NOMODULE       -1
#define H9_NOALGORITHM    -1
#define CC_DISABLED       255
#define MAX_CC            99

typedef enum h9_status {
    kH9_UNKNOWN = 0U,
    kH9_OK,
    kH9_SYSEX_PREAMBLE_INCORRECT,
    kH9_SYSEX_INVALID,
    kH9_SYSEX_CHECKSUM_INVALID,
} h9_status;

typedef enum h9_message_code {
    // kH9_OK is returned by the pedal often to confirm the last request.
    kH9_ERROR             = 0x0d,  // Response from pedal with problem. Body of response is ASCII readable error description.
    kH9_USER_VALUE_PUT    = 0x2d,  // COMMAND to set indicated key to value. Data format: [key]<space>[value] in "ASCII hex". Response is VALUE_DUMP
    kH9_SYSEX_VALUE_DUMP  = 0x2e,  // Response containing a single value as "ASCII hex"
    kH9_OBJECTINFO_WANT   = 0x31,  // Request value of specified key. Data format: [key] = ["ASCII hex", e.g. "201" = 0x32 0x30 0x31 0x00]
    kH9_VALUE_WANT        = 0x3b,  // Same as OBJECTINFO_WANT. Both reply with a VALUE_DUMP.
    kH9_USER_OBJECT_SHORT = 0x3c,  // COMMAND: XXXX YY = [key] [value], same as VALUE_PUT.
    kH9_DUMP_ALL          = 0x48,  // Requests all programs. Response is a PROGRAM_DUMP.
    kH9_PROGRAM_DUMP      = 0x49,  // Response containing all programs in memory on the unit, sequentially.
    kH9_TJ_SYSVARS_WANT   = 0x4c,  // Request full sysvars. Response is a TJ_SYSVARS_DUMP.
    kH9_TJ_SYSVARS_DUMP   = 0x4d,  // Response to SYSVARS_WANT, contains full sysvar dump in unspecified format.
    kH9_DUMP_ONE          = 0x4e,  // Requests the currently loaded PROGRAM. Response is PROGRAM.
    kH9_PROGRAM           = 0x4f,  // COMMAND to set temporary PROGRAM, RESPONSE contains indicated PROGRAM.
} h9_message_code;

typedef enum control_id {
    KNOB0 = 0U,  // KNOB 0 MUST REMAIN 0
    KNOB1,
    KNOB2,
    KNOB3,
    KNOB4,
    KNOB5,
    KNOB6,
    KNOB7,
    KNOB8,
    KNOB9,  // KNOBS ARE ALWAYS 0-9
    EXPR,
    PSW,
    NUM_CONTROLS,  // KEEP THIS LAST
} control_id;

typedef enum h9_callback_action {
    kH9_SUPPRESS_CALLBACK = 0U,
    kH9_TRIGGER_CALLBACK,
} h9_callback_action;

typedef float control_value;  // 0.00 to 1.00 always.

typedef struct h9_algorithm {
    uint8_t id;
    uint8_t module;
    char*   name;
    char*   label_knob1;
    char*   label_knob2;
    char*   label_knob3;
    char*   label_knob4;
    char*   label_knob5;
    char*   label_knob6;
    char*   label_knob7;
    char*   label_knob8;
    char*   label_knob9;
    char*   label_knob10;
    char*   label_psw;
} h9_algorithm;

typedef struct h9_module {
    char*        name;
    uint8_t      sysex_num;
    uint8_t      psw_mode;
    h9_algorithm algorithms[H9_MAX_ALGORITHMS];
    size_t       num_algorithms;
} h9_module;

typedef struct h9_knob {
    control_value current_value;  // Physical position of the knob.
    control_value display_value;  // Display value, after adjustment by, e.g. exp or psw operation.
    float         mknob_value;    // Still no clue what this is exactly, seems some translated display value of the knob.
    control_value exp_min;
    control_value exp_max;
    control_value psw;
    bool          exp_mapped;
    bool          psw_mapped;
} h9_knob;

typedef struct h9_preset {
    char          name[H9_MAX_NAME_LEN];
    h9_module*    module;
    h9_algorithm* algorithm;
    h9_knob       knobs[H9_NUM_KNOBS];
    control_value expression;
    bool          psw;
    float         tempo;
    float         output_gain;
    uint8_t       xyz_map[3];
    bool          tempo_enabled;
    bool          modfactor_fast_slow;
} h9_preset;

struct h9;
typedef struct h9 h9;
typedef void (*h9_display_callback)(void* ctx, control_id control, control_value current_value, control_value display_value);
typedef void (*h9_cc_callback)(void* ctx, uint8_t midi_channel, uint8_t cc, uint8_t msb, uint8_t lsb);
typedef void (*h9_sysex_callback)(void* ctx, uint8_t* sysex, size_t len);

/*
 sysex_id can be 1-16 (0 is prohibited as it is the broadcast value). 1 is the pedal default.
 midi_channel can be 0-15 (equals channels 1-16)

 The CC maps map cc numbers to the defined control_id values, so cc_rx_map[EXPR] is the cc number for the expression pedal.
 Allowed values are 0-MAX_CC, or CC_DISABLED.
 NOTE: The definition of rx/tx is from the perspective of the pedal (matching the memory on the pedal)!!
 > cc_rx_map contains the cc number the PEDAL receives data on (e.g. the value the plugin should SEND data using)
 > cc_tx_map contains the cc number the PEDAL transmits data on (e.g. the value the plugin should LISTEN to)
 */
typedef struct h9_midi_config {
    uint8_t sysex_id;
    uint8_t midi_channel;
    uint8_t cc_rx_map[NUM_CONTROLS];
    uint8_t cc_tx_map[NUM_CONTROLS];
} h9_midi_config;

// The core H9 model
typedef struct h9 {
    char           name[H9_MAX_NAME_LEN];
    h9_midi_config midi_config;
    h9_preset*     preset;
    bool           dirty;  // true if changes have been made (e.g. knobs twiddled, exp map changed) after last load or save

    // Observer registration
    h9_display_callback display_callback;
    h9_cc_callback      cc_callback;
    h9_sysex_callback   sysex_callback;
    void*               callback_context;
} h9;

#ifdef __cplusplus
extern "C" {
#endif

// H9 API

const char* const h9_algorithmName(uint8_t module_sysex_id, uint8_t algorithm_sysex_id);
void              h9_delete(h9* h9);
control_value     h9_controlValue(h9* h9, control_id control);
void              h9_copyMidiConfig(h9* h9, h9_midi_config* dest_copy);
int8_t            h9_currentAlgorithm(h9* h9);  // returns NOALGORITHM if unloaded
const char*       h9_currentAlgorithmName(h9* h9);
int8_t            h9_currentModule(h9* h9);  // returns NOMODULE if unloaded
const char*       h9_currentModuleName(h9* h9);
bool              h9_dirty(h9* h9);
control_value     h9_displayValue(h9* h9, control_id control);
void              h9_knobMap(h9* h9, control_id knob_num, control_value* exp_min, control_value* exp_max, control_value* psw);
const char* const h9_moduleName(uint8_t module_sysex_id);
h9*               h9_new(void);  // Allocates and returns a pointer to a new H9 instance
size_t            h9_numAlgorithms(h9* h9, uint8_t module_sysex_id);
size_t            h9_numModules(h9* h9);
bool              h9_setAlgorithm(h9* h9, uint8_t module_sysex_id, uint8_t algorithm_sysex_id);
void              h9_setControl(h9* h9, control_id knob_num, control_value value, h9_callback_action cc_cb_action);
void              h9_setKnobMap(h9* h9, control_id knob_num, control_value exp_min, control_value exp_max, control_value psw);
bool              h9_setMidiConfig(h9* h9, const h9_midi_config* midi_config);

#ifdef __cplusplus
}
#endif

// Bring in the rest of the modules
#include "h9_sysex.h"

#endif /* h9_h */
