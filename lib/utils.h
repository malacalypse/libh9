/*  utils.h
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

#ifndef utils_h
#define utils_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t   hexdump(char *dest, size_t max_len, uint8_t *data, size_t len);
size_t   scanhex(char *str, size_t strlen, uint32_t *dest, size_t destlen);
size_t   scanhex_word(char *str, size_t strlen, uint16_t *dest, size_t destlen);
size_t   scanhex_byte(char *str, size_t strlen, uint8_t *dest, size_t destlen);
size_t   scanhex_bool(char *str, size_t strlen, bool *dest, size_t destlen);
size_t   scanhex_bool32(char *str, size_t strlen, uint32_t *dest);
size_t   scanfloat(char *str, size_t strlen, float *dest, size_t len);
uint16_t array_sum(uint32_t *array, size_t len);
uint16_t array_sum16(uint16_t *array, size_t len);
uint16_t array_sum8(uint8_t *array, size_t len);
uint16_t array_sum1(bool *array, size_t len);
uint16_t iarray_sumf(float *array, size_t len);
float    clip(float value, float min, float max);
size_t   find_lines(char *str, size_t strlen, char *line_heads[], size_t *line_lengths, size_t max_lines);

#ifdef __cplusplus
}
#endif

#endif /* utils_h */
