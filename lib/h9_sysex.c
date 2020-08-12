/*  h9_sysex.c
    This file is part of libh9, a library for remotely managing Eventide H9
    effects pedals.

    Copyright (C) 2020 Daniel Collins

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "h9_sysex.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "h9_module.h"
#include "libh9.h"
#include "utils.h"

#define DEBUG_LEVEL DEBUG_ERROR
#include "debug.h"

#define KNOB_MAX           0x7FE0  // By observation
#define DEFAULT_PRESET_NUM 1

extern h9_module h9_modules[H9_NUM_MODULES];

typedef enum h9_message_code {
    kH9_SYSEX_OK          = 0x0,
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

#define SYSVAR_BOOL_BASE   0x100
#define SYSVAR_BYTE_BASE   0x200
#define SYSVAR_WORD_BASE   0x300
#define SYSVAR_DUMMY_BASE  0x400
#define SYSVAR_CC_DISABLED 0x0
#define SYSVAR_CC_BASE     0x5  // CC0 value

typedef enum h9_sysvar {
    // Boolean read/write (set to 0 or 1, returns 0 or 1)
    unused1 = SYSVAR_BOOL_BASE + 0,  // 0 + 0x100
    unused2,                         // 1
    sp_bypass,                       // 2
    sp_kill_dry_global,              // 3 : The global setting. There's also one in the preset (see DUMMY section); the two together apply to the effective value.
    sp_midi_in,                      // 4 : 0 = DIN, 1 = USB
    unused3,                         // 5
    unused4,                         // 6
    sp_tap_syn,                      // 7 : tempo enabled
    unused5,                         // 8
    unused6,                         // 9
    sp_midiclock_enable,             // 10
    sp_tx_midi_cc,                   // 11
    sp_tx_midi_pchg,                 // 12
    sp_global_mix,                   // 13
    unused7,                         // 14
    unused8,                         // 15
    sp_global_tempo,                 // 16 : Use global tempo (= 1) or preset (= 0)
    sp_mod_display,                  // 17 : MF only, UNUSED for H9
    sp_midiclock_out,                // 18
    sp_midiclock_filter,             // 19
    sp_pedal_locked,                 // 20 : TF/SPC Only, UNUSED for H9
    sp_bluetooth_disabled,           // 21 : H9 Only
    sp_x_unlocked,                   // 22 : H9 Only, put x button in expert mode (= 1) or not (= 0)
    sp_y_unlocked,                   // 23 : Ibid, but y.
    sp_z_unlocked,                   // 24 : Ibid, but z.
    sp_pedal_cal_disabled,           // 25 : H9 Only
    unused9,                         // 26
    sp_ui_tempo_mode,                // 27 : put switch 3 in TAP mode (H9 Only)
    sp_blue_midi_enable,             // 28
    sp_global_inswell,               // 29
    sp_global_outswell,              // 30
    sp_send_PC_on_rx_PC,             // 31 : send PC when PC received

    // Byte params
    sp_bypass_mode = SYSVAR_BYTE_BASE + 0,  // 0 + 0x200
    unused10,                               // 1
    sp_startup_mode,                        // 2 : 0 = effect, 1 = preset
    sp_midi_rx_channel,                     // 3 : 0 = disabled, 1 = OMNI, 2-17 = 0-15
    sp_sysex_id,                            // 4 : MIDI sysex id number, 0 is usually broadcast. Use with caution. 1 is default.
    unused11,                               // 5
    sp_num_banks_lo,                        // 6
    unused12,                               // 7
    sp_midi_tx_channel,                     // 8 : 0 to 15
    sp_dump_type,                           // 9
    sp_num_banks,                           // 10
    sp_tap_average,                         // 11
    // Some source mappings skipped, only using the ones we really care about
    // Value for _src columns: 0 = OFF/DISABLED, 5 = CC0 ...
    sp_psw_src = SYSVAR_BYTE_BASE + 16,    // 16 : PSW
    sp_kb1_src = SYSVAR_BYTE_BASE + 18,    // 18 : Knob 0
    sp_kb2_src,                            // 19
    sp_kb3_src,                            // 20
    sp_kb4_src,                            // 21
    sp_kb5_src,                            // 22
    sp_kb6_src,                            // 23
    sp_kb7_src,                            // 24
    sp_kb8_src,                            // 25
    sp_kb9_src,                            // 26
    sp_kb10_src,                           // 27 : Knob 9
    sp_pdl_src   = SYSVAR_BYTE_BASE + 32,  // 32 : expression pedal
    sp_knob_mode = SYSVAR_BYTE_BASE + 69,  // 69

    // WORD parameters (a WORD is a UINT16_t in H9 parlance)
    sp_os_version = SYSVAR_WORD_BASE,       // 0 + 0x300
    sp_mix_knob,                            // 1
    sp_tempo,                               // 2
                                            // Mapping skipped - it only sets knob limits, not actual expression mapping.
                                            // You still have to load the preset, update the map, and dump it back.
    sp_input_gain = SYSVAR_WORD_BASE + 43,  // 43 : In 0.5 dB steps
    sp_output_gain,                         // 44 : In 0.5 dB steps
    sp_version,                             // 45 : encoded (v[0] << 12) + (v[1] << 8) + v[2] (x.y.z[a] - not including a)
    sp_pedal_cal_min,                       // 46
    sp_pedal_cal_max,                       // 47
    bluetoothPIN01,  // 48: Returns an ASCII integer string (e.g. "14129") which when converted to a uint16 (e.g. 0x3731) and then case as a char* are two ASCII characters, (e.g.
                     // "17"). PIN01 are the first two chars PIN32 are the last two, so if PIN01 decodes as "17" and PIN32 decodes as "53", the actual PIN is 1753. This indicates
                     // that the data is little-endian, read out as uint16_t pairs instead of chars.
    bluetoothPIN23,  // 49
    name01,          // 50 : Name field is encoded in the same ASCII integer string -> uint16 -> char[2] fielding.
    name23,  // 51 : Names can be up to 16 characters long. If they are shorter, they will end in a space and, if room, a NULL. While spaces are permitted in the name, the final
             // space before a NULL (or the last space if in the 16th character) is NOT part of the name.
    name45,  // 52
    name67,  // 53
    name89,  // 54
    nameAB,  // 55
    nameCD,  // 56
    nameEF,  // 57

    // DUMMY params (not saved in NVRAM between rebeoots, but some of these values affect the loaded preset)
    // NOTE: It appears that these actually don't exist or work at all - the pedal does not respond to them.
    sp_midiclock_present = SYSVAR_DUMMY_BASE,  // 0
    sp_preset_dirty,                           // 1
    sp_hotswitch_state,                        // 2 : TODO investigate if setting this to 1 is enough instead of fooling with the MIDI
    sp_preset_outgain,                         // 3 : The main preset output gain control.
                                               //     Twiddle this to affect the gain in realtime, but you must save it with the preset for it to "stick".
    sp_product_type,                           // 4
    sp_transient_preset,                       // 5
    sp_x_switch,                               // 6
    sp_y_switch,                               // 7
    sp_z_switch,                               // 8
    sp_bluetooth_connected,                    // 9
    sp_tuner_note,                             // 10
    sp_tuner_cents,                            // 11
    sp_performance_switch,                     // 12
    sp_preset_loading,                         // 13
    sp_slow,                                   // 14
    sp_inswell_enabled,                        // 15
    sp_outswell_enabled,                       // 16
    sp_routing_type,                           // 17
    sp_spill_done,                             // 18
    sp_kill_dry,                               // 19 : this is the combination of the global value and the preset value
    sp_preset_kill_dry,                        // 20 : this is the value stored in the preset
} h9_sysvar;

typedef struct h9_sysex_blob {
    h9_message_code type;
    uint8_t         dest_id;
    uint8_t *       data;
    size_t          len;
} h9_sysex_blob;

typedef struct h9_sysex_preset {
    int      preset_num;
    int      module_sysex_id;
    int      algorithm;
    int      algorithm_repeat;
    uint32_t control_values[11];  // Spec: 12 entries in this row, 0th is algorithm (again), 1st is knob7, 11th is expr
    uint32_t knob_map[30];        // Spec: 30 entries in this row
    uint32_t options[8];          // Spec: 8 entries in this row
    float    mknob_values[12];    // Spec: 12 entries in this row, 11th/12th always seem to be 65000
    int      checksum;
    char     patch_name[H9_MAX_NAME_LEN];
} h9_sysex_preset;

typedef struct h9_system_value_dump {
    uint8_t  byte_values[94];
    uint16_t word_values[58];
    bool     bit_values[32];
    uint32_t bitword;  // Bit values as a word
} h9_system_value_dump;

//////////////////// Private Function Declarations
static void      export_preset(h9_sysex_preset *sxpreset, h9_preset *preset);
static h9_status load_preset(h9 *h9, uint8_t *cursor, size_t len);

//////////////////// Private Functions

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
    return 65000.0f;  // This value is always accepted by the pedal.
}

static void export_knob_values(uint32_t *value_row, size_t index, h9_knob *knobs) {
    h9_knob *knob = &knobs[index];

    size_t indices[]          = {9, 8, 7, 6, 5, 4, 0, 1, 2, 3};
    value_row[indices[index]] = export_knob_value(knob->current_value);
}

static void export_knob_map(uint32_t *value_row, size_t index, h9_knob *knobs) {
    h9_knob *knob = &knobs[index];

    size_t min_indices[]          = {18, 16, 14, 12, 10, 8, 0, 2, 4, 6};
    size_t max_indices[]          = {19, 17, 15, 13, 11, 9, 1, 3, 5, 7};
    size_t psw_indices[]          = {29, 28, 27, 26, 25, 24, 20, 21, 22, 23};
    value_row[min_indices[index]] = export_knob_value(knob->exp_min);
    value_row[max_indices[index]] = export_knob_value(knob->exp_max);
    value_row[psw_indices[index]] = export_knob_value(knob->psw);
}

static void export_knob_mknob(float *value_row, size_t index, h9_knob *knobs) {
    h9_knob *knob             = &knobs[index];
    size_t   indices[]        = {9, 8, 7, 6, 5, 4, 0, 1, 2, 3};
    value_row[indices[index]] = export_mknob_value(knob->current_value);
}

static float import_control_value(uint32_t sysex_value) {
    return (float)sysex_value / (float)KNOB_MAX;
}

static void import_control_values(h9_preset *preset, uint32_t *value_row) {
    const size_t indices[] = {9, 8, 7, 6, 5, 4, 0, 1, 2, 3};

    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9_knob *knob       = &preset->knobs[i];
        uint32_t raw_value  = value_row[indices[i]];
        knob->current_value = import_control_value(raw_value);
        knob->display_value = knob->current_value;
    }
    preset->expression = import_control_value(value_row[10]);
    preset->psw        = (value_row[11] > 0);
}

static void import_knob_map(h9_preset *preset, uint32_t *knob_expr_psw_row) {
    size_t min_indices[] = {18, 16, 14, 12, 10, 8, 0, 2, 4, 6};
    size_t max_indices[] = {19, 17, 15, 13, 11, 9, 1, 3, 5, 7};
    size_t psw_indices[] = {29, 28, 27, 26, 25, 24, 20, 21, 22, 23};

    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9_knob *knob = &preset->knobs[i];

        knob->exp_min    = import_control_value(knob_expr_psw_row[min_indices[i]]);
        knob->exp_max    = import_control_value(knob_expr_psw_row[max_indices[i]]);
        knob->psw        = import_control_value(knob_expr_psw_row[psw_indices[i]]);
        knob->exp_mapped = (knob->exp_min != 0.0 || knob->exp_max != 0.0);
        knob->psw_mapped = (knob->psw != 0.0);
    }
}

static void import_mknob_values(h9_preset *preset, float *mknob_row) {
    size_t indices[] = {9, 8, 7, 6, 5, 4, 0, 1, 2, 3};

    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        h9_knob *knob     = &preset->knobs[i];
        knob->mknob_value = mknob_row[indices[i]];
    }
}

static bool unpack_preset(uint8_t *sysex, size_t len, h9_sysex_preset *sxpreset) {
    // Break up received data into lines
    size_t max_lines = 7;
    char * lines[max_lines];
    size_t lengths[max_lines];
    size_t found = find_lines((char *)sysex, len, lines, lengths, max_lines);

    if (found != max_lines) {
        debug_info("Did not find expected data. Retrieved only %zu lines.\n", found);
        return false;
    }
    debug_info("Retrieved %zu lines:\n", found);
    for (size_t i = 0; i < max_lines; i++) {
        debug_info("%s\n", lines[i]);
    }

    // Unpack Line 1: [00] 0 0 0 => [<preset>] {module} {unknown, always 5} {algorithm}
    found = sscanf(lines[0], "[%d] %d %*d %d", &sxpreset->preset_num, &sxpreset->algorithm, &sxpreset->module_sysex_id);
    if (found != 3) {
        debug_info("Line 1 did not validate, found %zu.\n", found);
        return false;
    }
    debug_info("Found [%d] => %d:%d\n", sxpreset->preset_num, sxpreset->module, sxpreset->algorithm);

    // Unpack Line 2: hex ascii knob values, order: <alg repeat> 7 8 9 10 6 5 4 3 2 1 <expression>
    size_t   expected_values = 12;
    uint32_t line_values[12];
    found = scanhex(lines[1], 100, line_values, expected_values);
    if (found != expected_values) {
        debug_info("Line 2 did not validate, found %zu.\n", found);
        return false;
    }
    // Assign them to the sxpreset
    sxpreset->algorithm_repeat = line_values[0];
    for (size_t i = 0; i < 11; i++) {
        sxpreset->control_values[i] = line_values[i + 1];
    }

    // Unpack Line 3: hex ascii knob mapping, knob order: paired [exp min] [exp max] x 10 : [psw] x 10
    expected_values = 30;
    found           = scanhex(lines[2], lengths[2], sxpreset->knob_map, expected_values);
    if (found != expected_values) {
        debug_info("Line 3 did not validate, found %zu.\n", found);
        return false;
    }

    // Unpack Line 4:  0 [tempo * 100] [tempo enable = 1] [output gain * 10 in two's complement] [x] [y] [z] [modfactor
    // fast/slow]
    expected_values = 8;
    found           = scanhex(lines[3], lengths[3], sxpreset->options, expected_values);
    if (found != expected_values) {
        debug_info("Line 4 did not validate, found %zu.\n", found);
        return false;
    }

    // Unpack Line 5: ascii float decimals x 12 : MKnob Values (unknown function, send back as-is)
    expected_values = 12;
    found           = scanfloat(lines[4], lengths[4], sxpreset->mknob_values, expected_values);
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
    memset(sxpreset->patch_name, 0x0, H9_MAX_NAME_LEN);
    strncpy(sxpreset->patch_name, lines[6], lengths[6] > H9_MAX_NAME_LEN ? H9_MAX_NAME_LEN : lengths[6]);

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
    checksum += sxpreset->algorithm_repeat;
    checksum += array_sum(sxpreset->control_values, 11);
    checksum += array_sum(sxpreset->knob_map, 30);
    checksum += array_sum(sxpreset->options, 8);
    checksum += iarray_sumf(sxpreset->mknob_values, 12);
    return checksum;
}

static bool validate_h9_sysex_preset(h9_sysex_preset *sxpreset) {
    bool is_valid = true;
    if (sxpreset->module_sysex_id < 1 || sxpreset->module_sysex_id > H9_NUM_MODULES) {
        is_valid = false;
        return is_valid;
    }
    if (sxpreset->algorithm < 0 || sxpreset->algorithm >= h9_modules[sxpreset->module_sysex_id - 1].num_algorithms) {
        is_valid = false;
        return is_valid;
    }
    if (sxpreset->algorithm != sxpreset->algorithm_repeat) {
        is_valid = false;
        return is_valid;
    }
    return is_valid;
}

static void import_preset(h9_preset *preset, h9_sysex_preset *sxpreset) {
    // Transform values to h9 state
    size_t module_index = sxpreset->module_sysex_id - 1;  // modules are 1-based, algorithms are 0-. Why? No clue.
    strncpy(preset->name, sxpreset->patch_name, H9_MAX_NAME_LEN);
    preset->module    = &h9_modules[module_index];
    preset->algorithm = &h9_modules[module_index].algorithms[sxpreset->algorithm];
    import_control_values(preset, sxpreset->control_values);
    import_knob_map(preset, sxpreset->knob_map);
    import_mknob_values(preset, sxpreset->mknob_values);
    preset->tempo               = (float)sxpreset->options[1] / 100.0;
    preset->tempo_enabled       = (sxpreset->options[2] != 0);
    preset->xyz_map[0]          = sxpreset->options[4];
    preset->xyz_map[1]          = sxpreset->options[5];
    preset->xyz_map[2]          = sxpreset->options[6];
    preset->modfactor_fast_slow = (sxpreset->options[7] != 0);

    // Output gain is funky, it's premultiplied by 10 and is a standard 2s complement signed int, but with a 24-bit
    // width.
    preset->output_gain = ((int32_t)(sxpreset->options[3] << 8) >> 8) * 0.1f;
}

static void export_preset(h9_sysex_preset *sxpreset, h9_preset *preset) {
    sxpreset->module_sysex_id  = preset->module->sysex_id;
    sxpreset->algorithm        = preset->algorithm->id;
    sxpreset->algorithm_repeat = preset->algorithm->id;
    sxpreset->preset_num       = DEFAULT_PRESET_NUM;

    // Dump knob values
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        export_knob_values(sxpreset->control_values, i, preset->knobs);
        export_knob_map(sxpreset->knob_map, i, preset->knobs);
        export_knob_mknob(sxpreset->mknob_values, i, preset->knobs);
    }

    // Dump expression pedal value
    sxpreset->control_values[10] = export_knob_value(preset->expression);
    sxpreset->mknob_values[10]   = export_mknob_value(preset->expression);
    sxpreset->mknob_values[11]   = export_mknob_value(KNOB_MAX);  // Always seems to be constant.

    // Dump translated option values
    sxpreset->options[1] = (uint16_t)rintf(preset->tempo * 100.0f);
    sxpreset->options[2] = (preset->tempo_enabled ? 1 : 0);
    sxpreset->options[3] = (int16_t)rintf(preset->output_gain * 10.0f);
    sxpreset->options[4] = preset->xyz_map[0];
    sxpreset->options[5] = preset->xyz_map[1];
    sxpreset->options[6] = preset->xyz_map[2];
    sxpreset->options[7] = (preset->modfactor_fast_slow ? 1 : 0);

    // Preset name
    strncpy(sxpreset->patch_name, preset->name, H9_MAX_NAME_LEN);

    // Finally, update the checksum
    sxpreset->checksum = checksum(sxpreset);
}

static size_t format_sysex(uint8_t *sysex, size_t max_len, h9_sysex_preset *sxpreset, uint8_t sysex_id) {
    // The mknob floats are formatted to ASCII. Eventually we might want to individually adjust
    // the width and precision, as the pedal seems to do.
    uint8_t mknob_widths[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    size_t  bytes_written  = snprintf((char *)sysex,
                                    max_len,
                                    "\xf0%c%c%c%c"
                                    "[%d] %d 5 %d\r\n"
                                    " %d %x %x %x %x %x %x %x %x %x %x %x\r\n"
                                    " %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\r\n"
                                    " %x %x %x %x %x %x %x %x\r\n"
                                    " %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f %.*f\r\n"
                                    "C_%x\r\n"
                                    "%s\r\n",
                                    H9_SYSEX_EVENTIDE,
                                    H9_SYSEX_H9,
                                    sysex_id,
                                    kH9_PROGRAM,  // Preamble
                                    sxpreset->preset_num,
                                    sxpreset->algorithm,
                                    sxpreset->module_sysex_id,  // Line 1, etc..
                                    sxpreset->algorithm,        // Again, not sure why
                                    sxpreset->control_values[0],
                                    sxpreset->control_values[1],
                                    sxpreset->control_values[2],
                                    sxpreset->control_values[3],
                                    sxpreset->control_values[4],
                                    sxpreset->control_values[5],
                                    sxpreset->control_values[6],
                                    sxpreset->control_values[7],
                                    sxpreset->control_values[8],
                                    sxpreset->control_values[9],
                                    sxpreset->control_values[10],
                                    sxpreset->knob_map[0],
                                    sxpreset->knob_map[1],
                                    sxpreset->knob_map[2],
                                    sxpreset->knob_map[3],
                                    sxpreset->knob_map[4],
                                    sxpreset->knob_map[5],
                                    sxpreset->knob_map[6],
                                    sxpreset->knob_map[7],
                                    sxpreset->knob_map[8],
                                    sxpreset->knob_map[9],
                                    sxpreset->knob_map[10],
                                    sxpreset->knob_map[11],
                                    sxpreset->knob_map[12],
                                    sxpreset->knob_map[13],
                                    sxpreset->knob_map[14],
                                    sxpreset->knob_map[15],
                                    sxpreset->knob_map[16],
                                    sxpreset->knob_map[17],
                                    sxpreset->knob_map[18],
                                    sxpreset->knob_map[19],
                                    sxpreset->knob_map[20],
                                    sxpreset->knob_map[21],
                                    sxpreset->knob_map[22],
                                    sxpreset->knob_map[23],
                                    sxpreset->knob_map[24],
                                    sxpreset->knob_map[25],
                                    sxpreset->knob_map[26],
                                    sxpreset->knob_map[27],
                                    sxpreset->knob_map[28],
                                    sxpreset->knob_map[29],
                                    sxpreset->options[0],
                                    sxpreset->options[1],
                                    sxpreset->options[2],
                                    sxpreset->options[3],
                                    sxpreset->options[4],
                                    sxpreset->options[5],
                                    sxpreset->options[6],
                                    sxpreset->options[7],
                                    mknob_widths[0],
                                    sxpreset->mknob_values[0],
                                    mknob_widths[1],
                                    sxpreset->mknob_values[1],
                                    mknob_widths[2],
                                    sxpreset->mknob_values[2],
                                    mknob_widths[3],
                                    sxpreset->mknob_values[3],
                                    mknob_widths[4],
                                    sxpreset->mknob_values[4],
                                    mknob_widths[5],
                                    sxpreset->mknob_values[5],
                                    mknob_widths[6],
                                    sxpreset->mknob_values[6],
                                    mknob_widths[7],
                                    sxpreset->mknob_values[7],
                                    mknob_widths[8],
                                    sxpreset->mknob_values[8],
                                    mknob_widths[9],
                                    sxpreset->mknob_values[9],
                                    mknob_widths[10],
                                    sxpreset->mknob_values[10],
                                    mknob_widths[11],
                                    sxpreset->mknob_values[11],
                                    sxpreset->checksum,
                                    sxpreset->patch_name);
    bytes_written += 1;  // Count the null byte
    if (max_len > (bytes_written + 1)) {
        sysex[bytes_written] = 0xF7;
        bytes_written += 1;
    }
    return bytes_written;
};

static h9_status load_preset(h9 *h9, uint8_t *cursor, size_t len) {
    // Need to unpack before we can validate the checksum
    h9_sysex_preset sxpreset;
    memset(&sxpreset, 0x0, sizeof(sxpreset));
    if (!unpack_preset(cursor, len, &sxpreset)) {
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

    import_preset(h9->preset, &sxpreset);

    // Sync control state, trigger display callbacks
    h9_reset_display_values(h9);

    h9->preset->dirty  = false;
    h9->preset->loaded = true;
    return kH9_OK;
}

static h9_status parse_system_value_dump(h9 *h9, uint8_t *data, size_t len) {
    h9_system_value_dump values;
    memset(&values, 0x0, sizeof(values));
    size_t num_lines = 5;
    char * lines[num_lines];
    size_t lengths[num_lines];
    size_t found_lines = find_lines((char *)data, len, lines, lengths, num_lines);

    if (found_lines != num_lines) {
        return kH9_SYSEX_INVALID;
    }

    // Line 0 should be [SYSTEM] V VV V.V.V[V] etc... but we don't really care about this line, other than to validate it
    char system_start[] = "[SYSTEM] ";
    if (strncmp(lines[0], system_start, sizeof(system_start) - 1) != 0) {
        return kH9_SYSEX_INVALID;
    }

    // Line 1 should be byte values
    size_t expected_values = sizeof(values.byte_values) / sizeof(*values.byte_values);
    size_t found           = scanhex_byte((char *)lines[1], lengths[1], values.byte_values, expected_values);
    if (found != expected_values) {
        return kH9_SYSEX_INVALID;
    }

    // Line 2 should be word values
    expected_values = sizeof(values.word_values) / sizeof(*values.word_values);
    found           = scanhex_word((char *)lines[2], lengths[2], values.word_values, expected_values);
    if (found != expected_values) {
        return kH9_SYSEX_INVALID;
    }

    // Line 3 should be bit values
    expected_values = sizeof(values.bit_values) / sizeof(*values.bit_values);
    found           = scanhex_bool((char *)lines[3], lengths[3], values.bit_values, expected_values);
    if (found != expected_values) {
        return kH9_SYSEX_INVALID;
    }

    // Line 4 should have the checksum
    uint32_t checksum;
    found = sscanf(lines[4], "C_%x", &checksum);
    if (found != 1) {
        debug_info("Did not identify checksum.\n");
        return kH9_SYSEX_INVALID;
    }

    // Validate the checksum
    uint16_t computed_checksum = 0;
    computed_checksum += array_sum16(values.word_values, sizeof(values.word_values) / sizeof(*values.word_values));
    computed_checksum += array_sum8(values.byte_values, sizeof(values.byte_values) / sizeof(*values.byte_values));
    computed_checksum += array_sum1(values.bit_values, sizeof(values.bit_values) / sizeof(*values.bit_values));

    if (computed_checksum != (uint16_t)checksum) {
        return kH9_SYSEX_CHECKSUM_INVALID;
    }

    /*
      Value map:
      Bit values: [index]
        [2]  = bypass
        [3]  = killdry
        [7]  = tempo (global)
        [10] = MIDI tempo sync
        [11] = tx CC
        [12] = tx PC
        [16] = global TEMPO
      Byte values: [index]
        [3]  = MIDI RX Channel
        [4]  = Sysex ID
        [8]  = MIDI TX Channel
        [16] = PSW map to CC
        [18] = KNOB0 map to CC
        ...
        [27] = KNOB9 map to CC
        [32] = EXPR map to CC
        [69] = Knob Mode [0 = normal, 1 = catchup, 2 = "locked"
      Word values: [index]
        [48] + [49] : Bluetooth PIN (see notes in system variables)
        [50] -> [57] : Pedal name (also encoded)
     */
    h9->bypass                          = values.bit_values[2];
    h9->killdry                         = values.bit_values[3];
    h9->global_tempo                    = values.bit_values[7];
    h9->midi_config.midi_rx_channel     = values.byte_values[3];
    h9->midi_config.midi_tx_channel     = values.byte_values[8];
    h9->midi_config.sysex_id            = values.byte_values[4];
    h9->midi_config.transmit_cc_enabled = values.byte_values[11];
    h9->midi_config.transmit_pc_enabled = values.byte_values[12];
    h9->midi_config.midi_clock_sync     = values.byte_values[10];
    size_t control_indices[]            = {27, 26, 25, 24, 23, 22, 18, 19, 20, 21, 32, 16};
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        uint8_t cc = values.byte_values[control_indices[i]];
        if (cc < 5) {
            h9->midi_config.cc_rx_map[i] = CC_DISABLED;
            h9->midi_config.cc_tx_map[i] = CC_DISABLED;
        } else {
            h9->midi_config.cc_rx_map[i] = cc - 5;
            h9->midi_config.cc_tx_map[i] = cc - 5;
        }
    }
    strncpy(h9->name, (char *)&values.word_values[50], H9_MAX_NAME_LEN - 1);
    strncpy(h9->bluetooth_pin, (char *)&values.word_values[48], 4);

    return kH9_OK;
}

static void internalize_cc(h9 *h9, control_id id, uint32_t cc) {
    if (cc < 5) {
        h9->midi_config.cc_rx_map[id] = CC_DISABLED;
        h9->midi_config.cc_tx_map[id] = CC_DISABLED;
    }
    h9->midi_config.cc_rx_map[id] = cc - 5;
    h9->midi_config.cc_tx_map[id] = cc - 5;
}

static h9_status parse_system_value(h9 *h9, uint8_t *cursor, size_t len) {
    uint32_t key         = 0;
    uint32_t value       = 0;
    char *   value_chars = (char *)&value + 2;  // last two chars of 32-bit word

    if (sscanf("%d %d", (char *)cursor, &key, &value) == 2) {
        switch (key) {
            case sp_bypass:
                h9->bypass = value;
                break;
            case sp_tempo:
                h9->preset->tempo = (float)value / 100.0f;
                break;
            case sp_global_tempo:
                h9->global_tempo = value;
                break;
            case sp_kill_dry:
                h9->killdry = value;
                break;
            case sp_kill_dry_global:
                h9->killdry = value;
                break;
            case sp_kb1_src:
                internalize_cc(h9, 0, value);
                break;
            case sp_kb2_src:
                internalize_cc(h9, 1, value);
                break;
            case sp_kb3_src:
                internalize_cc(h9, 2, value);
                break;
            case sp_kb4_src:
                internalize_cc(h9, 3, value);
                break;
            case sp_kb5_src:
                internalize_cc(h9, 4, value);
                break;
            case sp_kb6_src:
                internalize_cc(h9, 5, value);
                break;
            case sp_kb7_src:
                internalize_cc(h9, 6, value);
                break;
            case sp_kb8_src:
                internalize_cc(h9, 7, value);
                break;
            case sp_kb9_src:
                internalize_cc(h9, 8, value);
                break;
            case sp_kb10_src:
                internalize_cc(h9, 9, value);
                break;
            case sp_pdl_src:
                internalize_cc(h9, 10, value);
                break;
            case sp_psw_src:
                internalize_cc(h9, 11, value);
                break;
            case sp_tx_midi_cc:
                h9->midi_config.transmit_cc_enabled = value;
                break;
            case sp_midi_rx_channel:
                h9->midi_config.midi_rx_channel = value;
                break;
            case sp_midi_tx_channel:
                h9->midi_config.midi_tx_channel = value;
                break;
            case sp_tx_midi_pchg:
                h9->midi_config.transmit_pc_enabled = value;
                break;
            case sp_sysex_id:
                h9->midi_config.sysex_id = value;
                break;
            case sp_knob_mode:
                h9->knob_mode = value;
                break;
            // Name handling is a bit funky
            case name01:
                strncpy(h9->name, value_chars, 2);
                break;
            case name23:
                strncpy(h9->name + 0x2, value_chars, 2);
                break;
            case name45:
                strncpy(h9->name + 0x4, value_chars, 2);
                break;
            case name67:
                strncpy(h9->name + 0x6, value_chars, 2);
                break;
            case name89:
                strncpy(h9->name + 0x8, value_chars, 2);
                break;
            case nameAB:
                strncpy(h9->name + 0xA, value_chars, 2);
                break;
            case nameCD:
                strncpy(h9->name + 0xC, value_chars, 2);
                break;
            case nameEF:
                strncpy(h9->name + 0xE, value_chars, 2);
                break;
            case bluetoothPIN01:
                strncpy(h9->bluetooth_pin, value_chars, 2);
                break;
            case bluetoothPIN23:
                strncpy(h9->bluetooth_pin + 2, value_chars, 2);
                break;
            default:
                return kH9_UNKNOWN;
        }
    } else {
        return kH9_SYSEX_INVALID;
    }
    return kH9_OK;
}

static h9_status parse_sysex_header(h9 *h9, uint8_t *sysex, size_t len, h9_sysex_blob *payload) {
    assert(h9);
    uint8_t *cursor = sysex;  // start the cursor at the beginning
    if (*cursor == 0xF0) {    // Skip the leading F0 if present
        cursor++;
    }

    // Validate that it's an H9 piece of sysex
    uint8_t preamble[] = {H9_SYSEX_EVENTIDE, H9_SYSEX_H9};
    for (size_t i = 0; i < 2; i++) {
        if (*cursor++ != preamble[i]) {
            return kH9_SYSEX_PREAMBLE_INCORRECT;
        }
    }

    payload->dest_id = *cursor++;
    payload->type    = *cursor++;
    payload->len     = len - (size_t)((uintptr_t)cursor - (uintptr_t)sysex);
    payload->data    = cursor;

    return kH9_OK;
}

h9_status h9_parse_sysex(h9 *h9, uint8_t *sysex, size_t len, h9_enforce_sysex_id enforce_sysex_id) {
    assert(h9);
    h9_sysex_blob payload;
    h9_status     result = parse_sysex_header(h9, sysex, len, &payload);
    if (result != kH9_OK) {
        return result;
    }

    switch (payload.type) {
        case kH9_PROGRAM:
            return load_preset(h9, payload.data, payload.len);
        case kH9_TJ_SYSVARS_DUMP:
            return parse_system_value_dump(h9, payload.data, payload.len);
        case kH9_SYSEX_VALUE_DUMP:
            return parse_system_value(h9, payload.data, payload.len);
        default:
            return kH9_UNSUPPORTED_COMMAND;
    }
}

size_t h9_dump(h9 *h9, uint8_t *sysex, size_t max_len, bool update_dirty_flag) {
    assert(h9->preset && h9->preset->module && h9->preset->algorithm);

    h9_sysex_preset sxpreset;
    memset(&sxpreset, 0x0, sizeof(sxpreset));
    export_preset(&sxpreset, h9->preset);

    size_t bytes_written = format_sysex(sysex, max_len, &sxpreset, h9->midi_config.sysex_id);
    if (bytes_written <= max_len) {
        if (update_dirty_flag) {
            h9->preset->dirty = false;
        }
        h9->preset->loaded = true;
    }

    return bytes_written;
}

// Requests and Writes = sysexGen* names generate the sysex but do not send via the callback, other names only send.
size_t h9_sysexGenRequestCurrentPreset(h9 *h9, uint8_t *sysex, size_t max_len) {
    size_t bytes_written = snprintf((char *)sysex, max_len, "\xf0%c%c%c%c\xf7", H9_SYSEX_EVENTIDE, H9_SYSEX_H9, h9->midi_config.sysex_id, kH9_DUMP_ONE);
    return bytes_written;  // No +1 here, the f7 is the terminator.
}

size_t h9_sysexGenRequestSystemConfig(h9 *h9, uint8_t *sysex, size_t max_len) {
    size_t bytes_written = snprintf((char *)sysex, max_len, "\xf0%c%c%c%c\xf7", H9_SYSEX_EVENTIDE, H9_SYSEX_H9, h9->midi_config.sysex_id, kH9_TJ_SYSVARS_WANT);
    return bytes_written;  // No +1 here, the f7 is the terminator.
}

size_t h9_sysexGenRequestConfigVar(h9 *h9, uint16_t key, uint8_t *sysex, size_t max_len) {
    size_t bytes_written = snprintf((char *)sysex, max_len, "\xf0%c%c%c%c%x%c\xf7", H9_SYSEX_EVENTIDE, H9_SYSEX_H9, h9->midi_config.sysex_id, kH9_VALUE_WANT, key, 0x0);
    return bytes_written;  // No +1 here, the f7 is the terminator.
}

size_t h9_sysexGenWriteConfigVar(h9 *h9, uint16_t key, uint16_t value, uint8_t *sysex, size_t max_len) {
    size_t bytes_written =
        snprintf((char *)sysex, max_len, "\xf0%c%c%c%c%x %x%c\xf7", H9_SYSEX_EVENTIDE, H9_SYSEX_H9, h9->midi_config.sysex_id, kH9_USER_VALUE_PUT, key, value, 0x0);
    return bytes_written;  // No +1 here, the f7 is the terminator.
}

void h9_sysexRequestCurrentPreset(h9 *h9) {
    size_t  len = 15;
    uint8_t sysex[len];
    size_t  bytes_written = h9_sysexGenRequestCurrentPreset(h9, sysex, len);
    if (bytes_written <= len && h9->sysex_callback != NULL) {
        h9->sysex_callback(h9->callback_context, sysex, bytes_written);
    }
}

void h9_sysexRequestSystemConfig(h9 *h9) {
    size_t  len = 15;
    uint8_t sysex[len];
    size_t  bytes_written = h9_sysexGenRequestSystemConfig(h9, sysex, len);
    if (bytes_written <= len && h9->sysex_callback != NULL) {
        h9->sysex_callback(h9->callback_context, sysex, bytes_written);
    }
}

void h9_sysexRequestConfigVar(h9 *h9, uint16_t key) {
    size_t  len = 15;
    uint8_t sysex[len];
    size_t  bytes_written = h9_sysexGenRequestConfigVar(h9, key, sysex, len);
    if (bytes_written <= len && h9->sysex_callback != NULL) {
        h9->sysex_callback(h9->callback_context, sysex, bytes_written);
    }
}

void h9_sysexWriteConfigVar(h9 *h9, uint16_t key, uint16_t value) {
    size_t  len = 15;
    uint8_t sysex[len];
    size_t  bytes_written = h9_sysexGenWriteConfigVar(h9, key, value, sysex, len);
    if (bytes_written <= len && h9->sysex_callback != NULL) {
        h9->sysex_callback(h9->callback_context, sysex, bytes_written);
    }
}

bool h9_isSystemConfig(h9 *h9, uint8_t *sysex, size_t len) {
    assert(h9);
    h9_sysex_blob payload;
    if (parse_sysex_header(h9, sysex, len, &payload) != kH9_OK) {
        return false;
    }
    return (payload.type == kH9_TJ_SYSVARS_DUMP);
}
