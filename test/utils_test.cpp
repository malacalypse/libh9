//
//  utils_test.cpp
//  h9_gtest
//
//  Created by Studio DC on 2020-07-07.
//

#include "utils.h"
#include <math.h>
#include <string.h>
#include "h9.h"
#include "test_helpers.hpp"

#include "gtest/gtest.h"

#define TEST_CLASS UtilsTest

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

    void SetUp() override {}

    void TearDown() override {}

    // Class members declared here can be used by all tests in the test suite
};

TEST_F(TEST_CLASS, scanhex_word_scansCorrectly) {
    char     string[]         = "f ef def cdef  0    01  2fa deadbeef";
    uint16_t expected_words[] = {0xf, 0xef, 0xdef, 0xcdef, 0x0, 0x1, 0x2fa, 0xbeef};
    uint16_t scanned_words[12];  // extra
    size_t   found = scanhex_word(string, strlen(string), scanned_words, 12);
    EXPECT_EQ(found, 8);
    size_t scan_length = (found > 8) ? 8 : found;
    for (size_t i = 0; i < scan_length; i++) {
        EXPECT_EQ(scanned_words[i], expected_words[i]);
    }
}

TEST_F(TEST_CLASS, scanhex_word_withEndingSpace_scansCorrectly) {
    char     string[]         = "f ef def cdef   0   01  2fa deadbeef ";
    uint16_t expected_words[] = {0xf, 0xef, 0xdef, 0xcdef, 0x0, 0x1, 0x2fa, 0xbeef};
    uint16_t scanned_words[12];  // extra
    size_t   found = scanhex_word(string, strlen(string), scanned_words, 12);
    EXPECT_EQ(found, 8);
    size_t scan_length = (found > 8) ? 8 : found;
    for (size_t i = 0; i < scan_length; i++) {
        EXPECT_EQ(scanned_words[i], expected_words[i]);
    }
}

TEST_F(TEST_CLASS, scanhex_word_withStartingSpace_scansCorrectly) {
    char     string[]         = " f ef def cdef   0   01  2fa deadbeef";
    uint16_t expected_words[] = {0xf, 0xef, 0xdef, 0xcdef, 0x0, 0x1, 0x2fa, 0xbeef};
    uint16_t scanned_words[12];  // extra
    size_t   found = scanhex_word(string, strlen(string), scanned_words, 12);
    EXPECT_EQ(found, 8);
    size_t scan_length = (found > 8) ? 8 : found;
    for (size_t i = 0; i < scan_length; i++) {
        EXPECT_EQ(scanned_words[i], expected_words[i]);
    }
}

TEST_F(TEST_CLASS, scanhex_word_withStartingAndEndingSpace_scansCorrectly) {
    char     string[]         = " f ef def cdef   0   01  2fa deadbeef ";
    uint16_t expected_words[] = {0xf, 0xef, 0xdef, 0xcdef, 0x0, 0x1, 0x2fa, 0xbeef};
    uint16_t scanned_words[12];  // extra
    size_t   found = scanhex_word(string, strlen(string), scanned_words, 12);
    EXPECT_EQ(found, 8);
    size_t scan_length = (found > 8) ? 8 : found;
    for (size_t i = 0; i < scan_length; i++) {
        EXPECT_EQ(scanned_words[i], expected_words[i]);
    }
}

TEST_F(TEST_CLASS, scanhex_byte_scansCorrectly) {
    char    string[]         = " f ef de cd  0 01  2a deadbeef";
    uint8_t expected_words[] = {0xf, 0xef, 0xde, 0xcd, 0x0, 0x1, 0x2a, 0xef};
    uint8_t scanned_words[12];  // extra
    size_t  found = scanhex_byte(string, strlen(string), scanned_words, 12);
    EXPECT_EQ(found, 8);
    size_t scan_length = (found > 8) ? 8 : found;
    for (size_t i = 0; i < scan_length; i++) {
        EXPECT_EQ(scanned_words[i], expected_words[i]);
    }
}

TEST_F(TEST_CLASS, scanhex_bool_scansCorrectly) {
    char   string[]         = " 0 1 1  0  1  0    0 1";
    bool   expected_words[] = {0, 1, 1, 0, 1, 0, 0, 1};
    bool   scanned_words[12];  // extra
    size_t found = scanhex_bool(string, strlen(string), scanned_words, 12);
    EXPECT_EQ(found, 8);
    size_t scan_length = (found > 8) ? 8 : found;
    for (size_t i = 0; i < scan_length; i++) {
        EXPECT_EQ(scanned_words[i], expected_words[i]);
    }
}

TEST_F(TEST_CLASS, scanhex_bool32_scansCorrectly) {
    char     string[]       = " 0 1 1  0  1  0    0 1 0110 10 01 0110 0 1 1 0 011 01111 ";
    uint32_t expected_value = 0x6969666F;
    uint32_t scanned_value  = 0U;
    size_t   found          = scanhex_bool32(string, strlen(string), &scanned_value);
    EXPECT_EQ(found, 32);
    EXPECT_EQ(scanned_value, expected_value);
}

TEST_F(TEST_CLASS, find_lines_works) {
    size_t max_lines = 12;
    char   string[]  = "This is a large line\r\nThis\r Is\n Not\n\r Normal\r\nBut it's totally ok.\r\n";
    char*  lines[max_lines];
    size_t lengths[max_lines];
    size_t found_lines = find_lines(string, strlen(string), lines, lengths, max_lines);
    ASSERT_EQ(found_lines, 6);
    EXPECT_STREQ(lines[0], string);
    EXPECT_STREQ(lines[1], &string[22]);
    EXPECT_STREQ(lines[2], &string[27]);
    EXPECT_STREQ(lines[3], &string[31]);
    EXPECT_STREQ(lines[4], &string[37]);
    EXPECT_STREQ(lines[5], &string[46]);
    EXPECT_EQ(lengths[0], 20);
    EXPECT_EQ(lengths[1], 4);
    EXPECT_EQ(lengths[2], 3);
    EXPECT_EQ(lengths[3], 4);
    EXPECT_EQ(lengths[4], 7);
    EXPECT_EQ(lengths[5], 20);
}

TEST_F(TEST_CLASS, find_lines_whenNoTrailingNewline_works) {
    size_t max_lines = 12;
    char   string[]  = "S\r\nI\nX\rL\n\rN\r\nS";
    char*  lines[max_lines];
    size_t lengths[max_lines];
    size_t found_lines = find_lines(string, strlen(string), lines, lengths, max_lines);
    ASSERT_EQ(found_lines, 6);
    EXPECT_EQ(lengths[5], 1);
}

}  // namespace h9_test
