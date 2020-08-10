/*  utils.c
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

#include "utils.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_LEVEL DEBUG_ERROR
#include "debug.h"

size_t hexdump(char *dest, size_t max_len, uint8_t *data, size_t len) {
    char *cursor = dest;
    char *end    = dest + max_len;
    for (size_t i = 0; i < len; i++) {
        if (cursor < (end - 2)) {
            cursor += sprintf(cursor, "%02x", (int)data[i]);
            if ((cursor < end - 1) && (i != 0) && ((i + 1) % 4 == 0)) {
                cursor += sprintf(cursor, " ");
            }
        }
    }
    size_t bytes_written = cursor - dest;
    return bytes_written;
}

// Returns 0 if not pure hex chars 0-9a-fA-F
uint8_t htoi(char h) {
    if (h >= '0' && h <= '9') {
        return h - '0';
    } else if (h >= 'a' && h <= 'f') {  // 97-102
        return h - 87;
    } else if (h >= 'A' && h <= 'F') {  // 65-70
        return h - 55;
    } else {
        return 0;
    }
}

size_t scanhex(char *str, size_t strlen, uint32_t *dest, size_t destlen) {
    int  dest_i = -1;
    bool found  = false;

    for (size_t i = 0; (i < strlen) && (dest_i < (int)destlen); i++) {
        char current = str[i];
        if ((current >= '0' && current <= '9') || (current >= 'a' && current <= 'f') || (current >= 'A' && current <= 'F')) {
            if (!found) {
                dest_i++;
                dest[dest_i] = htoi(current);
                found        = true;
            } else {
                dest[dest_i] = (dest[dest_i] << 4) + htoi(current);
            }
        } else if (current == ' ') {
            found = false;
        } else {
            break;  // we found a non-hex value.
        }
    }

    return dest_i + 1;
}

size_t scanhex_word(char *str, size_t strlen, uint16_t *dest, size_t destlen) {
    int  dest_i = -1;
    bool found  = false;

    for (size_t i = 0; (i < strlen) && (dest_i < (int)destlen); i++) {
        char current = str[i];
        if ((current >= '0' && current <= '9') || (current >= 'a' && current <= 'f') || (current >= 'A' && current <= 'F')) {
            if (!found) {
                dest_i++;
                dest[dest_i] = htoi(current);
                found        = true;
            } else {
                dest[dest_i] = (dest[dest_i] << 4) + htoi(current);
            }
        } else if (current == ' ') {
            found = false;
        } else {
            break;  // we found a non-hex value.
        }
    }

    return dest_i + 1;
}

size_t scanhex_byte(char *str, size_t strlen, uint8_t *dest, size_t destlen) {
    int  dest_i = -1;
    bool found  = false;

    for (size_t i = 0; (i < strlen) && (dest_i < (int)destlen); i++) {
        char current = str[i];
        if ((current >= '0' && current <= '9') || (current >= 'a' && current <= 'f') || (current >= 'A' && current <= 'F')) {
            if (!found) {
                dest_i++;
                dest[dest_i] = htoi(current);
                found        = true;
            } else {
                dest[dest_i] = (dest[dest_i] << 4) + htoi(current);
            }
        } else if (current == ' ') {
            found = false;
        } else {
            break;  // we found a non-hex value.
        }
    }

    return dest_i + 1;
}

size_t scanhex_bool(char *str, size_t strlen, bool *dest, size_t destlen) {
    int dest_i = 0;

    for (size_t i = 0; (i < strlen) && (dest_i < destlen); i++) {
        char current = str[i];
        switch (current) {
            case '0':
                dest[dest_i++] = 0;
                break;
            case '1':
                dest[dest_i++] = 1;
                break;
            case ' ':
                continue;
            default:
                break;
        }
    }

    return dest_i;
}

size_t scanhex_bool32(char *str, size_t strlen, uint32_t *dest) {
    char * cread  = str;
    size_t offset = 0;

    for (size_t i = 0; (i < strlen) && offset < 32; i++) {
        char current = *cread++;
        switch (current) {
            case '0':
                *dest = (*dest << 1);
                offset++;
                break;
            case '1':
                *dest = (*dest << 1) + 1;
                offset++;
                break;
            case ' ':
                break;
            default:
                return offset;
        }
    }
    return offset;
}

size_t scanfloat(char *str, size_t strlen, float *dest, size_t destlen) {
    size_t dest_i = 0;
    bool   found  = false;
    char * cursor;

    for (size_t i = 0; (i < strlen) && (dest_i < (int)destlen); i++) {
        char current = str[i];
        if ((current >= '0' && current <= '9') || (current == '.')) {
            if (!found) {
                cursor = &str[i];
                found  = true;
            }
        } else if (current == ' ') {
            if (found) {
                sscanf(cursor, "%f", &dest[dest_i++]);
                found = false;
            }
        } else {
            break;  // we found a non-float value.
        }
    }
    // Catch any last pending value
    if (found) {
        sscanf(cursor, "%f", &dest[dest_i++]);
        found = false;
    }

    return dest_i;
}

uint16_t array_sum(uint32_t *array, size_t len) {
    uint16_t result = 0U;
    for (size_t i = 0; i < len; i++) {
        result += array[i];
    }
    return result;
}

uint16_t array_sum16(uint16_t *array, size_t len) {
    uint16_t result = 0U;
    for (size_t i = 0; i < len; i++) {
        result += array[i];
    }
    return result;
}

uint16_t array_sum8(uint8_t *array, size_t len) {
    uint16_t result = 0U;
    for (size_t i = 0; i < len; i++) {
        result += array[i];
    }
    return result;
}

uint16_t array_sum1(bool *array, size_t len) {
    uint16_t result = 0U;
    for (size_t i = 0; i < len; i++) {
        if (array[i]) {
            result++;
        }
    }
    return result;
}

uint16_t iarray_sumf(float *array, size_t len) {
    uint16_t result = 0U;
    for (size_t i = 0; i < len; i++) {
        result += (uint16_t)truncf(array[i]);
    }
    return result;
}

float clip(float value, float min, float max) {
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    } else {
        return value;
    }
}

size_t find_lines(char *str, size_t strlen, char *line_heads[], size_t *line_lengths, size_t max_lines) {
    size_t num_lines    = 0;
    size_t line_index   = 0;
    size_t str_index    = 0;
    bool   accumulating = false;

    for (str_index = 0; (str_index <= strlen) && (line_index < max_lines); str_index++) {
        char current = str[str_index];
        bool eof     = (str_index == strlen);
        if (current == '\r' || current == '\n' || eof) {
            if (accumulating) {
                // Complete the current line
                line_lengths[line_index] = (uintptr_t)&str[str_index] - (uintptr_t)line_heads[line_index];
                line_index++;
                num_lines++;
                accumulating = false;
            }
        } else {
            // Accumulate
            if (!accumulating) {
                accumulating           = true;
                line_heads[line_index] = &str[str_index];
            }
        }
    }

    return num_lines;
}
