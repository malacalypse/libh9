//
//  utils.h
//  h9
//
//  Created by Studio DC on 2020-06-29.
//

#ifndef utils_h
#define utils_h

#ifndef NDEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#include <stdint.h>
#include <stddef.h>

void hexdump(uint8_t* data, size_t len);
size_t scanhex(char *str, uint32_t *dest, size_t len);
size_t scanfloat(char* str, float *dest, size_t len);
uint16_t array_sum(uint32_t *array, size_t len);
uint16_t iarray_sumf(float *array, size_t len);

#endif /* utils_h */
