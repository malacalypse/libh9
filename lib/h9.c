//
//  h9.c
//  The H9 core model
//
//  Created by Studio DC on 2020-06-24.
//

#include "h9.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "h9_modules.h"
#include "utils.h"

#define MIDI_MAX 32767 // 2^15 - 1 for 14-bit MIDI

#define DEBUG_LEVEL DEBUG_ERROR
#include "debug.h"

// Private
static void reset_knobs(h9* h9);
static void update_display_value(h9* h9, control_id control, control_value value);

static void reset_knobs(h9* h9) {
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9_knob* knob = &h9->preset->knobs[i];
        update_display_value(h9, (control_id)i, knob->current_value);
    }
}

// This exists to handle future callbacks or other dynamic behaviour
static void update_display_value(h9* h9, control_id control, control_value value) {
    h9_knob* knob = &h9->preset->knobs[control];
    knob->display_value = knob->current_value;
    if (h9->display_callback != NULL) {
        h9->display_callback(h9, control, value);
    }
}

static void h9_setExpr(h9* h9, control_value value) {
    control_value expval = clip(value, 0.0f, 1.0f);

    h9->expression = expval;
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9_knob* knob = &h9->preset->knobs[i];
        if (knob->exp_mapped) {
            control_value interpolated_value = (knob->exp_max - knob->exp_min) * expval + knob->exp_min;
            update_display_value(h9, (control_id)i, interpolated_value);
        }
    }
    if (h9->display_callback != NULL) {
        h9->display_callback(h9, EXPR, value);
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
            update_display_value(h9, (control_id)i, knob->psw);
        }
    }
    if (h9->display_callback != NULL) {
        h9->display_callback(h9, PSW, psw_on ? 1.0 : 0.0f);
    }
}

static bool h9_getPsw(h9* h9) {
    return h9->psw;
}

// Create
h9* h9_new(void) {
    h9* h9 = malloc(sizeof(*h9));
    assert(h9 != NULL);
    h9->preset = malloc(sizeof(*(h9->preset)));
    assert(h9->preset != NULL);
    h9->sysex_id = 1U; // Default
    h9->expression = 0.0f;
    return h9;
}

// Free / Delete
void h9_free(h9* h9) {
    if (h9 == NULL) {
        return;
    }
    if (h9->preset != NULL) {
        free(h9->preset);
    }
    free(h9);
}

// Common H9 operations

static void cc_callback(h9* h9, control_id control, float value) {
    if ((h9->cc_callback == NULL) || (h9->midi_config.cc_tx_map[control] == 0)) {
        return;
    }
    uint8_t midi_channel = h9->midi_config.midi_channel;
    uint8_t control_cc = h9->midi_config.cc_tx_map[control];
    uint16_t cc_value = clip(value, 0.0f, 1.0f) * MIDI_MAX;
    h9->cc_callback(h9, midi_channel, control_cc, (uint8_t)(cc_value >> 8), (uint8_t)cc_value);
}

bool h9_preset_loaded(h9* h9) {
    return (h9->preset->algorithm != NULL && h9->preset->module != NULL);
}

// Knob, Expr, and PSW operations
void h9_setControl(h9* h9, control_id control, control_value value) {
    if (control >= NUM_CONTROLS) {
        return; // Control is invalid
    }

    // Knobs can be set even if a preset isn't loaded.
    h9_knob* knob;
    switch(control) {
        case EXPR:
            h9_setExpr(h9, value);
            break;
        case PSW:
            h9_setPsw(h9, (value > 0));
            break;
        default: // A knob
            knob = &h9->preset->knobs[control];
            knob->current_value = value;
            h9->dirty = true;
            if (knob->current_value != knob->display_value) {
                update_display_value(h9, control, knob->current_value);
            }
    }
    cc_callback(h9, control, value);
}

void h9_setKnobMap(h9* h9, control_id knob_num, control_value exp_min, control_value exp_max, control_value psw) {
    if (knob_num > KNOB9) {
        return;
    }
    h9_knob* knob = &h9->preset->knobs[knob_num];
    knob->exp_min = exp_min;
    knob->exp_max = exp_max;
    knob->psw = psw;
}

control_value h9_getControl(h9* h9, control_id control) {
    h9_knob* knob;

    switch(control) {
        case EXPR:
            return h9_getExpr(h9);
        case PSW:
            return h9_getPsw(h9) ? 0.0f : 1.0f;
        default:
            if (control > KNOB9) {
                return -1.0f;
            }
            knob = &h9->preset->knobs[control];
            return knob->current_value;
    }
}

control_value h9_getKnobDisplay(h9* h9, control_id knob_num) {
    if (knob_num > KNOB9) {
        return -1.0f;
    }
    h9_knob* knob = &h9->preset->knobs[knob_num];
    return knob->display_value;
}

void h9_getKnobMap(h9* h9, control_id knob_num, control_value* exp_min, control_value* exp_max, control_value* psw) {
    if (knob_num > KNOB9) {
        return;
    }
    h9_knob* knob = &h9->preset->knobs[knob_num];
    *exp_min = knob->exp_min;
    *exp_max = knob->exp_max;
    *psw = knob->psw;
}

// Preset Operations
void h9_switchAlgorithm(h9* h9, uint8_t module_sysex_id, uint8_t algorithm_sysex_id) {
    assert(module_sysex_id > 0 && module_sysex_id <= H9_NUM_MODULES);
    h9_module *module = &modules[module_sysex_id];
    assert(algorithm_sysex_id < module->num_algorithms);
    h9->preset->module = module;
    h9->preset->algorithm = &module->algorithms[algorithm_sysex_id];
    h9->dirty = true;
    reset_knobs(h9);
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

// Syncing
size_t h9_sysexGenRequestCurrentPreset(h9* h9, uint8_t* sysex, size_t max_len) {
    size_t bytes_written = snprintf((char *)sysex, max_len, "\xf0%c%c%c%c\xf7", H9_SYSEX_EVENTIDE, H9_SYSEX_H9, h9->sysex_id, kH9_DUMP_ONE);
    return bytes_written; // No +1 here, the f7 is the terminator.
}

size_t h9_sysexGenRequestSystemConfig(h9* h9, uint8_t* sysex, size_t max_len) {
    size_t bytes_written = snprintf((char *)sysex, max_len, "\xf0%c%c%c%c\xf7", H9_SYSEX_EVENTIDE, H9_SYSEX_H9, h9->sysex_id, kH9_TJ_SYSVARS_WANT);
    return bytes_written; // No +1 here, the f7 is the terminator.
}
