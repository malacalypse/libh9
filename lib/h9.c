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
#include <stdarg.h>

#include "h9_modules.h"
#include "utils.h"

#define NUM_MODULES 5
#define KNOB_MAX 0x7FE0 // By observation

// Private
static void reset_knobs(h9* h9);
static void update_display_value(h9* h9, h9_knob* knob, knob_value value);

static void reset_knobs(h9* h9) {
    for (size_t i = 0; i < NUM_KNOBS; i++) {
        h9_knob* knob = &h9->preset->knobs[i];
        if (knob->exp_mapped || knob->psw_mapped) {
            update_display_value(h9, knob, knob->current_value);
        }
    }
}

// This exists to handle future callbacks or other dynamic behaviour
static void update_display_value(h9* h9, h9_knob* knob, knob_value value) {
    knob->display_value = knob->current_value;
}

// Create
h9* h9_new(void) {
    h9* h9 = malloc(sizeof(*h9));
    assert(h9 != NULL);
    h9->preset = malloc(sizeof(*(h9->preset)));
    assert(h9->preset != NULL);
    h9->sysex_id = 1U; // Default
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
static void import_knob_values(h9_knob* knobs, size_t index, uint32_t* value_row) {
    h9_knob *knob = &knobs[index];

    size_t indices[] = { 9, 8, 7, 6, 5, 4, 0, 1, 2, 3 };
    uint32_t raw_value = value_row[indices[index]];
    knob->default_value = raw_value / (float)KNOB_MAX; // Scale between 0.0 and 1.0
    knob->current_value = knob->default_value;
    knob->display_value = knob->default_value;
}

static void import_knob_map(h9_knob* knobs, size_t index, uint32_t *knob_expr_psw_row) {
    h9_knob *knob = &knobs[index];

    size_t min_indices[] = { 18, 16, 14, 12, 10, 8, 0, 2, 4, 6 };
    size_t max_indices[] = { 19, 17, 15, 13, 11, 9, 1, 3, 5, 7 };
    size_t psw_indices[] = { 29, 28, 27, 26, 25, 24, 20, 21, 22, 23 };
    knob->exp_min = knob_expr_psw_row[min_indices[index]] / (float)KNOB_MAX;
    knob->exp_max = knob_expr_psw_row[max_indices[index]] / (float)KNOB_MAX;
    knob->psw     = knob_expr_psw_row[psw_indices[index]] / (float)KNOB_MAX;
    knob->exp_mapped = (knob->exp_min != 0.0 || knob->exp_max != 0.0);
    knob->psw_mapped = (knob->psw != 0.0);
}

static void import_knob_mknob(h9_knob* knobs, size_t index, float* mknob_row) {
    h9_knob *knob = &knobs[index];

    size_t indices[] = { 9, 8, 7, 6, 5, 4, 0, 1, 2, 3 };
    knob->mknob_value = mknob_row[indices[index]];
}

// A sysex chunk, stripped of leading and trailing 0xF0/0xF7
h9_status h9_load(h9* h9, uint8_t* sysex, size_t len) {
    assert(h9);

    char *cursor = (char *)sysex; // start the cursor at the beginning

    // Validate preamble
    uint8_t preamble[] = { SYSEX_EVENTIDE, SYSEX_H9, h9->sysex_id, kH9_PRESET };
    bool preamble_validated = true;
    for (size_t i = 0; i < sizeof(preamble) / sizeof(*preamble); i++) {
        if (sysex[i] != preamble[i]) {
            preamble_validated = false;
        }
    }
    cursor += sizeof(preamble);
    if (!preamble_validated) {
        return kH9_SYSEX_PREAMBLE_INCORRECT;
    }

    // Debug
    debug("Debugging...\n");
    hexdump(sysex, len);
    debug("%.*s\n", (int)len, sysex); // Print as string
    debug("Debug complete.\n");


    // Need to unpack before we can validate the checksum
    int preset_num = 0;
    int module = 0;
    int algorithm = 0;
    uint32_t knob_values[12]; // Spec: 12 entries in this row, 11th is expr, 12th unknown, perhaps PSW
    uint32_t knob_map[30];    // Spec: 30 entries in this row
    uint32_t options[8];      // Spec: 8 entries in this row
    float mknob_values[12];   // Spec: 12 entries in this row, 11th/12th always seem to be 65000
    int checksum = 0;
    uint16_t computed_checksum = 0;
    char* patch_name;

    memset(knob_values, 0x0, sizeof(knob_values));
    memset(knob_map, 0x0, sizeof(knob_map));
    memset(options, 0x0, sizeof(options));
    memset(mknob_values, 0x0, sizeof(mknob_values));

    // Break up received data into lines
    size_t max_lines = 7;
    size_t max_length = 128; // Longest line is 4 hex chars * 30 positions + space, null, and \r\n = 124. 128 is comfortable, and ensures a terminating null.
    char *lines[max_lines];
    for (size_t i = 0; i < max_lines; i++) {
        lines[i] = malloc(sizeof(char) * max_length);
    }
    size_t found = sscanf(cursor, "%127[^\r\n]\r\n%127[^\r\n]\r\n%127[^\r\n]\r\n%127[^\r\n]\r\n%127[^\r\n]\r\n%127[^\r\n]\r\n%127[^\r\n]", lines[0], lines[1], lines[2], lines[3], lines[4], lines[5], lines[6]);

    if (found != max_lines) {
        printf("Did not find expected data. Retrieved only %zu lines.\n", found);
        return kH9_SYSEX_CHECKSUM_INVALID;
    }
    printf("Retrieved %zu lines:\n", found);
    for (size_t i = 0; i < max_lines; i++) {
        debug("%s\n", lines[i]);
    }

    // Unpack Line 1: [00] 0 0 0 => [<preset>] {module} {unknown, always 5} {algorithm}
    found = sscanf(lines[0], "[%d] %d %*d %d", &preset_num, &algorithm, &module);
    if (found != 3) {
        debug("Line 1 did not validate, found %zu.\n", found);
        return kH9_SYSEX_CHECKSUM_INVALID;
    }
    debug("Found [%d] => %d:%d\n", preset_num, module, algorithm);

    // Unpack Line 2: hex ascii knob values, order: 7 8 9 10 6 5 4 3 2 1 expression <unused>
    size_t expected_values = 12;
    found = scanhex(lines[1], knob_values, expected_values);
    if (found != expected_values) {
        debug("Line 2 did not validate, found %zu.\n", found);
        return kH9_SYSEX_CHECKSUM_INVALID;
    }
    // Unpack Line 3: hex ascii knob mapping, knob order: paired [exp min] [exp max] x 10 : [psw] x 10
    expected_values = 30;
    found = scanhex(lines[2], knob_map, expected_values);
    if (found != expected_values) {
        debug("Line 3 did not validate, found %zu.\n", found);
        return kH9_SYSEX_CHECKSUM_INVALID;
    }

    // Unpack Line 4:  0 [tempo * 100] [tempo enable = 1] [output gain * 10 in two's complement] [x] [y] [z] [modfactor fast/slow]
    expected_values = 8;
    found = scanhex(lines[3], options, expected_values);
    if (found != expected_values) {
        debug("Line 4 did not validate, found %zu.\n", found);
        return kH9_SYSEX_CHECKSUM_INVALID;
    }

    // Unpack Line 5: ascii float decimals x 12 : MKnob Values (unknown function, send back as-is)
    expected_values = 12;
    found = scanfloat(lines[4], mknob_values, expected_values);
    if (found != expected_values) {
        debug("Line 4 did not validate, found %zu.\n", found);
        return kH9_SYSEX_CHECKSUM_INVALID;
    }

    // Unpack Line 6: C_xxxx -> xxxx = ascii hex checksum (LSB) ** see note
    found = sscanf(lines[5], "C_%x", &checksum);
    if (found != 1) {
        debug("Did not identify checksum.\n");
        return kH9_SYSEX_CHECKSUM_INVALID;
    }

    // Unpack Line 7: ASCII string patch name
    patch_name = lines[6];

    //
    // NOTE: Checksum is computed by summing:
    //         1. The INTEGER values of each of the ASCII HEX from lines 2, 3, 4
    //         2. The INTEGER values of the floating point numbers from line 5
    //       The resulting INTEGER is then formatted as HEX and the last 4 characters are compared
    //       to the characters on line 6.

    uint16_t target_checksum = (uint16_t)checksum;
    computed_checksum = 0U;
    computed_checksum += array_sum(knob_values, 12);
    computed_checksum += array_sum(knob_map, 30);
    computed_checksum += array_sum(options, 8);
    computed_checksum += iarray_sumf(mknob_values, 12);

    if (computed_checksum != target_checksum) {
        debug("Checksum is invalid. Difference is %d\n", computed_checksum - target_checksum);
        return kH9_SYSEX_CHECKSUM_INVALID;
    }

    debug("Checksum VALID.\n");

    // Transform values to h9 state
    h9_preset* preset = h9->preset;
    size_t module_index = module - 1; // modules are 1-based, algorithms are 0-. Why? No clue.
    strncpy(preset->name, patch_name, MAX_NAME_LEN);
    preset->algorithm = &modules[module_index].algorithms[algorithm];
    preset->module = &modules[module_index];
    for (size_t i = 0; i < NUM_KNOBS; i++) {
        import_knob_values(preset->knobs, i, knob_values);
        import_knob_map(preset->knobs, i, knob_map);
        import_knob_mknob(preset->knobs, i, mknob_values);
    }
    preset->tempo = (float)options[1] / 100.0;
    preset->tempo_enabled = (options[2] != 0);
    preset->xyz_map[0] = options[4];
    preset->xyz_map[1] = options[5];
    preset->xyz_map[2] = options[6];
    preset->modfactor_fast_slow = (options[7] != 0);

    // Output gain is funky, it's premultiplied by 10 and is a standard 2s complement signed int, but with a 24-bit width.
    preset->output_gain = ((int32_t)(options[3] << 8) >> 8) * 0.1f;

    // Update the dirty flags
    h9->preset_dirty = false;
    h9->sync_dirty = false;

    return kH9_OK;
}

static uint32_t export_knob_value(float knob_value) {
    float interim = rintf(knob_value * KNOB_MAX);
    if (interim > KNOB_MAX) {
        interim = KNOB_MAX;
    } else if (interim < 0.0f) {
        interim = 0.0f;
    }
    return (uint32_t)interim;
}

bool h9_preset_loaded(h9* h9) {
    return (h9->preset->algorithm != NULL && h9->preset->module != NULL);
}

size_t h9_dump(h9* h9, uint8_t* sysex, size_t max_len, bool update_sync_dirty) {
    h9_preset* preset = h9->preset;
    if (!h9_preset_loaded(h9)) {
        return 0;
    }

    uint16_t checksum = 0U;
    uint32_t line2[] = {
        export_knob_value(preset->knobs[6].current_value),
        export_knob_value(preset->knobs[7].current_value),
        export_knob_value(preset->knobs[8].current_value),
        export_knob_value(preset->knobs[9].current_value),
        export_knob_value(preset->knobs[5].current_value),
        export_knob_value(preset->knobs[4].current_value),
        export_knob_value(preset->knobs[3].current_value),
        export_knob_value(preset->knobs[2].current_value),
        export_knob_value(preset->knobs[1].current_value),
        export_knob_value(preset->knobs[0].current_value),
        export_knob_value(h9->expression)
    };
    uint32_t line3[] = {
        export_knob_value(preset->knobs[6].exp_min),
        export_knob_value(preset->knobs[6].exp_max),
        export_knob_value(preset->knobs[7].exp_min),
        export_knob_value(preset->knobs[7].exp_max),
        export_knob_value(preset->knobs[8].exp_min),
        export_knob_value(preset->knobs[8].exp_max),
        export_knob_value(preset->knobs[9].exp_min),
        export_knob_value(preset->knobs[9].exp_max),
        export_knob_value(preset->knobs[5].exp_min),
        export_knob_value(preset->knobs[5].exp_max),
        export_knob_value(preset->knobs[4].exp_min),
        export_knob_value(preset->knobs[4].exp_max),
        export_knob_value(preset->knobs[3].exp_min),
        export_knob_value(preset->knobs[3].exp_max),
        export_knob_value(preset->knobs[2].exp_min),
        export_knob_value(preset->knobs[2].exp_max),
        export_knob_value(preset->knobs[1].exp_min),
        export_knob_value(preset->knobs[1].exp_max),
        export_knob_value(preset->knobs[0].exp_min),
        export_knob_value(preset->knobs[0].exp_max),
        export_knob_value(preset->knobs[6].psw),
        export_knob_value(preset->knobs[7].psw),
        export_knob_value(preset->knobs[8].psw),
        export_knob_value(preset->knobs[9].psw),
        export_knob_value(preset->knobs[5].psw),
        export_knob_value(preset->knobs[4].psw),
        export_knob_value(preset->knobs[3].psw),
        export_knob_value(preset->knobs[2].psw),
        export_knob_value(preset->knobs[1].psw),
        export_knob_value(preset->knobs[0].psw),
    };
    uint32_t line4[] = {
        (uint16_t)rintf(preset->tempo * 100.0f),
        (preset->tempo_enabled ? 1 : 0),
        (int16_t)rintf(preset->output_gain * 10.0f),
        preset->xyz_map[0],
        preset->xyz_map[1],
        preset->xyz_map[2],
        (preset->modfactor_fast_slow ? 1 : 0),
    };
    float line5[] = {
        65000.0f, 65000.0f, 65000.0f, 65000.0f, 65000.0f, 65000.0f, 65000.0f, 65000.0f, 65000.0f, 65000.0f, 65000.0f, 65000.0f
    };
    uint8_t line5_widths[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    // Compute checksum
    checksum += array_sum(line2, 11);
    checksum += array_sum(line3, 30);
    checksum += array_sum(line4, 7);
    checksum += iarray_sumf(line5, 12);

    size_t bytes_written = snprintf((char *)sysex,
                                    max_len,
                                    "\xf0%c%c%c%c"
                                    "[1] %d 5 %d\r\n"
                                    " %x %x %x %x %x %x %x %x %x %x %x 0\r\n"
                                    " %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\r\n"
                                    " 0 %x %x %x %x %x %x %x\r\n"
                                    " %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f\r\n"
                                    "C_%x\r\n"
                                    "%s\r\n",
                                    SYSEX_EVENTIDE, SYSEX_H9, h9->sysex_id, kH9_PRESET, // Preamble
                                    preset->algorithm->id, preset->module->sysex_num,   // Line 1, etc..
                                    line2[0], line2[1], line2[2], line2[3], line2[4], line2[5], line2[6], line2[7], line2[8], line2[9], line2[10],
                                    line3[0],  line3[1],  line3[2],  line3[3],  line3[4],  line3[5],  line3[6],  line3[7],  line3[8],  line3[9], line3[10], line3[11], line3[12], line3[13], line3[14], line3[15], line3[16], line3[17], line3[18], line3[19], line3[20], line3[21], line3[22], line3[23], line3[24], line3[25], line3[26], line3[27], line3[28], line3[29],
                                    line4[0], line4[1], line4[2], line4[3], line4[4], line4[5], line4[6],
                                    line5_widths[0], line5[0], line5_widths[1], line5[1], line5_widths[2], line5[2], line5_widths[3], line5[3], line5_widths[4], line5[4], line5_widths[5], line5[5], line5_widths[6], line5[6], line5_widths[7], line5[7], line5_widths[8], line5[8], line5_widths[9], line5[9], line5_widths[10], line5[10], line5_widths[11], line5[11],
                                    checksum,
                                    preset->name
                                    );
    bytes_written += 1; // Count the null byte
    if (max_len > (bytes_written + 1)) {
        sysex[bytes_written] = 0xF7;
        bytes_written += 1;

        if (update_sync_dirty) {
            h9->sync_dirty = false;
        }
    }
    return bytes_written;
}

// Knob, Expr, and PSW operations
void h9_setKnob(h9* h9, knob_id knob_num, knob_value value) {
    // Knobs can be set even if a preset isn't loaded.
    h9_knob* knob = &h9->preset->knobs[knob_num];
    knob->current_value = value;
    if (knob->current_value != knob->default_value) {
        h9->preset_dirty = true;
    }
    if (knob->current_value != knob->display_value) {
        update_display_value(h9, knob, knob->current_value);
    }
}


void h9_setKnobMap(h9* h9, knob_id knob_num, knob_value exp_min, knob_value exp_max, knob_value psw) {
    h9_knob* knob = &h9->preset->knobs[knob_num];
    knob->exp_min = exp_min;
    knob->exp_max = exp_max;
    knob->psw = psw;
}

knob_value h9_getKnob(h9* h9, knob_id knob_num) {
    h9_knob* knob = &h9->preset->knobs[knob_num];
    return knob->current_value;
}

knob_value h9_getKnobDisplay(h9* h9, knob_id knob_num) {
    h9_knob* knob = &h9->preset->knobs[knob_num];
    return knob->display_value;
}

void h9_getKnobMap(h9* h9, knob_id knob_num, knob_value* exp_min, knob_value* exp_max, knob_value* psw) {
    h9_knob* knob = &h9->preset->knobs[knob_num];
    *exp_min = knob->exp_min;
    *exp_max = knob->exp_max;
    *psw = knob->psw;
}

void h9_setExpr(h9* h9, knob_value value) {
    knob_value expval = clip(value, 0.0f, 1.0f);

    h9->expression = expval;
    for (size_t i = 0; i < NUM_KNOBS; i++) {
        h9_knob* knob = &h9->preset->knobs[i];
        if (knob->exp_mapped) {
            knob_value interpolated_value = (knob->exp_max - knob->exp_min) * expval + knob->exp_min;
            update_display_value(h9, knob, interpolated_value);
        }
    }
}

knob_value h9_getExpr(h9* h9) {
    return h9->expression;
}

void h9_setPsw(h9* h9, bool psw_on) {
    h9->psw = psw_on;

    for (size_t i = 0; i < NUM_KNOBS; i++) {
        h9_knob* knob = &h9->preset->knobs[i];
        if (knob->psw_mapped) {
            update_display_value(h9, knob, knob->psw);
        }
    }
}

bool h9_getPsw(h9* h9) {
    return h9->psw;
}

// Preset Operations
void h9_switchAlgorithm(h9* h9, uint8_t module_sysex_id, uint8_t algorithm_sysex_id) {
    assert(module_sysex_id > 0 && module_sysex_id <= NUM_MODULES);
    h9_module *module = &modules[module_sysex_id];
    assert(algorithm_sysex_id < module->num_algorithms);
    h9->preset->module = module;
    h9->preset->algorithm = &module->algorithms[algorithm_sysex_id];
    reset_knobs(h9);
}

// (S)Setters and getters for preset name, module (by name or number), and algorithm (by name or number).
size_t h9_numModules(h9* h9) {
    return NUM_MODULES;
}

size_t h9_numAlgorithms(h9* h9, uint8_t module_sysex_id) {
    assert(module_sysex_id > 0 && module_sysex_id <= NUM_MODULES);
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
    assert(module_sysex_id > 0 && module_sysex_id <= NUM_MODULES);
    return modules[module_sysex_id].name;
}

const char* h9_currentModuleName(h9* h9) {
    if (!h9_preset_loaded(h9)) {
        return NULL;
    }
    return h9->preset->module->name;
}

const char* const h9_algorithmName(uint8_t module_sysex_id, uint8_t algorithm_sysex_id) {
    assert(module_sysex_id > 0 && module_sysex_id <= NUM_MODULES);
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
    size_t bytes_written = snprintf((char *)sysex, max_len, "\xf0%c%c%c%c\xf7", SYSEX_EVENTIDE, SYSEX_H9, h9->sysex_id, kH9_DUMP_ONE);
    return bytes_written; // No +1 here, the f7 is the terminator.
}

size_t h9_sysexGenRequestSystemConfig(h9* h9, uint8_t* sysex, size_t max_len) {
    size_t bytes_written = snprintf((char *)sysex, max_len, "\xf0%c%c%c%c\xf7", SYSEX_EVENTIDE, SYSEX_H9, h9->sysex_id, kH9_GET_CONFIG);
    return bytes_written; // No +1 here, the f7 is the terminator.
}
