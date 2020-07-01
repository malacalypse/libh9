//
//  utils.h
//  h9
//
//  Created by Studio DC on 2020-06-29.
//

#ifndef utils_h
#define utils_h

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t   hexdump(char *dest, size_t max_len, uint8_t *data, size_t len);
size_t   scanhex(char *str, uint32_t *dest, size_t len);
size_t   scanfloat(char *str, float *dest, size_t len);
uint16_t array_sum(uint32_t *array, size_t len);
uint16_t iarray_sumf(float *array, size_t len);
float    clip(float value, float min, float max);

#ifdef __cplusplus
}
#endif

#endif /* utils_h */
