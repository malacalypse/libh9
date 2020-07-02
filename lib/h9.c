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

#include "h9_module.h"
#include "h9_modules.h"
#include "utils.h"

#define MIDI_MAX          16383  // 2^14 - 1 for 14-bit MIDI
#define DEFAULT_MODULE    4      // zero-indexed
#define DEFAULT_ALGORITHM 0
#define DEFAULT_KNOB_CC   22
#define DEFAULT_EXPR_CC   15
#define DEFAULT_PSW_CC    CC_DISABLED
#define EMPTY_PRESET_NAME "Empty"

#define DEBUG_LEVEL DEBUG_ERROR
#include "debug.h"

/* ==== Private Function Declarations ============================================= */
static void          h9_setExpr(h9* h9, control_value value);
static control_value h9_getExpr(h9* h9);
static void          h9_setPsw(h9* h9, bool psw_on);
static bool          h9_getPsw(h9* h9);

/* ==== MODULE Private Function Definitions (implements h9_module.h) ============== */
void h9_reset_knobs(h9* h9) {
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9_knob* knob = &h9->preset->knobs[i];
        h9_update_display_value(h9, (control_id)i, knob->current_value);
    }
}

// This exists to handle future callbacks or other dynamic behaviour
void h9_update_display_value(h9* h9, control_id control, control_value value) {
    h9_knob* knob       = &h9->preset->knobs[control];
    knob->display_value = knob->current_value;
    if (h9->display_callback != NULL) {
        h9->display_callback(h9, control, value);
    }
}

/* ==== Private Functions ========================================================= */

static void h9_setExpr(h9* h9, control_value value) {
    control_value expval = clip(value, 0.0f, 1.0f);

    h9->expression = expval;
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9_knob* knob = &h9->preset->knobs[i];
        if (knob->exp_mapped) {
            control_value interpolated_value = (knob->exp_max - knob->exp_min) * expval + knob->exp_min;
            h9_update_display_value(h9, (control_id)i, interpolated_value);
        }
    }
    if (h9->display_callback != NULL) {
        h9->display_callback(h9->callback_context, EXPR, value);
    }
}

static control_value h9_getExpr(h9* h9) {
    return h9->expression;
}

static void h9_setPsw(h9* h9, bool psw_on) {
    h9->psw = psw_on;

    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9_knob* knob = &h9->preset->knobs[i];
        if (knob->psw_mapped) {
            h9_update_display_value(h9, (control_id)i, knob->psw);
        }
    }
    if (h9->display_callback != NULL) {
        h9->display_callback(h9->callback_context, PSW, psw_on ? 1.0 : 0.0f);
    }
}

static bool h9_getPsw(h9* h9) {
    return h9->psw;
}

static void cc_callback(h9* h9, control_id control, float value) {
    if ((h9->cc_callback == NULL) || (h9->midi_config.cc_rx_map[control] == CC_DISABLED)) {
        return;
    }
    uint8_t  midi_channel = h9->midi_config.midi_channel;
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
    h9->midi_config.sysex_id     = 1U;
    h9->midi_config.midi_channel = 0U;
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9->midi_config.cc_rx_map[i] = DEFAULT_KNOB_CC + i;
        h9->midi_config.cc_tx_map[i] = DEFAULT_KNOB_CC + i;
    }
    h9->midi_config.cc_rx_map[EXPR] = CC_DISABLED;  // Per the user guide, the default for RX is disabled.
    h9->midi_config.cc_tx_map[EXPR] = DEFAULT_EXPR_CC;
    h9->midi_config.cc_rx_map[PSW]  = DEFAULT_PSW_CC;
    h9->midi_config.cc_tx_map[PSW]  = DEFAULT_PSW_CC;

    h9->expression       = 0.0f;
    h9->cc_callback      = NULL;
    h9->display_callback = NULL;
    h9->sysex_callback   = NULL;
    h9->callback_context = (void *)h9;
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

    return h9_preset;
}

// Common H9 operations

bool h9_preset_loaded(h9* h9) {
    return (h9->preset->algorithm != NULL && h9->preset->module != NULL);
}

// Knob, Expr, and PSW operations
void h9_setControl(h9* h9, control_id control, control_value value, h9_callback_action cc_cb_action) {
    if (control >= NUM_CONTROLS) {
        return;  // Control is invalid
    }

    // Knobs can be set even if a preset isn't loaded.
    h9_knob* knob;
    switch (control) {
        case EXPR:
            h9_setExpr(h9, value);
            break;
        case PSW:
            h9_setPsw(h9, (value > 0.0f));
            break;
        default:  // A knob
            knob                = &h9->preset->knobs[control];
            knob->current_value = value;
            h9->dirty           = true;
            if (knob->current_value != knob->display_value) {
                h9_update_display_value(h9->callback_context, control, knob->current_value);
            }
    }
    if (cc_cb_action == kH9_TRIGGER_CALLBACK) {
        cc_callback(h9->callback_context, control, value);
    }
}

void h9_setKnobMap(h9* h9, control_id knob_num, control_value exp_min, control_value exp_max, control_value psw) {
    if (knob_num > KNOB9) {
        return;
    }
    h9_knob* knob = &h9->preset->knobs[knob_num];
    knob->exp_min = exp_min;
    knob->exp_max = exp_max;
    knob->psw     = psw;
}

control_value h9_controlValue(h9* h9, control_id control) {
    if (control > NUM_CONTROLS) {
        return -1.0f;
    }

    h9_knob* knob;

    switch (control) {
        case EXPR:
            return h9_getExpr(h9);
        case PSW:
            return h9_getPsw(h9) ? 1.0f : 0.0f;
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
void h9_setAlgorithm(h9* h9, uint8_t module_sysex_id, uint8_t algorithm_sysex_id) {
    assert(module_sysex_id > 0 && module_sysex_id <= H9_NUM_MODULES);
    h9_module* module = &modules[module_sysex_id];
    assert(algorithm_sysex_id < module->num_algorithms);
    h9->preset->module    = module;
    h9->preset->algorithm = &module->algorithms[algorithm_sysex_id];
    h9->dirty             = true;
    h9_reset_knobs(h9);
}

// (S)Setters and getters for preset name, module (by name or number), and algorithm (by name or number).
size_t h9_numModules(h9* h9) {
    return H9_NUM_MODULES;
}

size_t h9_numAlgorithms(h9* h9, uint8_t module_sysex_id) {
    assert(module_sysex_id > 0 && module_sysex_id <= H9_NUM_MODULES);
    return modules[module_sysex_id - 1].num_algorithms;
}

int8_t h9_currentModule(h9* h9) {
    if (!h9_preset_loaded(h9)) {
        return -1;
    }
    return h9->preset->module->sysex_num;
}
int8_t h9_currentAlgorithm(h9* h9) {
    if (!h9_preset_loaded(h9)) {
        return -1;
    }
    return h9->preset->algorithm->id;
}

const char* const h9_moduleName(uint8_t module_sysex_id) {
    assert(module_sysex_id > 0 && module_sysex_id <= H9_NUM_MODULES);
    return modules[module_sysex_id].name;
}

const char* h9_currentModuleName(h9* h9) {
    if (!h9_preset_loaded(h9)) {
        return NULL;
    }
    return h9->preset->module->name;
}

const char* const h9_algorithmName(uint8_t module_sysex_id, uint8_t algorithm_sysex_id) {
    assert(module_sysex_id > 0 && module_sysex_id <= H9_NUM_MODULES);
    h9_module* module = &modules[module_sysex_id];
    assert(algorithm_sysex_id < module->num_algorithms);
    return module->algorithms[algorithm_sysex_id].name;
}

const char* h9_currentAlgorithmName(h9* h9) {
    if (!h9_preset_loaded(h9)) {
        return NULL;
    }
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
    return h9->dirty;
}
