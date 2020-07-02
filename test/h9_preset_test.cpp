//
//  h9_preset_test.cpp
//  h9_gtest
//
//  Created by Studio DC on 2020-07-01.
//

#include <math.h>
#include <string.h>
#include "h9.h"
#include "test_helpers.hpp"
#include "utils.h"

#include "gtest/gtest.h"

#define TEST_CLASS        H9PresetTest
#define EMPTY_PRESET_NAME "Empty"  // TODO: Find an elegant way to match with h9.c
#define DEFAULT_KNOB_CC   22       // Per the user guide
#define DEFAULT_EXPR_CC   15       // Per the user guide, but only for transmit

// Various sysex constants for testing certian things

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
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }

    // Class members declared here can be used by all tests in the test suite
    h9 *h9obj;
};

TEST_F(TEST_CLASS, h9_new_populates_default_preset) {
    ASSERT_NE(h9obj, nullptr);
    EXPECT_NE(h9obj->preset, nullptr);
    EXPECT_NE(h9obj->preset->module, nullptr);
    EXPECT_NE(h9obj->preset->algorithm, nullptr);
    char expected_name[] = EMPTY_PRESET_NAME;
    EXPECT_STREQ(h9obj->preset->name, expected_name);
}

TEST_F(TEST_CLASS, h9_setControl_flagsPresetAsDirty) {
    EXPECT_FALSE(h9_dirty(h9obj));
    h9_setControl(h9obj, KNOB1, 0.5f, kH9_SUPPRESS_CALLBACK);
    EXPECT_TRUE(h9_dirty(h9obj));
}

}  // namespace h9_test
