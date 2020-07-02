//
//  h9_sysex_test.cpp
//  h9_gtest
//
//  Created by Studio DC on 2020-06-29.
//

#include <math.h>
#include <string.h>
#include "h9.h"
#include "utils.h"

#include "gtest/gtest.h"

#define TEST_CLASS H9Test
#define KNOB_MAX   0x7fe0

// Various sysex constants for testing certian things
char sysex_hrmdlo[] =
    "\x1c\x70\x01\x4f"
    "[1] 8 5 5\r\n"
    " 8 3ff0 3ff0 3ff0 2c92 293c 3226 3458 b12 5656 0 0\r\n"
    " 0 0 0 0 0 0 0 0 0 0 0 0 3459 2c38 0 0 5657 6fcf 7088 6264 23cf 0 0 0 0 0 0 0 0 0\r\n"
    " 0 c42 0 14 9 8 4 0\r\n"
    " 65000 65000 65000 65000 65000 65000 65000 65000 65000 65000 65000 65000\r\n"
    "C_ee49\r\n"
    "HRMDLO\r\n";
float    display_callback_tracker[12];
int8_t   cc_callback_tracker[128];
uint8_t *sysex_callback_tracker;
size_t   sysex_callback_tracker_len;

void cc_callback(h9 *h9obj, uint8_t midi_channel, uint8_t cc_num, uint8_t msb, uint8_t lsb) {
    cc_callback_tracker[cc_num] = static_cast<int8_t>(msb);
}

void display_callback(h9 *h9obj, control_id control, control_value value) {
    display_callback_tracker[control] = value;
}

void sysex_callback(h9 *h9obj, uint8_t *sysex, size_t len) {
    sysex_callback_tracker     = sysex;
    sysex_callback_tracker_len = len;
}

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
        for (size_t i = 0; i < sizeof(display_callback_tracker) / sizeof(*display_callback_tracker); i++) {
            display_callback_tracker[i] = -1.0f;
        }
        for (size_t i = 0; i < sizeof(cc_callback_tracker) / sizeof(*cc_callback_tracker); i++) {
            cc_callback_tracker[i] = -1;
        }
        sysex_callback_tracker     = NULL;
        sysex_callback_tracker_len = 0;
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }

    void LoadPatch(h9 *h9obj, char *sysex) {
        h9_status status = h9_load(h9obj, reinterpret_cast<uint8_t *>(sysex), strnlen(sysex, 1000));
        ASSERT_EQ(status, kH9_OK);
    }

    // Class members declared here can be used by all tests in the test suite
    h9 *h9obj;
};

// Tests that h9_new does not leave the object with a NULL preset pointer
TEST_F(TEST_CLASS, h9_new_populates_empty_preset) {
    ASSERT_NE(h9obj, nullptr);
    EXPECT_NE(h9obj->preset, nullptr);
}

// Tests that h9_load correctly parses valid sysex
TEST_F(TEST_CLASS, h9_load_withCorrectSysex_returns_kH9_OK) {
    LoadPatch(h9obj, sysex_hrmdlo);
    ASSERT_EQ(h9obj->preset->algorithm->id, 8);
    ASSERT_EQ(h9obj->preset->module->sysex_num, 5);
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
    size_t position       = 63;  // TODO: make this less brittle by re-parsing

    char found_string[5];
    strncpy(found_string, reinterpret_cast<char *>(&output[position]), 4);
    EXPECT_STREQ(found_string, expected_str);
}

TEST_F(TEST_CLASS, h9_load_triggers_display_callback) {
    h9obj->display_callback = display_callback;
    // Ensure that it's cleared before we run
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        EXPECT_EQ(display_callback_tracker[i], -1);
    }

    // Load the patch and assert the display callback fired for each control
    LoadPatch(h9obj, sysex_hrmdlo);
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        EXPECT_NE(display_callback_tracker[i], -1);
    }
}

TEST_F(TEST_CLASS, h9_load_doesNOT_trigger_cc_callback) {
    h9obj->cc_callback = cc_callback;
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        h9obj->midi_config.cc_rx_map[i] = i;
    }
    // Ensure that it's cleared before we run
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        EXPECT_EQ(cc_callback_tracker[h9obj->midi_config.cc_rx_map[i]], -1);
    }
    // Load the patch and assert that we didn't update any of them
    LoadPatch(h9obj, sysex_hrmdlo);
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        EXPECT_EQ(cc_callback_tracker[h9obj->midi_config.cc_rx_map[i]], -1);
    }
}

TEST_F(TEST_CLASS, h9_dump_works_without_previously_loading) {
    size_t  buf_len = 1000U;
    uint8_t output[buf_len];
    size_t  bytes_written = 0U;
    bytes_written         = h9_dump(h9obj, output, buf_len, true);
    EXPECT_GT(bytes_written, 200);
}

/*
 Tests to do:
 - Test that knob values are populated from preset properly
 - Test that display values are synched
 - Examine pedal behaviour with expression pedal on load and see what it does (does it reset the expression pedal position, does it "ignore" it until it updates? - and does the
 catch-up mode matter?) and then test that the behaviour matches.
 - Validate that callbacks fire properly on load
 - Test dirty behaviour with sync
 */

}  // namespace h9_test
