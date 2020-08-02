/*  test_helpers.cpp
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

#include "test_helpers.hpp"

#define DISPLAY_CALLBACK_NULL -1.0f
#define CC_CALLBACK_NULL      -1
#ifndef MAX_CC
#define MAX_CC 127
#endif

static control_value display_callback_tracker[12];
static int8_t        cc_callback_tracker[MAX_CC + 1];
static uint32_t      cc_callback_counter = 0U;
static uint8_t *     sysex_callback_tracker;
static size_t        sysex_callback_tracker_len;

void cc_callback(void *ctx, uint8_t midi_channel, uint8_t cc_num, uint8_t msb, uint8_t lsb) {
    cc_callback_tracker[cc_num] = static_cast<int8_t>(msb);
    cc_callback_counter++;
}

void display_callback(void *ctx, control_id control, control_value current_value, control_value display_value) {
    display_callback_tracker[control] = display_value;
}

void sysex_callback(void *ctx, uint8_t *sysex, size_t len) {
    sysex_callback_tracker     = sysex;
    sysex_callback_tracker_len = len;
}

void init_callback_helpers(void) {
    for (size_t i = 0; i < sizeof(display_callback_tracker) / sizeof(*display_callback_tracker); i++) {
        display_callback_tracker[i] = DISPLAY_CALLBACK_NULL;
    }
    for (size_t i = 0; i < sizeof(cc_callback_tracker) / sizeof(*cc_callback_tracker); i++) {
        cc_callback_tracker[i] = CC_CALLBACK_NULL;
    }
    sysex_callback_tracker     = nullptr;
    sysex_callback_tracker_len = 0;
    cc_callback_counter        = 0U;
}

bool cc_callback_triggered(uint8_t cc, uint8_t *callback_value) {
    if (cc_callback_tracker[cc] == CC_CALLBACK_NULL) {
        return false;
    }
    if (callback_value != nullptr) {
        *callback_value = cc_callback_tracker[cc];
    }
    return true;
}

uint32_t cc_callback_count(void) {
    return cc_callback_counter;
}

bool display_callback_triggered(control_id control, control_value *callback_value) {
    if (display_callback_tracker[control] == DISPLAY_CALLBACK_NULL) {
        return false;
    }
    if (callback_value != nullptr) {
        *callback_value = display_callback_tracker[control];
    }
    return true;
}

bool sysex_callback_triggered(uint8_t **sysex, size_t *len) {
    if ((sysex_callback_tracker_len == 0) && (sysex_callback_tracker != nullptr)) {
        return false;
    }
    *sysex = sysex_callback_tracker;
    *len   = sysex_callback_tracker_len;
    return true;
}
