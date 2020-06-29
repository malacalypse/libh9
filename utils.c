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

void hexdump(uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        debug("%02x", (int)data[i]);
        if ((i != 0) && ((i + 1) % 4 == 0)) {
            debug(" ");
        }
    }
    debug("\n");
}

size_t scanhex(char *str, uint32_t *dest, size_t len) {
    FILE *stream;
    size_t i = 0;

    stream = fmemopen(str, strlen(str), "r");
    for (i = 0; i < len; i++) {
        if (fscanf(stream, "%x", &dest[i]) != 1) {
            debug("Failed scanning ASCII hex: successfully read %zu of %zu values.\n", i, len);
            return i;
        }
    }

    debug("Scanned: ");
    for (size_t i = 0; i < len; i++) {
        debug("%d ", dest[i]);
    }
    debug("\n");

    return i;
}

size_t scanfloat(char* str, float *dest, size_t len) {
    FILE *stream;
    size_t i = 0;

    stream = fmemopen(str, strlen(str), "r");
    for (i = 0; i < len; i++) {
        if (fscanf(stream, "%f", &dest[i]) != 1) {
            debug("Failed scanning ASCII hex: successfully read %zu of %zu values.\n", i, len);
            return i;
        }
    }

    printf("Scanned: ");
    for (size_t i = 0; i < len; i++) {
        debug("%f ", dest[i]);
    }
    debug("\n");

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
