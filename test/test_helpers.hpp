/*  test_helpers.hpp
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

#ifndef test_helpers_hpp
#define test_helpers_hpp

#include <stddef.h>
#include <stdint.h>
#include "libh9.h"

void     cc_callback(void *ctx, uint8_t midi_channel, uint8_t cc_num, uint8_t msb, uint8_t lsb);
void     display_callback(void *ctx, control_id control, control_value current_value, control_value display_value);
void     sysex_callback(void *ctx, uint8_t *sysex, size_t len);
void     init_callback_helpers(void);
bool     cc_callback_triggered(uint8_t cc, uint8_t *callback_value);
uint32_t cc_callback_count(void);
bool     display_callback_triggered(control_id control, control_value *callback_value);
bool     sysex_callback_triggered(uint8_t **sysex, size_t *len);

#endif /* test_helpers_hpp */
