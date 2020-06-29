//
//  h9.c
//  The H9 core model
//
//  Created by Studio DC on 2020-06-24.
//

#include "h9.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "h9_module.h"
#include "h9_modules.h"
#include "utils.h"

#define MIDI_MAX                     16383  // 2^14 - 1 for 14-bit MIDI
#define DEFAULT_MODULE               4      // zero-indexed
#define DEFAULT_ALGORITHM            0
#define DEFAULT_KNOB_CC              22
#define DEFAULT_EXPR_CC              15
#define DEFAULT_PSW_CC               CC_DISABLED
#define DEFAULT_KNOB_VALUE           0.5
#define EMPTY_PRESET_NAME            "Empty"
#define MIDI_ACCEPTABLE_LSB_DELAY_MS 3.5  // roughly the amount of time to transmit the CC over a slow DIN connection

#define DEBUG_LEVEL DEBUG_ERROR
#include "debug.h"

/* ==== Private Variables ========================================================= */

static uint8_t last_msb_cc             = CC_DISABLED;
static uint8_t last_msb                = 0;
static double  last_msb_timestamp_msec = 0;

/* ==== Private Function Declarations ============================================= */
static void h9_setExpr(h9* h9, control_value value);
static void h9_setPsw(h9* h9, bool psw_on);
static void h9_setKnob(h9* h9, control_id control, control_value value);
static void display_callback(h9* h9, control_id control, double current_value, double display_value);
static void cc_callback(h9* h9, control_id control, double value);

/* ==== MODULE Private Function Definitions (implements h9_module.h) ============== */
void h9_reset_display_values(h9* h9) {
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9_knob* knob = &h9->preset->knobs[i];
        h9_update_display_value(h9, (control_id)i, knob->current_value);
    }
    h9_update_display_value(h9, EXPR, h9->preset->expression);
    h9_update_display_value(h9, PSW, h9->preset->psw ? 1.0f : 0.0f);
}

// This exists to handle future callbacks or other dynamic behaviour
void h9_update_display_value(h9* h9, control_id control, control_value value) {
    if (control < H9_NUM_KNOBS) {
        h9_knob* knob       = &h9->preset->knobs[control];
        knob->display_value = value;
        display_callback(h9, control, knob->current_value, knob->display_value);
    } else {
        display_callback(h9, control, value, value);
    }
}

/* ==== Private Functions ========================================================= */

static void h9_setExpr(h9* h9, control_value value) {
    control_value expval = clip(value, 0.0f, 1.0f);

    if (h9->preset->expression == expval) {
        return;  // break update cyclic loops
    }

    h9->preset->expression = expval;
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9_knob* knob = &h9->preset->knobs[i];
        if (knob->exp_mapped) {
            control_value interpolated_value = (knob->exp_max - knob->exp_min) * expval + knob->exp_min;
            h9_update_display_value(h9, (control_id)i, interpolated_value);
        }
    }
    h9_update_display_value(h9, EXPR, expval);
}

static void h9_setPsw(h9* h9, bool psw_on) {
    if (h9->preset->psw == psw_on) {
        return;  // break cyclic loops
    }

    h9->preset->psw = psw_on;
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9_knob* knob = &h9->preset->knobs[i];
        if (knob->psw_mapped) {
            // There might be an issue here if the expression has moved the knob and the PSW is turned on then off. Check vs. the pedal's behaviour.
            h9_update_display_value(h9, (control_id)i, psw_on ? knob->psw : knob->current_value);
        }
    }
    h9_update_display_value(h9, PSW, psw_on ? 1.0 : 0.0f);
}

static void h9_setKnob(h9* h9, control_id control, control_value value) {
    h9_knob* knob       = &h9->preset->knobs[control];
    knob->current_value = value;
    if (knob->current_value != knob->display_value) {
        h9_update_display_value(h9, control, knob->current_value);
    }
}

static void display_callback(h9* h9, control_id control, double current_value, double display_value) {
    if (h9->display_callback != NULL) {
        h9->display_callback(h9->callback_context, control, current_value, display_value);
    }
}

static void cc_callback(h9* h9, control_id control, double value) {
    if ((h9->cc_callback == NULL) || (h9->midi_config.cc_rx_map[control] == CC_DISABLED)) {
        return;
    }
    uint8_t  midi_channel = h9->midi_config.midi_rx_channel;
    uint8_t  control_cc   = h9->midi_config.cc_rx_map[control];
    uint16_t cc_value     = clip(value, 0.0f, 1.0f) * MIDI_MAX;
    h9->cc_callback(h9->callback_context, midi_channel, control_cc, (uint8_t)(cc_value >> 7), (uint8_t)(cc_value & 0x7F));
}

/* ==== PUBLIC (exported) Functions =============================================== */

h9* h9_new(void) {
    h9* h9 = malloc(sizeof(*h9));
    if (h9 == NULL) {
        return h9;
    }

    // Init the preset object
    h9->preset = h9_preset_new();
    if (h9->preset == NULL) {
        h9_delete(h9);
        h9 = NULL;
        return h9;
    }

    // Set sane default values so the object functions correctly
    h9->midi_config.sysex_id        = 1U;
    h9->midi_config.midi_rx_channel = 0U;
    h9->midi_config.midi_tx_channel = 0U;
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9->midi_config.cc_rx_map[i] = DEFAULT_KNOB_CC + i;
        h9->midi_config.cc_tx_map[i] = DEFAULT_KNOB_CC + i;
    }
    h9->midi_config.cc_rx_map[EXPR] = CC_DISABLED;  // Per the user guide, the default for RX is disabled.
    h9->midi_config.cc_tx_map[EXPR] = DEFAULT_EXPR_CC;
    h9->midi_config.cc_rx_map[PSW]  = DEFAULT_PSW_CC;
    h9->midi_config.cc_tx_map[PSW]  = DEFAULT_PSW_CC;

    h9->cc_callback      = NULL;
    h9->display_callback = NULL;
    h9->sysex_callback   = NULL;
    h9->callback_context = (void*)h9;

    memset(h9->name, 0x0, sizeof(h9->name));
    memset(h9->bluetooth_pin, 0x0, sizeof(h9->bluetooth_pin));
    strcpy(h9->name, "H9");
    strcpy(h9->bluetooth_pin, "0000");

    return h9;
}

void h9_delete(h9* h9) {
    if (h9 == NULL) {
        return;
    }
    if (h9->preset != NULL) {
        free(h9->preset);
    }
    free(h9);
}

h9_preset* h9_preset_new(void) {
    h9_preset* h9_preset = malloc(sizeof(*h9_preset));
    if (h9_preset == NULL) {
        return h9_preset;
    }

    // Set up a safe (but not very useful) default preset
    h9_preset->module    = &modules[DEFAULT_MODULE];
    h9_preset->algorithm = &h9_preset->module->algorithms[DEFAULT_ALGORITHM];
    strncpy(h9_preset->name, EMPTY_PRESET_NAME, H9_MAX_NAME_LEN);
    h9_preset->output_gain = 0.0f;
    h9_preset->tempo       = 120.0f;

    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9_knob* knob       = &h9_preset->knobs[i];
        knob->current_value = DEFAULT_KNOB_VALUE;
        knob->display_value = DEFAULT_KNOB_VALUE;
        knob->exp_mapped    = false;
        knob->mknob_value   = 0.0;
        knob->psw_mapped    = false;
    }
    h9_preset->expression = 0.0;
    h9_preset->psw        = false;

    return h9_preset;
}

// Common H9 operations

// Knob, Expr, and PSW operations
void h9_setControl(h9* h9, control_id control, control_value value, h9_callback_action cc_cb_action) {
    if (control >= NUM_CONTROLS) {
        return;  // Control is invalid
    }

    switch (control) {
        case EXPR:
            h9_setExpr(h9, value);
            break;
        case PSW:
            h9_setPsw(h9, (value > 0.0f));
            break;
        default:  // A knob
            h9_setKnob(h9, control, value);
    }
    h9->preset->dirty = true;
    if (cc_cb_action == kH9_TRIGGER_CALLBACK) {
        cc_callback(h9, control, value);
    }
}

void h9_setKnobMap(h9* h9, control_id knob_num, control_value exp_min, control_value exp_max, control_value psw) {
    if (knob_num > KNOB9) {
        return;
    }
    h9_knob* knob    = &h9->preset->knobs[knob_num];
    knob->exp_min    = exp_min;
    knob->exp_max    = exp_max;
    knob->psw        = psw;
    knob->exp_mapped = (exp_min != exp_max);
    knob->psw_mapped = (psw != 0.0f && psw != knob->current_value);
}

control_value h9_controlValue(h9* h9, control_id control) {
    if (control > NUM_CONTROLS) {
        return -1.0f;
    }

    h9_knob* knob;

    switch (control) {
        case EXPR:
            return h9->preset->expression;
        case PSW:
            return h9->preset->psw ? 1.0f : 0.0f;
        default:
            knob = &h9->preset->knobs[control];
            return knob->current_value;
    }
}

control_value h9_displayValue(h9* h9, control_id control) {
    if (control > KNOB9) {
        return h9_controlValue(h9, control);
    }
    h9_knob* knob = &h9->preset->knobs[control];
    return knob->display_value;
}

void h9_knobMap(h9* h9, control_id knob_num, control_value* exp_min, control_value* exp_max, control_value* psw) {
    if (knob_num > KNOB9) {
        return;
    }
    h9_knob* knob = &h9->preset->knobs[knob_num];
    *exp_min      = knob->exp_min;
    *exp_max      = knob->exp_max;
    *psw          = knob->psw;
}

// Preset Operations
bool h9_setAlgorithm(h9* h9, uint8_t module_zero_indexed_id, uint8_t algorithm_zero_indexed_id) {
    if (module_zero_indexed_id >= H9_NUM_MODULES) {
        return false;
    }
    h9_module* module = &modules[module_zero_indexed_id];
    if (algorithm_zero_indexed_id >= module->num_algorithms) {
        return false;
    }
    h9->preset->module    = module;
    h9->preset->algorithm = &module->algorithms[algorithm_zero_indexed_id];
    h9->preset->dirty     = true;
    h9_reset_display_values(h9);
    return true;
}

// (S)Setters and getters for preset name, module (by name or number), and algorithm (by name or number).
size_t h9_numModules(h9* h9) {
    return H9_NUM_MODULES;
}

size_t h9_numAlgorithms(h9* h9, uint8_t module_sysex_id) {
    assert(module_sysex_id > 0 && module_sysex_id <= H9_NUM_MODULES);
    return modules[module_sysex_id - 1].num_algorithms;
}

h9_module* h9_currentModule(h9* h9) {
    return h9->preset->module;
}

uint8_t h9_currentModuleIndex(h9* h9) {
    return h9->preset->module->sysex_num - 1;  // Zero index externally
}

h9_algorithm* h9_currentAlgorithm(h9* h9) {
    return h9->preset->algorithm;
}

uint8_t h9_currentAlgorithmIndex(h9* h9) {
    return h9->preset->algorithm->id;
}

const char* const h9_moduleName(uint8_t module_sysex_id) {
    assert(module_sysex_id > 0 && module_sysex_id <= H9_NUM_MODULES);
    return modules[module_sysex_id].name;
}

const char* h9_currentModuleName(h9* h9) {
    return h9->preset->module->name;
}

const char* h9_presetName(h9* h9, size_t* len) {
    if (len != NULL) {
        *len = strnlen(h9->preset->name, H9_MAX_NAME_LEN);
    }
    return h9->preset->name;
}

bool h9_setPresetName(h9* h9, const char* name, size_t len) {
    char              new_name[H9_MAX_NAME_LEN];
    const char* const valid_characters        = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ|_-+*abcdefghijklmnopqrstuvwxyz";
    const size_t      valid_characters_length = strlen(valid_characters);
    size_t            len_to_scan             = (len <= H9_MAX_NAME_LEN ? len : H9_MAX_NAME_LEN);
    size_t            valid_len               = 0;
    size_t            accumulated_spaces      = 0;
    // Simultaneously strip trailing spaces and validate string
    for (size_t i = 0; i < len_to_scan; i++) {
        for (size_t j = 0; j <= valid_characters_length; j++) {
            if (name[i] == valid_characters[j]) {
                // We found a valid character
                new_name[i] = name[i];
                if (j == 0) {
                    accumulated_spaces++;
                } else {
                    // Add any accumulated spaces plus the current character
                    valid_len += accumulated_spaces;
                    accumulated_spaces = 0;
                    valid_len++;
                }
                break;
            } else {
                // Valid character not found at this position
                if (j == valid_characters_length) {
                    // We're at the end of the array, character is not valid
                    new_name[i] = ' ';  // replace invalid characters with spaces
                    accumulated_spaces++;
                }
            }
        }
    }
    if (valid_len > 0) {
        if (valid_len < H9_MAX_NAME_LEN) {
            valid_len++;  // Add room for the null terminator
        }                 // otherwise truncate to make room for it.
        strlcpy(h9->preset->name, new_name, valid_len);
        return true;
    } else {
        return false;  // Blank names are not permitted by the pedal.
    }
}

const char* const h9_algorithmName(uint8_t module_sysex_id, uint8_t algorithm_sysex_id) {
    assert(module_sysex_id > 0 && module_sysex_id <= H9_NUM_MODULES);
    h9_module* module = &modules[module_sysex_id];
    assert(algorithm_sysex_id < module->num_algorithms);
    return module->algorithms[algorithm_sysex_id].name;
}

const char* h9_currentAlgorithmName(h9* h9) {
    return h9->preset->algorithm->name;
}

// MIDI configuration

// Copies the config, does not retain a reference
bool h9_setMidiConfig(h9* h9, const h9_midi_config* midi_config) {
    if (midi_config == NULL) {
        return false;
    }
    if (midi_config->sysex_id < 1 || midi_config->sysex_id > 16) {
        return false;
    }
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        if (midi_config->cc_rx_map[i] != CC_DISABLED && midi_config->cc_rx_map[i] > MAX_CC) {
            return false;
        }
        if (midi_config->cc_tx_map[i] != CC_DISABLED && midi_config->cc_tx_map[i] > MAX_CC) {
            return false;
        }
    }
    memcpy(&h9->midi_config, midi_config, sizeof(*midi_config));
    return true;
}

void h9_copyMidiConfig(h9* h9, h9_midi_config* dest_copy) {
    if (dest_copy == NULL) {
        return;
    }
    memcpy(dest_copy, &h9->midi_config, sizeof(*dest_copy));
}

bool h9_dirty(h9* h9) {
    return h9->preset->dirty;
}

double now_ms(void) {
    struct timespec now;  // both C11 and POSIX
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    double now_ms = ((double)now.tv_sec + 10.0E-9 * (double)now.tv_nsec) * 1000.0;
    return now_ms;
}

void h9_cc(h9* h9, uint8_t cc_num, uint8_t cc_value) {
    uint8_t value = cc_value & 0x7F;

    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        if (h9->midi_config.cc_tx_map[i] == cc_num) {
            // i is the control listening to that CC, value is the MSB half

            // Timestamp and save this information in case the LSB half shows up soon after
            last_msb_cc             = cc_num;
            last_msb                = value;
            last_msb_timestamp_msec = now_ms();
            h9_setKnob(h9, (control_id)i, ((double)value / 127.0f));
            return;
        } else if (h9->midi_config.cc_tx_map[i] == (cc_num - 32)) {
            // i is the control listening to the CC, value is the LSB half

            if (last_msb_cc != cc_num) {
                return;  // Ignore, MSB shouldn't be sent randomly
            }

            double time_ms = now_ms();
            if ((time_ms - last_msb_timestamp_msec) > MIDI_ACCEPTABLE_LSB_DELAY_MS) {
                // It's been too long since we got the last MSB for this control, ignore and reset for next MSB.
                last_msb_cc = CC_DISABLED;
                return;
            }

            uint16_t high_res_cc   = (last_msb << 7) + value;
            double   control_value = (double)high_res_cc / (double)(1 << 14);
            h9_setKnob(h9, (control_id)i, control_value);
            last_msb_cc = CC_DISABLED;
            return;
        }
    }
}
