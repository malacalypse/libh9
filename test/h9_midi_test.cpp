//
//  h9_midi_test.cpp
//  h9_gtest
//
//  Created by Studio DC on 2020-07-15.
//

#include <math.h>
#include <string.h>
#include "h9.h"
#include "test_helpers.hpp"
#include "utils.h"

#include "gtest/gtest.h"

#define TEST_CLASS H9MIDITest

namespace h9_test {

// Test Fixture
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
        h9obj->display_callback = display_callback;
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }

    // Class members declared here can be used by all tests in the test suite
    h9 *h9obj;
};

TEST_F(TEST_CLASS, h9_cc_withNonMappedCC_doesNothing) {
    uint8_t non_mapped_cc = 99;
    uint8_t some_value    = 42;
    h9_cc(h9obj, non_mapped_cc, some_value);
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        if (i < H9_NUM_KNOBS) {
            EXPECT_EQ(display_callback_triggered((control_id)i, NULL), false);
            EXPECT_EQ(h9_controlValue(h9obj, (control_id)i), 0.5f);
        } else {
            EXPECT_EQ(display_callback_triggered((control_id)i, NULL), false);
            EXPECT_EQ(h9_controlValue(h9obj, (control_id)i), 0.0f);
        }
    }
}

TEST_F(TEST_CLASS, h9_cc_withMappedCC_updatesControl) {
    control_id chosen_control = KNOB5;
    uint8_t    mapped_cc      = h9obj->midi_config.cc_tx_map[chosen_control];
    uint8_t    some_value     = 42;
    h9_cc(h9obj, mapped_cc, some_value);
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        if (i != chosen_control) {
            if (i < H9_NUM_KNOBS) {
                EXPECT_EQ(display_callback_triggered((control_id)i, NULL), false);
                EXPECT_EQ(h9_controlValue(h9obj, (control_id)i), 0.5f);
            } else {
                EXPECT_EQ(display_callback_triggered((control_id)i, NULL), false);
                EXPECT_EQ(h9_controlValue(h9obj, (control_id)i), 0.0f);
            }
        } else {
            control_value updated_value;
            EXPECT_EQ(display_callback_triggered((control_id)i, &updated_value), true);
            EXPECT_NEAR(updated_value, (double)some_value / 127.0, 0.001);
        }
    }
}

}  // namespace h9_test
