//
//  utils.c
//  h9
//
//  Created by Studio DC on 2020-06-29.
//

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

size_t scanhex(char *str, uint32_t *dest, size_t len) {
    FILE * stream;
    size_t i = 0;

    stream = fmemopen(str, strlen(str), "r");
    for (i = 0; i < len; i++) {
        if (fscanf(stream, "%x", &dest[i]) != 1) {
            debug_info("Failed scanning ASCII hex: successfully read %zu of %zu values.\n", i, len);
            return i;
        }
    }

    debug_info("Scanned: ");
    for (size_t i = 0; i < len; i++) {
        debug_info("%d ", dest[i]);
    }
    debug_info("\n");

    return i;
}

size_t scanhex_word(char *str, size_t strlen, uint16_t *dest, size_t destlen) {
    char *    cread    = str;
    uint16_t *cwrite   = dest;
    uint16_t  currword = 0U;
    uint8_t   byteval  = 0U;
    bool      parsed   = false;

    for (size_t i = 0; (i < strlen) && (((uintptr_t)cwrite - (uintptr_t)dest) < destlen); i++) {
        char current = *cread++;
        if (current >= '0' && current <= '9') {
            byteval = (uint8_t)(current - '0');
            parsed  = true;
        } else if (current >= 'a' && current <= 'f') {
            byteval = (uint8_t)(current - 'a');
            parsed  = true;
        } else if (current >= 'A' && current <= 'F') {
            byteval = (uint8_t)(current - 'A');
            parsed  = true;
        } else if (current == ' ') {
            if (parsed) {
                *cwrite++ = currword;
                currword  = 0;
                byteval   = 0;
                parsed    = false;
            }
            continue;
        } else {
            break;  // we found a non-hex value.
        }
        currword = (currword << 4) + byteval;  // Safe with repeated spaces
    }
    size_t found = ((uintptr_t)cwrite - (uintptr_t)dest);

    debug_info("Scanned: ");
    for (size_t i = 0; i < found; i++) {
        debug_info("%d ", dest[i]);
    }
    debug_info("\n");

    return found;
}

size_t scanhex_byte(char *str, size_t strlen, uint8_t *dest, size_t destlen) {
    char *   cread   = str;
    uint8_t *cwrite  = dest;
    uint8_t  byteval = 0U;
    bool     parsed  = false;

    for (size_t i = 0; (i < strlen) && (((uintptr_t)cwrite - (uintptr_t)dest) < destlen); i++) {
        char current = *cread++;
        if (current >= '0' && current <= '9') {
            byteval = (uint8_t)(current - '0');
            parsed  = true;
        } else if (current >= 'a' && current <= 'f') {
            byteval = (uint8_t)(current - 'a');
            parsed  = true;
        } else if (current >= 'A' && current <= 'F') {
            byteval = (uint8_t)(current - 'A');
            parsed  = true;
        } else if (current == ' ') {
            if (parsed) {
                *cwrite++ = byteval;
                byteval   = 0;
                parsed    = false;
            }
            continue;
        } else {
            break;  // we found a non-hex value.
        }
        byteval = (byteval << 4) + byteval;  // Safe with repeated spaces
    }
    size_t found = ((uintptr_t)cwrite - (uintptr_t)dest);

    debug_info("Scanned: ");
    for (size_t i = 0; i < found; i++) {
        debug_info("%d ", dest[i]);
    }
    debug_info("\n");

    return found;
}

size_t scanhex_bool(char *str, size_t strlen, bool *dest, size_t destlen) {
    char *cread  = str;
    bool *cwrite = dest;

    for (size_t i = 0; (i < strlen) && (((uintptr_t)cwrite - (uintptr_t)dest) < destlen); i++) {
        char current = *cread++;
        switch (current) {
            case '0':
                *cwrite++ = false;
            case '1':
                *cwrite++ = true;
                break;
            case ' ':
                break;
            default:
                return i;
        }
    }
    size_t found = ((uintptr_t)cwrite - (uintptr_t)dest);

    debug_info("Scanned: ");
    for (size_t i = 0; i < found; i++) {
        debug_info("%d ", dest[i]);
    }
    debug_info("\n");

    return found;
}

size_t scanhex_bool32(char *str, size_t strlen, uint32_t *dest) {
    char * cread  = str;
    size_t offset = 0;

    for (size_t i = 0; (i < strlen) && offset < 32; i++) {
        char current = *cread++;
        switch (current) {
            case '0':
                *dest = (*dest << 1);
            case '1':
                *dest = (*dest << 1) + 1;
                break;
            case ' ':
                break;
            default:
                return i;
        }
    }
    debug_info("Scanned: %x\n", *dest);
    return offset;
}

size_t scanfloat(char *str, float *dest, size_t len) {
    FILE * stream;
    size_t i = 0;

    stream = fmemopen(str, strlen(str), "r");
    for (i = 0; i < len; i++) {
        if (fscanf(stream, "%f", &dest[i]) != 1) {
            debug_info("Failed scanning ASCII hex: successfully read %zu of %zu values.\n", i, len);
            return i;
        }
    }

    debug_info("Scanned: ");
    for (size_t i = 0; i < len; i++) {
        debug_info("%f ", dest[i]);
    }
    debug_info("\n");

    return i;
}

uint16_t array_sum(uint32_t *array, size_t len) {
    uint16_t result = 0U;
    for (size_t i = 0; i < len; i++) {
        result += array[i];
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
