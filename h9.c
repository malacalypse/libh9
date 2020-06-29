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

#ifndef NDEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#define NUM_MODULES 5
#define KNOB_MAX 0x7FE0 // By observation

// Create
h9* h9_new(void) {
    h9* h9 = malloc(sizeof(h9));
    assert(h9 != NULL);
    memset(h9, 0x0, sizeof(*h9)); // pedantically explicitly zero
    h9->preset = malloc(sizeof(*h9->preset));
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

void hexdump(uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        debug("%02x", (int)data[i]);
        if ((i != 0) && ((i + 1) % 4 == 0)) {
            debug(" ");
        }
    }
    debug("\n");
}

size_t scanhex(char *str, uint32_t *dest, size_t len) {
    FILE *stream;
    size_t i = 0;

    stream = fmemopen(str, strlen(str), "r");
    for (i = 0; i < len; i++) {
        if (fscanf(stream, "%x", &dest[i]) != 1) {
            debug("Failed scanning ASCII hex: successfully read %zu of %zu values.\n", i, len);
            return i;
        }
    }

    debug("Scanned: ");
    for (size_t i = 0; i < len; i++) {
        debug("%d ", dest[i]);
    }
    debug("\n");

    return i;
}

size_t scanfloat(char* str, float *dest, size_t len) {
    FILE *stream;
    size_t i = 0;

    stream = fmemopen(str, strlen(str), "r");
    for (i = 0; i < len; i++) {
        if (fscanf(stream, "%f", &dest[i]) != 1) {
            debug("Failed scanning ASCII hex: successfully read %zu of %zu values.\n", i, len);
            return i;
        }
    }

    printf("Scanned: ");
    for (size_t i = 0; i < len; i++) {
        debug("%f ", dest[i]);
    }
    debug("\n");

    return i;
}

uint16_t array_sum(uint32_t *array, size_t len) {
    uint16_t result = 0U;
    for (size_t i = 0; i < len; i++) {
        result += array[i];
    }
    return result;
}

uint16_t iarray_sumf(float *array, size_t len) {
    uint16_t result = 0U;
    for (size_t i = 0; i < len; i++) {
        result += (uint16_t)truncf(array[i]);
    }
    return result;
}

void import_knob_values(h9_knob* knobs, size_t index, uint32_t* value_row) {
    h9_knob *knob = &knobs[index];

    size_t indices[] = { 9, 8, 7, 6, 5, 4, 0, 1, 2, 3 };
    uint32_t raw_value = value_row[indices[index]];
    knob->default_value = raw_value / (float)KNOB_MAX; // Scale between 0.0 and 1.0
    knob->current_value = knob->default_value;
    knob->display_value = knob->default_value;
}

void import_knob_map(h9_knob* knobs, size_t index, uint32_t *knob_expr_psw_row) {
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

void import_knob_mknob(h9_knob* knobs, size_t index, float* mknob_row) {
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
    return kH9_OK;
}

/*
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
*/
