/*  h9_controls_test.cpp
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

#define TEST_CLASS      H9ControlTest
#define DEFAULT_KNOB_CC 22  // Per the user guide
#define DEFAULT_EXPR_CC 15  // Per the user guide, but only for transmit

extern h9_module h9_modules[H9_NUM_MODULES];

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

TEST_F(TEST_CLASS, h9_setKnobMap_updatesKnobMaps) {
    control_value lowers[H9_NUM_KNOBS];
    control_value uppers[H9_NUM_KNOBS];
    control_value psws[H9_NUM_KNOBS];
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        lowers[i] = i / 30.0;
        uppers[i] = 1.0 - lowers[i];
        psws[i]   = 0.5 + lowers[i];
        h9_setKnobMap(h9obj, (control_id)i, lowers[i], uppers[i], psws[i]);
    }
    // read back the set values with h9_knobMap getter
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        control_value lower;
        control_value upper;
        control_value psw;
        h9_knobMap(h9obj, (control_id)i, &lower, &upper, &psw);
        EXPECT_EQ(lowers[i], lower);
        EXPECT_EQ(uppers[i], upper);
        EXPECT_EQ(psws[i], psw);
    }
}

TEST_F(TEST_CLASS, h9_knobMap_whenKnobMapped_setsMappingValues) {
    control_value lowers[H9_NUM_KNOBS];
    control_value uppers[H9_NUM_KNOBS];
    control_value psws[H9_NUM_KNOBS];
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        lowers[i] = i / 30.0;
        uppers[i] = 1.0 - lowers[i];
        psws[i]   = 0.5 + lowers[i];
        h9_setKnobMap(h9obj, (control_id)i, lowers[i], uppers[i], psws[i]);
    }
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        control_value lower;
        control_value upper;
        control_value psw;
        h9_knobMap(h9obj, (control_id)i, &lower, &upper, &psw);
        EXPECT_EQ(lower, lowers[i]);
        EXPECT_EQ(upper, uppers[i]);
        EXPECT_EQ(psw, psws[i]);
    }
}

TEST_F(TEST_CLASS, h9_knobMap_whenKnobInvalid_doesNotUpdateReturnParameters) {
    control_value lower = 0.12;
    control_value upper = 0.34;
    control_value psw   = 0.45;
    h9_knobMap(h9obj, EXPR, &lower, &upper, &psw);
    EXPECT_EQ(lower, 0.12);
    EXPECT_EQ(upper, 0.34);
    EXPECT_EQ(psw, 0.45);
}

TEST_F(TEST_CLASS, h9_knobExprMapped_whenMapped_returnsTrue) {
    h9_setKnobMap(h9obj, KNOB0, 0.12, 0.34, 0);
    EXPECT_TRUE(h9_knobExprMapped(h9obj, KNOB0));
}

TEST_F(TEST_CLASS, h9_knobExprMapped_whenNotMapped_returnsFalse) {
    h9_setKnobMap(h9obj, KNOB0, 0, 0, 0.45);
    EXPECT_FALSE(h9_knobExprMapped(h9obj, KNOB0));
}

TEST_F(TEST_CLASS, h9_knobExprMapped_whenKnobInvalid_returnsFalse) {
    EXPECT_FALSE(h9_knobExprMapped(h9obj, EXPR));
}

TEST_F(TEST_CLASS, h9_knobPswMapped_whenMapped_returnsTrue) {
    h9_setKnobMap(h9obj, KNOB0, 0, 0, 0.45);
    EXPECT_TRUE(h9_knobPswMapped(h9obj, KNOB0));
}

TEST_F(TEST_CLASS, h9_knobPswMapped_whenNotMapped_returnsFalse) {
    h9_setKnobMap(h9obj, KNOB0, 0.12, 0.34, 0);
    EXPECT_FALSE(h9_knobPswMapped(h9obj, KNOB0));
}

TEST_F(TEST_CLASS, h9_knobPswMapped_whenKnobInvalid_returnsFalse) {
    EXPECT_FALSE(h9_knobPswMapped(h9obj, EXPR));
}

// Technically, this test could be broken up into the part for the display callback
// and the explicit display fetch, but since they're so closely related, for now
// we test them simultaneously.
TEST_F(TEST_CLASS, h9_setControl_whenSettingExpression_withKnobMaps_updatesDisplayValuesAccordingly) {
    h9obj->display_callback = display_callback;
    h9_setControl(h9obj, EXPR, 0.1002345, kH9_SUPPRESS_CALLBACK);  // ensure that all values below trigger

    control_value lowers[H9_NUM_KNOBS];
    control_value uppers[H9_NUM_KNOBS];
    control_value psws[H9_NUM_KNOBS];
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        lowers[i] = i / 30.0;
        uppers[i] = 1.0 - lowers[i];
        psws[i]   = 0.5 + lowers[i];
        h9_setKnobMap(h9obj, (control_id)i, lowers[i], uppers[i], psws[i]);
    }

    size_t        n       = 5;
    control_value exprs[] = {0.0, 0.25, 0.5, 0.75, 1.0};

    for (size_t i = 0; i < n; i++) {
        h9_setControl(h9obj, EXPR, exprs[i], kH9_SUPPRESS_CALLBACK);
        for (size_t j = 0; j < H9_NUM_KNOBS; j++) {
            control_value expected = lowers[j] + exprs[i] * (uppers[j] - lowers[j]);
            control_value actual;
            EXPECT_TRUE(display_callback_triggered((control_id)j, &actual));
            EXPECT_EQ(expected, actual);
            EXPECT_EQ(expected, h9_displayValue(h9obj, (control_id)j));
        }
    }
}

TEST_F(TEST_CLASS, h9_setControl_whenSettingKnobWithExprMapAfterExprMove_overridesExpr) {
    // Map knobs to expression pedal
    control_value lowers[H9_NUM_KNOBS];
    control_value uppers[H9_NUM_KNOBS];
    control_value psws[H9_NUM_KNOBS];
    for (size_t i = 0; i < H9_NUM_KNOBS; i++) {
        lowers[i] = i / 30.0;
        uppers[i] = 1.0 - lowers[i];
        psws[i]   = 0.5 + lowers[i];
        h9_setKnobMap(h9obj, (control_id)i, lowers[i], uppers[i], psws[i]);
    }

    // Set expression pedal
    control_value expr_val = 1.0 / 3.0;
    h9_setControl(h9obj, EXPR, expr_val, kH9_SUPPRESS_CALLBACK);

    // Update each knob to an explicit value, in turn, and test display callback value and readback
    h9obj->display_callback = display_callback;
    for (size_t j = 0; j < H9_NUM_KNOBS; j++) {
        control_value expected = 0.2345 + j / 20.0;
        control_value actual;

        h9_setControl(h9obj, (control_id)j, expected, kH9_SUPPRESS_CALLBACK);
        EXPECT_TRUE(display_callback_triggered((control_id)j, &actual));
        EXPECT_EQ(expected, actual);
        EXPECT_EQ(expected, h9_displayValue(h9obj, (control_id)j));
    }
}

TEST_F(TEST_CLASS, h9_numAlgorithms_whenInvalidModule_returnsZero) {
    EXPECT_EQ(h9_numAlgorithms(h9obj, h9_numModules(h9obj)), 0);
}

TEST_F(TEST_CLASS, h9_setAlgorithm_withInvalidModule_returnsFalse) {
    size_t num_modules = h9_numModules(h9obj);
    EXPECT_FALSE(h9_setAlgorithm(h9obj, num_modules, 0));
}

TEST_F(TEST_CLASS, h9_setAlgorithm_withInvalidAlgorithm_returnsFalse) {
    size_t num_algorithms = h9_numAlgorithms(h9obj, 1);
    EXPECT_FALSE(h9_setAlgorithm(h9obj, 1, num_algorithms));
}

TEST_F(TEST_CLASS, h9_setAlgorithm_withValidAlgorithm_returnsTrue) {
    EXPECT_TRUE(h9_setAlgorithm(h9obj, 1, 1));
}

TEST_F(TEST_CLASS, h9_setAlgorithm_withValidAlgorithm_marksPresetDirty) {
    EXPECT_FALSE(h9_dirty(h9obj));
    EXPECT_TRUE(h9_setAlgorithm(h9obj, 1, 1));
    EXPECT_TRUE(h9_dirty(h9obj));
}

TEST_F(TEST_CLASS, h9_setAlgorithm_withValidAlgorithm_setsModule) {
    h9_algorithm *current_algorithm = h9_currentAlgorithm(h9obj);
    EXPECT_NE(current_algorithm->id, 1);
    EXPECT_NE(current_algorithm->module_id, 1);
    EXPECT_TRUE(h9_setAlgorithm(h9obj, 1, 1));
    current_algorithm = h9_currentAlgorithm(h9obj);
    EXPECT_EQ(current_algorithm->id, 1);
    EXPECT_EQ(current_algorithm->module_id, 1);
}

TEST_F(TEST_CLASS, h9_currentModule_returnsActivePresetModule) {
    h9_module *space_module = &h9_modules[3];
    EXPECT_TRUE(h9_setAlgorithm(h9obj, 3, 6));  // ModEchoVerb
    h9_module *current_module = h9_currentModule(h9obj);
    EXPECT_EQ(current_module, space_module);
}

TEST_F(TEST_CLASS, h9_currentModuleIndex_returnsActivePresetModuleIndex) {
    h9_module *space_module = &h9_modules[3];
    EXPECT_TRUE(h9_setAlgorithm(h9obj, 3, 6));  // ModEchoVerb
    EXPECT_EQ(&h9_modules[h9_currentModuleIndex(h9obj)], space_module);
}

TEST_F(TEST_CLASS, h9_currentAlgorithm_returnsActivePresetAlgorithm) {
    h9_algorithm *modechoverb = &h9_modules[3].algorithms[6];
    EXPECT_TRUE(h9_setAlgorithm(h9obj, 3, 6));
    h9_algorithm *current_algorithm = h9_currentAlgorithm(h9obj);
    EXPECT_EQ(current_algorithm, modechoverb);
}

TEST_F(TEST_CLASS, h9_currentAlgorithmIndex_returnsActivePresetAlgorithmId) {
    h9_algorithm *modechoverb = &h9_modules[3].algorithms[6];
    EXPECT_TRUE(h9_setAlgorithm(h9obj, 3, 6));
    EXPECT_EQ(&h9_modules[3].algorithms[h9_currentAlgorithmIndex(h9obj)], modechoverb);
}

TEST_F(TEST_CLASS, h9_moduleName_returnsModuleName) {
    for (size_t i = 0; i < H9_NUM_MODULES; i++) {
        EXPECT_EQ(h9_moduleName(i), h9_modules[i].name);
    }
}

TEST_F(TEST_CLASS, h9_algorithmName_returnsAlgorithmName) {
    h9_algorithm *modechoverb = &h9_modules[3].algorithms[6];
    EXPECT_STREQ(h9_algorithmName(3, 6), modechoverb->name);
}

TEST_F(TEST_CLASS, h9_currentAlgorithmName_returnsCurrentAlgorithmName) {
    h9_algorithm *modechoverb = &h9_modules[3].algorithms[6];
    EXPECT_TRUE(h9_setAlgorithm(h9obj, 3, 6));
    EXPECT_STREQ(h9_currentAlgorithmName(h9obj), modechoverb->name);
}

TEST_F(TEST_CLASS, h9_currentModuleName_returnsCurrentModuleName) {
    h9_module *space = &h9_modules[3];
    EXPECT_TRUE(h9_setAlgorithm(h9obj, 3, 6));
    EXPECT_STREQ(h9_currentModuleName(h9obj), space->name);
}

TEST_F(TEST_CLASS, h9_setPresetName_withValidName_returnsTrue) {
    char const *new_name = "DoodleToot";
    EXPECT_TRUE(h9_setPresetName(h9obj, new_name, strlen(new_name)));
}

TEST_F(TEST_CLASS, h9_setPresetName_withValidName_updatesPresetName) {
    char const *new_name = "DoodleToot";
    h9_setPresetName(h9obj, new_name, strlen(new_name));
    size_t len;
    EXPECT_STREQ(h9_presetName(h9obj, &len), new_name);
    EXPECT_EQ(len, strlen(new_name));
}

TEST_F(TEST_CLASS, h9_setPresetName_withValidName_whenNameLenExceedsLimit_updatesPresetName) {
    char const *new_name = "ThisNameIsMuchTooLong";
    h9_setPresetName(h9obj, new_name, strlen(new_name));
    size_t      len;
    char const *current_name = h9_presetName(h9obj, &len);
    EXPECT_LT(len, strlen(new_name));

    for (size_t i = 0; i < len; i++) {
        EXPECT_EQ(new_name[i], current_name[i]);
    }
}

TEST_F(TEST_CLASS, h9_setPresetName_withValidName_trimsTrailingWhitespace) {
    char const *new_name      = "Doodle    Toot    ";
    char const *expected_name = "Doodle    Toot";
    h9_setPresetName(h9obj, new_name, strlen(new_name));
    size_t len;
    EXPECT_STREQ(h9_presetName(h9obj, &len), expected_name);
    EXPECT_EQ(len, strlen(expected_name));
}

TEST_F(TEST_CLASS, h9_setPresetName_withInvalidCharacters_replacesWithSpaces) {
    char const *new_name      = "Doodle()\\}}Toot[12]    ";
    char const *expected_name = "Doodle     Toot ";
    h9_setPresetName(h9obj, new_name, strlen(new_name));
    size_t len;
    EXPECT_STREQ(h9_presetName(h9obj, &len), expected_name);
    EXPECT_EQ(len, strlen(expected_name));
}

}  // namespace h9_test
