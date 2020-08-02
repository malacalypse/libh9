/*  h9_preset_test.cpp
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
#include <string.h>
#include "libh9.h"
#include "test_helpers.hpp"
#include "utils.h"

#include "gtest/gtest.h"

#define TEST_CLASS        H9PresetTest
#define EMPTY_PRESET_NAME "Empty"  // TODO: Find an elegant way to match with h9.c

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
