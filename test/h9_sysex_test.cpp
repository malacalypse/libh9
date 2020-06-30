//
//  h9_sysex_test.cpp
//  h9_gtest
//
//  Created by Studio DC on 2020-06-29.
//

#include "h9.h"

#include "gtest/gtest.h"

#define TEST_CLASS H9Test

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
     // Code here will be called immediately after the constructor (right
     // before each test).
  }

  void TearDown() override {
     // Code here will be called immediately after each test (right
     // before the destructor).
  }

  // Class members declared here can be used by all tests in the test suite
  // for Foo.
};

// Tests that h9_new does not leave the object with a NULL preset pointer
TEST_F(TEST_CLASS, h9_new_populates_empty_preset) {
    h9* h9obj = h9_new();
    ASSERT_NE(h9obj, nullptr);
    EXPECT_NE(h9obj->preset, nullptr);
}

// Tests that h9_load correctly parses valid sysex
TEST_F(TEST_CLASS, h9_load_withCorrectSysex_returns_kH9_OK) {
    h9* h9obj = h9_new();
    char sysex[] =  "\x1c\x70\x01\x4f"
                    "[1] 8 5 5\r\n"
                    " 8 3ff0 3ff0 3ff0 2c92 293c 3226 3458 b12 5656 0 0\r\n"
                    " 0 0 0 0 0 0 0 0 0 0 0 0 3459 2c38 0 0 5657 6fcf 7088 6264 23cf 0 0 0 0 0 0 0 0 0\r\n"
                    " 0 c42 0 14 9 8 4 0\r\n"
                    " 65000 65000 65000 65000 65000 65000 65000 65000 65000 65000 65000 65000\r\n"
                    "C_ee49\r\n"
                    "HRMDLO\r\n";
    h9_status status = h9_load(h9obj, reinterpret_cast<uint8_t *>(sysex), strnlen(sysex, 1000));
    ASSERT_EQ(status, kH9_OK);
}

}  // namespace h9_test
