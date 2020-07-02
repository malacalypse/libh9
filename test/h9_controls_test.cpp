//
//  h9_controls_test.cpp
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

#define TEST_CLASS H9ControlTest

// Various sysex constants for testing certian things

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

    // Class members declared here can be used by all tests in the test suite
    h9 *h9obj;
};

// Tests that h9_new does not leave the object with a NULL preset pointer
TEST_F(TEST_CLASS, h9_new_populates_empty_preset) {
    ASSERT_NE(h9obj, nullptr);
    EXPECT_NE(h9obj->preset, nullptr);
}

}  // namespace h9_test
