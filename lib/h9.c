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
#define DEFAULT_PRESET_NUM 1

#define DEBUG_LEVEL DEBUG_ERROR
#include "debug.h"

typedef struct h9_sysex_preset {
    int preset_num;
    int module;
    int algorithm;
    uint32_t knob_values[12]; // Spec: 12 entries in this row, 11th is expr, 12th unknown, perhaps PSW
    uint32_t knob_map[30];    // Spec: 30 entries in this row
    uint32_t options[8];      // Spec: 8 entries in this row
    float mknob_values[12];   // Spec: 12 entries in this row, 11th/12th always seem to be 65000
    int checksum;
    char patch_name[MAX_NAME_LEN];
} h9_sysex_preset;

// Private
static void reset_knobs(h9* h9);
static void update_display_value(h9* h9, h9_knob* knob, knob_value value);
static void dump_preset(h9_sysex_preset *sxpreset, h9_preset *preset, float expression);

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

static uint32_t export_knob_value(float knob_value) {
    float interim = rintf(knob_value * KNOB_MAX);
    if (interim > KNOB_MAX) {
        interim = KNOB_MAX;
    } else if (interim < 0.0f) {
        interim = 0.0f;
    }
    return (uint32_t)interim;
}

static float export_mknob_value(float knob_value) {
    return 65000.0f; // This value is always accepted by the pedal.
}

static void export_knob_values(uint32_t* value_row, size_t index, h9_knob* knobs) {
    h9_knob *knob = &knobs[index];

    size_t indices[] = { 9, 8, 7, 6, 5, 4, 0, 1, 2, 3 };
    value_row[indices[index]] = export_knob_value(knob->current_value);
}

static void export_knob_map(uint32_t* value_row, size_t index, h9_knob* knobs) {
    h9_knob *knob = &knobs[index];

    size_t min_indices[] = { 18, 16, 14, 12, 10, 8, 0, 2, 4, 6 };
    size_t max_indices[] = { 19, 17, 15, 13, 11, 9, 1, 3, 5, 7 };
    size_t psw_indices[] = { 29, 28, 27, 26, 25, 24, 20, 21, 22, 23 };
    value_row[min_indices[index]] = export_knob_value(knob->exp_min);
    value_row[max_indices[index]] = export_knob_value(knob->exp_max);
    value_row[psw_indices[index]] = export_knob_value(knob->psw);
}

static void export_knob_mknob(float *value_row, size_t index, h9_knob* knobs) {
    h9_knob *knob = &knobs[index];
    size_t indices[] = { 9, 8, 7, 6, 5, 4, 0, 1, 2, 3 };
    value_row[indices[index]] = export_mknob_value(knob->current_value);
}

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

static bool parse_h9_sysex_preset(uint8_t *sysex, h9_sysex_preset *sxpreset) {
    // Break up received data into lines
    size_t max_lines = 7;
    size_t max_length = 128; // Longest line is 4 hex chars * 30 positions + space, null, and \r\n = 124. 128 is comfortable, and ensures a terminating null.
    char *lines[max_lines];
    for (size_t i = 0; i < max_lines; i++) {
        lines[i] = malloc(sizeof(char) * max_length);
    }
    size_t found = sscanf((char *)sysex, "%127[^\r\n]\r\n%127[^\r\n]\r\n%127[^\r\n]\r\n%127[^\r\n]\r\n%127[^\r\n]\r\n%127[^\r\n]\r\n%127[^\r\n]", lines[0], lines[1], lines[2], lines[3], lines[4], lines[5], lines[6]);

    if (found != max_lines) {
        debug_info("Did not find expected data. Retrieved only %zu lines.\n", found);
        return false;
    }
    debug_info("Retrieved %zu lines:\n", found);
    for (size_t i = 0; i < max_lines; i++) {
        debug_info("%s\n", lines[i]);
    }

    // Unpack Line 1: [00] 0 0 0 => [<preset>] {module} {unknown, always 5} {algorithm}
    found = sscanf(lines[0], "[%d] %d %*d %d", &sxpreset->preset_num, &sxpreset->algorithm, &sxpreset->module);
    if (found != 3) {
        debug_info("Line 1 did not validate, found %zu.\n", found);
        return false;
    }
    debug_info("Found [%d] => %d:%d\n", sxpreset->preset_num, sxpreset->module, sxpreset->algorithm);

    // Unpack Line 2: hex ascii knob values, order: 7 8 9 10 6 5 4 3 2 1 expression <unused>
    size_t expected_values = 12;
    found = scanhex(lines[1], sxpreset->knob_values, expected_values);
    if (found != expected_values) {
        debug_info("Line 2 did not validate, found %zu.\n", found);
        return false;
    }
    // Unpack Line 3: hex ascii knob mapping, knob order: paired [exp min] [exp max] x 10 : [psw] x 10
    expected_values = 30;
    found = scanhex(lines[2], sxpreset->knob_map, expected_values);
    if (found != expected_values) {
        debug_info("Line 3 did not validate, found %zu.\n", found);
        return false;
    }

    // Unpack Line 4:  0 [tempo * 100] [tempo enable = 1] [output gain * 10 in two's complement] [x] [y] [z] [modfactor fast/slow]
    expected_values = 8;
    found = scanhex(lines[3], sxpreset->options, expected_values);
    if (found != expected_values) {
        debug_info("Line 4 did not validate, found %zu.\n", found);
        return false;
    }

    // Unpack Line 5: ascii float decimals x 12 : MKnob Values (unknown function, send back as-is)
    expected_values = 12;
    found = scanfloat(lines[4], sxpreset->mknob_values, expected_values);
    if (found != expected_values) {
        debug_info("Line 4 did not validate, found %zu.\n", found);
        return false;
    }

    // Unpack Line 6: C_xxxx -> xxxx = ascii hex checksum (LSB) ** see note
    found = sscanf(lines[5], "C_%x", &sxpreset->checksum);
    if (found != 1) {
        debug_info("Did not identify checksum.\n");
        return false;
    }

    // Unpack Line 7: ASCII string patch name
    memset(sxpreset->patch_name, 0x0, MAX_NAME_LEN);
    strncpy(sxpreset->patch_name, lines[6], MAX_NAME_LEN);

    return true;
}

static uint16_t checksum(h9_sysex_preset *sxpreset) {
    //
    // NOTE: Checksum is computed by summing:
    //         1. The INTEGER values of each of the ASCII HEX from lines 2, 3, 4
    //         2. The INTEGER values of the floating point numbers from line 5
    //       The resulting INTEGER is then formatted as HEX and the last 4 characters are compared
    //       to the characters on line 6.
    uint16_t checksum = 0U;
    checksum += array_sum(sxpreset->knob_values, 12);
    checksum += array_sum(sxpreset->knob_map, 30);
    checksum += array_sum(sxpreset->options, 8);
    checksum += iarray_sumf(sxpreset->mknob_values, 12);
    return checksum;
}

static bool validate_h9_sysex_preset(h9_sysex_preset *sxpreset) {
    bool is_valid = true;
    if (sxpreset->module < 1 || sxpreset->module > NUM_MODULES) {
        is_valid = false;
        return is_valid;
    }
    if (sxpreset->algorithm < 0 || sxpreset->algorithm >= modules[sxpreset->module - 1].num_algorithms) {
        is_valid = false;
        return is_valid;
    }
    return is_valid;
}

static void load_preset(h9_preset *preset, h9_sysex_preset *sxpreset) {
    // Transform values to h9 state
    size_t module_index = sxpreset->module - 1; // modules are 1-based, algorithms are 0-. Why? No clue.
    strncpy(preset->name, sxpreset->patch_name, MAX_NAME_LEN);
    preset->module = &modules[module_index];
    preset->algorithm = &modules[module_index].algorithms[sxpreset->algorithm];
    for (size_t i = 0; i < NUM_KNOBS; i++) {
        import_knob_values(preset->knobs, i, sxpreset->knob_values);
        import_knob_map(preset->knobs, i, sxpreset->knob_map);
        import_knob_mknob(preset->knobs, i, sxpreset->mknob_values);
    }
    preset->tempo = (float)sxpreset->options[1] / 100.0;
    preset->tempo_enabled = (sxpreset->options[2] != 0);
    preset->xyz_map[0] = sxpreset->options[4];
    preset->xyz_map[1] = sxpreset->options[5];
    preset->xyz_map[2] = sxpreset->options[6];
    preset->modfactor_fast_slow = (sxpreset->options[7] != 0);

    // Output gain is funky, it's premultiplied by 10 and is a standard 2s complement signed int, but with a 24-bit width.
    preset->output_gain = ((int32_t)(sxpreset->options[3] << 8) >> 8) * 0.1f;
}

static void dump_preset(h9_sysex_preset *sxpreset, h9_preset *preset, float expression) {
    sxpreset->module = preset->module->sysex_num;
    sxpreset->algorithm = preset->algorithm->id;
    sxpreset->preset_num = DEFAULT_PRESET_NUM;

    // Dump knob values
    for (size_t i = 0; i < NUM_KNOBS; i++) {
        export_knob_values(sxpreset->knob_values, i, preset->knobs);
        export_knob_map(sxpreset->knob_map, i, preset->knobs);
        export_knob_mknob(sxpreset->mknob_values, i, preset->knobs);
    }

    // Dump expression pedal value
    sxpreset->knob_values[10] = export_knob_value(expression);
    sxpreset->mknob_values[10] = export_mknob_value(expression);
    sxpreset->mknob_values[11] = export_mknob_value(KNOB_MAX); // Always seems to be constant.

    // Dump translated option values
    sxpreset->options[1] = (uint16_t)rintf(preset->tempo * 100.0f);
    sxpreset->options[2] = (preset->tempo_enabled ? 1 : 0);
    sxpreset->options[3] = (int16_t)rintf(preset->output_gain * 10.0f);
    sxpreset->options[4] = preset->xyz_map[0];
    sxpreset->options[5] = preset->xyz_map[1];
    sxpreset->options[6] = preset->xyz_map[2];
    sxpreset->options[7] = (preset->modfactor_fast_slow ? 1 : 0);

    // Preset name
    strncpy(sxpreset->patch_name, preset->name, MAX_NAME_LEN);

    // Finally, update the checksum
    sxpreset->checksum = checksum(sxpreset);
}

static size_t format_sysex(uint8_t *sysex, size_t max_len, h9_sysex_preset *sxpreset, uint8_t sysex_id) {
    // The mknob floats are formatted to ASCII. Eventually we might want to individually adjust
    // the width and precision, as the pedal seems to do.
    uint8_t mknob_widths[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    size_t bytes_written = snprintf((char *)sysex,
                                    max_len,
                                    "\xf0%c%c%c%c"
                                    "[%d] %d 5 %d\r\n"
                                    " %x %x %x %x %x %x %x %x %x %x %x 0\r\n"
                                    " %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\r\n"
                                    " %x %x %x %x %x %x %x %x\r\n"
                                    " %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f\r\n"
                                    "C_%x\r\n"
                                    "%s\r\n",
                                    SYSEX_EVENTIDE, SYSEX_H9, sysex_id, kH9_PRESET, // Preamble
                                    sxpreset->preset_num, sxpreset->algorithm, sxpreset->module,   // Line 1, etc..
                                    sxpreset->knob_values[0], sxpreset->knob_values[1], sxpreset->knob_values[2], sxpreset->knob_values[3], sxpreset->knob_values[4], sxpreset->knob_values[5], sxpreset->knob_values[6], sxpreset->knob_values[7], sxpreset->knob_values[8], sxpreset->knob_values[9], sxpreset->knob_values[10],
                                    sxpreset->knob_map[0],  sxpreset->knob_map[1],  sxpreset->knob_map[2],  sxpreset->knob_map[3],  sxpreset->knob_map[4],  sxpreset->knob_map[5],  sxpreset->knob_map[6],  sxpreset->knob_map[7],  sxpreset->knob_map[8],  sxpreset->knob_map[9], sxpreset->knob_map[10], sxpreset->knob_map[11], sxpreset->knob_map[12], sxpreset->knob_map[13], sxpreset->knob_map[14], sxpreset->knob_map[15], sxpreset->knob_map[16], sxpreset->knob_map[17], sxpreset->knob_map[18], sxpreset->knob_map[19], sxpreset->knob_map[20], sxpreset->knob_map[21], sxpreset->knob_map[22], sxpreset->knob_map[23], sxpreset->knob_map[24], sxpreset->knob_map[25], sxpreset->knob_map[26], sxpreset->knob_map[27], sxpreset->knob_map[28], sxpreset->knob_map[29],
                                    sxpreset->options[0], sxpreset->options[1], sxpreset->options[2], sxpreset->options[3], sxpreset->options[4], sxpreset->options[5], sxpreset->options[6], sxpreset->options[7],
                                    mknob_widths[0], sxpreset->mknob_values[0], mknob_widths[1], sxpreset->mknob_values[1], mknob_widths[2], sxpreset->mknob_values[2], mknob_widths[3], sxpreset->mknob_values[3], mknob_widths[4], sxpreset->mknob_values[4], mknob_widths[5], sxpreset->mknob_values[5], mknob_widths[6], sxpreset->mknob_values[6], mknob_widths[7], sxpreset->mknob_values[7], mknob_widths[8], sxpreset->mknob_values[8], mknob_widths[9], sxpreset->mknob_values[9], mknob_widths[10], sxpreset->mknob_values[10], mknob_widths[11], sxpreset->mknob_values[11],
                                    sxpreset->checksum,
                                    sxpreset->patch_name
                                    );
    bytes_written += 1; // Count the null byte
    if (max_len > (bytes_written + 1)) {
        sysex[bytes_written] = 0xF7;
        bytes_written += 1;
    }
    return bytes_written;
};

h9_status h9_load(h9* h9, uint8_t* sysex, size_t len) {
    assert(h9);
    char *cursor = (char *)sysex; // start the cursor at the beginning
    if (*cursor == (char)0xF0) {
        cursor++;
    }

    // Validate preamble
    uint8_t preamble[] = { SYSEX_EVENTIDE, SYSEX_H9, h9->sysex_id, kH9_PRESET };
    for (size_t i = 0; i < sizeof(preamble) / sizeof(*preamble); i++) {
        if (*cursor++ != preamble[i]) {
            return kH9_SYSEX_PREAMBLE_INCORRECT;
        }
    }

    // Debug
#if (DEBUG_LEVEL >= DEBUG_INFO)
    char sysex_buffer[1000];
    size_t bytes_written = hexdump(sysex_buffer, 1000, sysex, len);

    debug_info("Debugging...\n");
    debug_info("%*s\n", bytes_written, sysex_buffer);
    debug_info("%.*s\n", (int)len, sysex); // Print as string
    debug_info("Debug complete.\n");
#endif

    // Need to unpack before we can validate the checksum
    h9_sysex_preset sxpreset;
    memset(&sxpreset, 0x0, sizeof(sxpreset));
    if (!parse_h9_sysex_preset((uint8_t *)cursor, &sxpreset)) {
        return kH9_SYSEX_INVALID;
    }

    uint16_t computed_checksum = checksum(&sxpreset);
    if (sxpreset.checksum != computed_checksum) {
        debug_info("Checksum is invalid. Difference is %d\n", sxpreset.checksum - computed_checksum);
        return kH9_SYSEX_CHECKSUM_INVALID;
    }
    debug_info("Checksum VALID.\n");

    // Valiate contents - checksum is fine, but if the module / algorithm indices are invalid, we cannot continue.
    if (!validate_h9_sysex_preset(&sxpreset)) {
        return kH9_SYSEX_INVALID;
    }

    load_preset(h9->preset, &sxpreset);
    h9->preset_dirty = false;
    h9->sync_dirty = false;

    return kH9_OK;
}

bool h9_preset_loaded(h9* h9) {
    return (h9->preset->algorithm != NULL && h9->preset->module != NULL);
}

size_t h9_dump(h9* h9, uint8_t* sysex, size_t max_len, bool update_sync_dirty) {
    if (!h9_preset_loaded(h9)) {
        return 0;
    }

    h9_sysex_preset sxpreset;
    memset(&sxpreset, 0x0, sizeof(sxpreset));
    dump_preset(&sxpreset, h9->preset, h9->expression);

    size_t bytes_written = format_sysex(sysex, max_len, &sxpreset, h9->sysex_id);
    if (bytes_written <= max_len && update_sync_dirty) {
        h9->sync_dirty = false;
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
