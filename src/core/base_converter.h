#ifndef HXD_BASE_CONVERTER_H
#define HXD_BASE_CONVERTER_H

#include "types.h"
#include <stdbool.h>

typedef struct {
    char bin[128];
    char oct[64];
    char dec[64];
    char hex[64];
    bool preserve_leading_zeros;
    int  bit_width; // 8, 16, 24, 32, 64
} base_converter_state_t;

void base_converter_init(base_converter_state_t *bc);
void base_converter_update_from_uint64(base_converter_state_t *bc, uint64_t val, int bit_width);
bool base_converter_update_from_bin(base_converter_state_t *bc, const char *bin_str);
bool base_converter_update_from_oct(base_converter_state_t *bc, const char *oct_str);
bool base_converter_update_from_dec(base_converter_state_t *bc, const char *dec_str);
bool base_converter_update_from_hex(base_converter_state_t *bc, const char *hex_str);

#endif // HXD_BASE_CONVERTER_H
