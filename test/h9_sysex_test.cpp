/*  h9_sysex_test.cpp
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

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "libh9.h"
#include "test_helpers.hpp"
#include "utils.h"

#include "gtest/gtest.h"

#define TEST_CLASS H9SysexTest
#define KNOB_MAX   0x7fe0

// Various sysex constants for testing certain things
char sysex_hrmdlo[] =
    "\x1c\x70\x01\x4f"
    "[1] 8 5 5\r\n"
    " 8 3ff0 3ff0 3ff0 2c92 293c 3226 3458 b12 5656 0 0\r\n"
    " 0 0 0 0 0 0 0 0 0 0 0 0 3459 2c38 0 0 5657 6fcf 7088 6264 23cf 0 0 0 0 0 0 0 0 0\r\n"
    " 0 c42 0 14 9 8 4 0\r\n"
    " 65000 65000 65000 65000 65000 65000 65000 65000 65000 65000 65000 65000\r\n"
    "C_ee49\r\n"
    "HRMDLO\r\n";

namespace h9_test {
// The fixture for testing class Foo.
class TEST_CLASS : public ::testing::Test {
 protected:
    // You can remove any or all of the following functions if their bodies would
    // be empty.

    TEST_CLASS() {
        // You can do set-up work for each test here.
    }

    ~TEST_CLASS() override {
        // You can do clean-up work that doesn't throw exceptions here.
    }

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:

    void SetUp() override {
        h9obj = h9_new();
        init_callback_helpers();
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }

    void LoadPatch(h9 *h9obj, char *sysex) {
        h9_status status = h9_parse_sysex(h9obj, reinterpret_cast<uint8_t *>(sysex), strnlen(sysex, 1000), kH9_RESTRICT_TO_SYSEX_ID);
        ASSERT_EQ(status, kH9_OK);
    }

    // Class members declared here can be used by all tests in the test suite
    h9 *h9obj;
};

// Tests that h9_load correctly parses valid sysex
TEST_F(TEST_CLASS, h9_load_withCorrectSysex_returns_kH9_OK) {
    LoadPatch(h9obj, sysex_hrmdlo);
    ASSERT_EQ(h9obj->preset->algorithm->id, 8);
    ASSERT_EQ(h9obj->preset->module->sysex_id, 5);
}

// Test that the h9_load correctly dumps the same sysex it parsed
TEST_F(TEST_CLASS, h9_dump_dumps_loaded_sysex) {
    LoadPatch(h9obj, sysex_hrmdlo);

    size_t  buf_len = 1000U;
    uint8_t output[buf_len];
    size_t  bytes_written = 0U;
    bytes_written         = h9_dump(h9obj, output, buf_len, true);

    // Check, byte for byte, that the output is identical.
    // Note that dump wraps with 0xF0/0xF7, so we need to add one and subtract 1.
    EXPECT_EQ(sizeof(sysex_hrmdlo), bytes_written - 2);
    for (size_t i = 1; i < (bytes_written - 1); i++) {
        if (sysex_hrmdlo[i - 1] != output[i]) {
            printf("%s\n", sysex_hrmdlo);
            printf("%s\n", output + 1);
            char hexdump1[1000];
            char hexdump2[1000];
            hexdump(hexdump1, 1000, (uint8_t *)sysex_hrmdlo, sizeof(sysex_hrmdlo));
            hexdump(hexdump2, 1000, output + 1, bytes_written - 2);
            printf("%s\n", hexdump1);
            printf("%s\n", hexdump2);
            printf("Failed at position %zu.\n", i);
        }
        ASSERT_EQ(sysex_hrmdlo[i - 1], output[i]);
    }
}

TEST_F(TEST_CLASS, h9_dump_uses_sysex_id) {
    uint8_t expected_sysex_id = 0x0F;
    LoadPatch(h9obj, sysex_hrmdlo);
    h9obj->midi_config.sysex_id = expected_sysex_id;  // 16, the max
    size_t  buf_len             = 1000U;
    uint8_t output[buf_len];
    size_t  bytes_written = 0U;
    bytes_written         = h9_dump(h9obj, output, buf_len, true);
    ASSERT_EQ(output[3], expected_sysex_id);
}

TEST_F(TEST_CLASS, h9_dump_writes_expr) {
    float expected_expr = 0.9f;
    char  expected_str[5];
    sprintf(expected_str, "%4x", static_cast<uint16_t>(rintf(expected_expr * KNOB_MAX)));

    LoadPatch(h9obj, sysex_hrmdlo);
    h9_setControl(h9obj, EXPR, expected_expr, kH9_TRIGGER_CALLBACK);
    size_t  buf_len = 1000U;
    uint8_t output[buf_len];
    size_t  bytes_written = 0U;
    bytes_written         = h9_dump(h9obj, output, buf_len, true);
    size_t position       = 65;  // TODO: make this less brittle by re-parsing

    char found_string[5];
    strncpy(found_string, reinterpret_cast<char *>(&output[position]), 4);
    EXPECT_STREQ(found_string, expected_str);
}

TEST_F(TEST_CLASS, h9_load_triggers_display_callback) {
    h9obj->display_callback = display_callback;
    // Ensure that it's cleared before we run
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        EXPECT_FALSE(display_callback_triggered(control_id(i), NULL));
    }

    // Load the patch and assert the display callback fired for each control
    LoadPatch(h9obj, sysex_hrmdlo);
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        EXPECT_TRUE(display_callback_triggered(control_id(i), nullptr));
    }
}

TEST_F(TEST_CLASS, h9_load_doesNOT_trigger_cc_callback) {
    h9obj->cc_callback = cc_callback;
    h9_midi_config midi_config;
    h9_copyMidiConfig(h9obj, &midi_config);
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        midi_config.cc_rx_map[i] = i;
    }
    h9_setMidiConfig(h9obj, &midi_config);

    // Ensure that it's cleared before we run
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        uint8_t cc = h9obj->midi_config.cc_rx_map[i];
        EXPECT_FALSE(cc_callback_triggered(cc, nullptr));
    }
    // Load the patch and assert that we didn't update any of them
    LoadPatch(h9obj, sysex_hrmdlo);
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        uint8_t cc = h9obj->midi_config.cc_rx_map[i];
        EXPECT_FALSE(cc_callback_triggered(cc, nullptr));
    }
}

TEST_F(TEST_CLASS, h9_dump_works_without_previously_loading) {
    size_t  buf_len = 1000U;
    uint8_t output[buf_len];
    size_t  bytes_written = 0U;
    bytes_written         = h9_dump(h9obj, output, buf_len, true);
    EXPECT_GT(bytes_written, 200);
}

TEST_F(TEST_CLASS, h9_load_flags_preset_clean) {
    h9obj->preset->dirty = true;  // force it
    EXPECT_TRUE(h9_dirty(h9obj));
    LoadPatch(h9obj, sysex_hrmdlo);
    ASSERT_FALSE(h9_dirty(h9obj));
}

TEST_F(TEST_CLASS, h9_load_parses_system_variable_dump) {
    FILE *sysvar_dump_file;
    sysvar_dump_file = fopen("../test_data/Device_Config3.syx", "r");
    ASSERT_NE(sysvar_dump_file, nullptr);
    uint8_t buffer[1000];
    size_t  buffer_len;
    buffer_len = fread(buffer, 1000, 1, sysvar_dump_file);

    // Fill h9obj with dummy values so we can be sure they were loaded from the file
    h9obj->bypass                          = true;
    h9obj->global_tempo                    = false;
    h9obj->killdry                         = true;
    h9obj->knob_mode                       = kKnobModeLocked;
    h9obj->midi_config.midi_rx_channel     = 9;
    h9obj->midi_config.midi_tx_channel     = 9;
    h9obj->midi_config.midi_clock_sync     = false;
    h9obj->midi_config.sysex_id            = 7;
    h9obj->midi_config.transmit_cc_enabled = false;
    h9obj->midi_config.transmit_pc_enabled = false;
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        h9obj->midi_config.cc_rx_map[i] = 0;
        h9obj->midi_config.cc_tx_map[i] = 0;
    }

    // Load the sysex
    ASSERT_EQ(h9_parse_sysex(h9obj, buffer, buffer_len, kH9_RESPOND_TO_ANY_SYSEX_ID), kH9_OK);

    // See what happens
    EXPECT_EQ(h9obj->global_tempo, true);
    EXPECT_EQ(h9obj->killdry, false);
    EXPECT_EQ(h9obj->bypass, false);
    EXPECT_EQ(h9obj->midi_config.sysex_id, 0);
    EXPECT_EQ(h9obj->midi_config.midi_rx_channel, 2);   // 0 = off, 1 = OMNI, 2 = MIDI Channel 1... 16
    EXPECT_EQ(h9obj->midi_config.midi_tx_channel, 10);  // MIDI CH 11 on the device
    EXPECT_EQ(h9obj->midi_config.transmit_cc_enabled, true);
    EXPECT_EQ(h9obj->midi_config.transmit_pc_enabled, true);
    EXPECT_EQ(h9obj->midi_config.midi_clock_sync, true);
    uint8_t rx_map[] = {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 11, 71};
    uint8_t tx_map[] = {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 11, 71};
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        EXPECT_EQ(h9obj->midi_config.cc_rx_map[i], rx_map[i]);  // +5 becuase 0 = disabled, 1-5 are switches...
        EXPECT_EQ(h9obj->midi_config.cc_tx_map[i], tx_map[i]);
    }
    EXPECT_EQ(h9obj->midi_config.cc_rx_map[EXPR], rx_map[EXPR]);
    EXPECT_EQ(h9obj->midi_config.cc_tx_map[PSW], tx_map[PSW]);
    EXPECT_STREQ(h9obj->name, "DC-H9Std");
    EXPECT_STREQ(h9obj->bluetooth_pin, "1723");
}

/*
Tests to do:
 - Loading a preset from sysex sets loaded and clears dirty
 - Dumping a preset sets loaded (to true)
 - Dumping a preset clears dirty if the flag is set, does not clear it if the flag is not set
 - Dumping a preset into a buffer that is too small does not set loaded
 - Dumping a preset into a buffer that is too small does not clear the dirty flag if it is set
 - Test that knob values are populated from preset properly
 - Examine pedal behaviour with expression pedal across preset load and see what it does
    (does it reset the expression pedal position, does it "ignore" it until it updates? - and does the catch-up mode matter?)
    and then test that the behaviour matches.
 - Test generator methods for requesting data
 - When incoming sysex id is 0, restrict is ignored (correctly)
 - When incoming sysex id is not 0, and h9 sysex id is 0, restrict is ignored (correctly)
 - When incoming sysex id is not 0 and h9 sysex id is not 0, if restrict is set, the sysex is not processed
 - And when it is not set, the sysex is processed.
*/

}  // namespace h9_test
