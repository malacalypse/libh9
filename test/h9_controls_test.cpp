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

#define TEST_CLASS      H9ControlTest
#define DEFAULT_KNOB_CC 22  // Per the user guide
#define DEFAULT_EXPR_CC 15  // Per the user guide, but only for transmit

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

TEST_F(TEST_CLASS, h9_new_has_default_midi_config) {
    h9_midi_config midi_config;
    h9_copyMidiConfig(h9obj, &midi_config);
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        EXPECT_EQ(midi_config.cc_rx_map[i], DEFAULT_KNOB_CC + i);
        EXPECT_EQ(midi_config.cc_tx_map[i], DEFAULT_KNOB_CC + i);
    }
    EXPECT_EQ(midi_config.cc_rx_map[EXPR], CC_DISABLED);
    EXPECT_EQ(midi_config.cc_tx_map[EXPR], DEFAULT_EXPR_CC);
    EXPECT_EQ(midi_config.cc_rx_map[PSW], CC_DISABLED);
    EXPECT_EQ(midi_config.cc_tx_map[PSW], CC_DISABLED);
}

TEST_F(TEST_CLASS, h9_setControl_updatesControlValue) {
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        float value          = float(1.0f / (float)i);
        float expected_value = value;
        if (control_id(i) == PSW) {
            expected_value = (value == 0.0f) ? 0.0f : 1.0f;
        }
        h9_setControl(h9obj, control_id(i), value, kH9_TRIGGER_CALLBACK);
        EXPECT_EQ(h9_controlValue(h9obj, control_id(i)), expected_value);
    }
}

TEST_F(TEST_CLASS, h9_setControl_updatesDisplayValue) {
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        float value          = float(1.0f / (float)i);
        float expected_value = value;
        if (control_id(i) == PSW) {
            expected_value = (value == 0.0f) ? 0.0f : 1.0f;
        }
        h9_setControl(h9obj, control_id(i), value, kH9_TRIGGER_CALLBACK);
        EXPECT_EQ(h9_displayValue(h9obj, control_id(i)), expected_value);
    }
}

TEST_F(TEST_CLASS, h9_setControl_withDisplayCallback_callsDisplayCallback) {
    h9obj->display_callback = display_callback;
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        control_value value          = control_value(i) / ((control_value)NUM_CONTROLS + 1.0);
        control_value expected_value = value;
        if (control_id(i) == PSW) {
            value          = (value <= 0.5f) ? 0.0f : 1.0f;
            expected_value = (value <= 0.0f) ? 0.0f : 1.0f;
        }
        h9_setControl(h9obj, control_id(i), value, kH9_TRIGGER_CALLBACK);
        control_value retrieved_value = -1.0f;
        EXPECT_TRUE(display_callback_triggered(control_id(i), &retrieved_value));
        EXPECT_NEAR(retrieved_value, expected_value, 0.00001);
    }
}

TEST_F(TEST_CLASS, h9_setControl_withCCCallback_whenTriggering_callsCCCallback) {
    h9obj->cc_callback = cc_callback;
    // Ensure every CC value is set
    h9_midi_config midi_config;
    h9_copyMidiConfig(h9obj, &midi_config);
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        midi_config.cc_rx_map[i] = i;
    }
    ASSERT_TRUE(h9_setMidiConfig(h9obj, &midi_config));

    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        control_value value             = control_value(rand()) / control_value(RAND_MAX);
        uint8_t       expected_cc_value = static_cast<uint8_t>(static_cast<uint16_t>(rintf(value * 16383)) >> 7);
        uint8_t       retrieved_value   = 128;
        h9_setControl(h9obj, control_id(i), value, kH9_TRIGGER_CALLBACK);
        uint8_t cc_value_to_check = h9obj->midi_config.cc_rx_map[i];
        EXPECT_TRUE(cc_callback_triggered(cc_value_to_check, &retrieved_value));
        EXPECT_EQ(retrieved_value, expected_cc_value);
    }
}

TEST_F(TEST_CLASS, h9_setControl_withCCCallback_whenSuppressing_suppressesCCCallback) {
    h9obj->cc_callback = cc_callback;
    // Set the MIDI CC rx values to valid values
    h9_midi_config midi_config;
    h9_copyMidiConfig(h9obj, &midi_config);
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        midi_config.cc_rx_map[i] = i;
    }
    ASSERT_TRUE(h9_setMidiConfig(h9obj, &midi_config));
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        control_value value             = control_value(rand()) / control_value(RAND_MAX);  // random between 0.0f and 1.0f
        uint8_t       retrieved_value   = 128;
        uint8_t       cc_value_to_check = h9obj->midi_config.cc_rx_map[i];
        h9_setControl(h9obj, control_id(i), value, kH9_SUPPRESS_CALLBACK);
        EXPECT_FALSE(cc_callback_triggered(cc_value_to_check, &retrieved_value));
    }
}

TEST_F(TEST_CLASS, h9_setControl_withCCCallback_withCCDisabled_whenTriggering_suppressesCCCallback) {
    h9obj->cc_callback = cc_callback;
    // Set the MIDI CC rx values to disabled
    h9_midi_config midi_config;
    h9_copyMidiConfig(h9obj, &midi_config);
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        midi_config.cc_rx_map[i] = CC_DISABLED;
    }
    ASSERT_TRUE(h9_setMidiConfig(h9obj, &midi_config));

    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        control_value value = control_value(rand()) / control_value(RAND_MAX);  // random between 0.0f and 1.0f
        h9_setControl(h9obj, control_id(i), value, kH9_TRIGGER_CALLBACK);
    }
    EXPECT_EQ(cc_callback_count(), 0);
}

TEST_F(TEST_CLASS, h9_setControl_withDisplayCallback_whenSuppressing_stillCallsDisplayCallback) {
    h9obj->display_callback = display_callback;
    h9obj->cc_callback      = cc_callback;
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        control_value value          = control_value(rand()) / control_value(RAND_MAX);  // random between 0.0f and 1.0f
        control_value expected_value = value;
        if (control_id(i) == PSW) {
            expected_value = (value == 0.0f) ? 0.0f : 1.0f;
        }
        h9_setControl(h9obj, control_id(i), value, kH9_SUPPRESS_CALLBACK);
        control_value retrieved_value = -1.0f;
        EXPECT_TRUE(display_callback_triggered(control_id(i), &retrieved_value));
        EXPECT_NEAR(retrieved_value, expected_value, 0.00001);
    }
}

/*
Tests to do:
 - Test that display values are synched when expression changes them, based on the preset map
 - Test that setting them after expression move snaps them back until the next expression move
*/

}  // namespace h9_test
