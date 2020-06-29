//
//  test_helpers.hpp
//  h9_gtest
//
//  Created by Studio DC on 2020-07-01.
//

#ifndef test_helpers_hpp
#define test_helpers_hpp

#include <stddef.h>
#include <stdint.h>
#include "h9.h"

void     cc_callback(void *ctx, uint8_t midi_channel, uint8_t cc_num, uint8_t msb, uint8_t lsb);
void     display_callback(void *ctx, control_id control, control_value current_value, control_value display_value);
void     sysex_callback(void *ctx, uint8_t *sysex, size_t len);
void     init_callback_helpers(void);
bool     cc_callback_triggered(uint8_t cc, uint8_t *callback_value);
uint32_t cc_callback_count(void);
bool     display_callback_triggered(control_id control, control_value *callback_value);
bool     sysex_callback_triggered(uint8_t **sysex, size_t *len);

#endif /* test_helpers_hpp */
