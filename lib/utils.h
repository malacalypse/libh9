//
//  utils.h
//  h9
//
//  Created by Studio DC on 2020-06-29.
//

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
