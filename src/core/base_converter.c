#include "base_converter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void base_converter_init(base_converter_state_t *bc) {
    if (!bc) return;
    memset(bc, 0, sizeof(*bc));
    bc->bit_width = 32;
    bc->preserve_leading_zeros = true;
    base_converter_update_from_uint64(bc, 0, 32);
}

void base_converter_update_from_uint64(base_converter_state_t *bc, uint64_t val, int bit_width) {
    if (!bc) return;
    if (bit_width <= 0 || bit_width > 64) bit_width = 32;
    bc->bit_width = bit_width;

    uint64_t mask = (bit_width == 64) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << bit_width) - 1);
    val &= mask;

    // Hex
    if (bc->preserve_leading_zeros) {
        int hex_digits = (bit_width + 3) / 4;
        snprintf(bc->hex, sizeof(bc->hex), "%0*llX", hex_digits, (unsigned long long)val);
    } else {
        snprintf(bc->hex, sizeof(bc->hex), "%llX", (unsigned long long)val);
    }

    // Dec
    snprintf(bc->dec, sizeof(bc->dec), "%llu", (unsigned long long)val);

    // Oct
    if (bc->preserve_leading_zeros) {
        int oct_digits = (bit_width + 2) / 3;
        snprintf(bc->oct, sizeof(bc->oct), "%0*llo", oct_digits, (unsigned long long)val);
    } else {
        snprintf(bc->oct, sizeof(bc->oct), "%llo", (unsigned long long)val);
    }

    // Bin
    if (bc->preserve_leading_zeros) {
        for (int i = bit_width - 1; i >= 0; i--) {
            bc->bin[bit_width - 1 - i] = (val & (1ULL << i)) ? '1' : '0';
        }
        bc->bin[bit_width] = '\0';
    } else {
        int start = 63;
        while (start > 0 && !(val & (1ULL << start))) start--;
        int idx = 0;
        for (int i = start; i >= 0; i--) {
            bc->bin[idx++] = (val & (1ULL << i)) ? '1' : '0';
        }
        bc->bin[idx] = '\0';
    }
}

bool base_converter_update_from_bin(base_converter_state_t *bc, const char *bin_str) {
    if (!bc || !bin_str) return false;
    uint64_t val = 0;
    int count = 0;
    for (size_t i = 0; bin_str[i]; i++) {
        if (isspace((unsigned char)bin_str[i])) continue;
        if (bin_str[i] != '0' && bin_str[i] != '1') return false;
        val = (val << 1) | (bin_str[i] - '0');
        count++;
    }
    int width = count > 32 ? 64 : (count > 16 ? 32 : (count > 8 ? 16 : 8));
    base_converter_update_from_uint64(bc, val, width);
    return true;
}

bool base_converter_update_from_oct(base_converter_state_t *bc, const char *oct_str) {
    if (!bc || !oct_str) return false;
    char *end = NULL;
    uint64_t val = strtoull(oct_str, &end, 8);
    if (!end || *end != '\0') return false;
    base_converter_update_from_uint64(bc, val, bc->bit_width);
    return true;
}

bool base_converter_update_from_dec(base_converter_state_t *bc, const char *dec_str) {
    if (!bc || !dec_str) return false;
    char *end = NULL;
    uint64_t val = strtoull(dec_str, &end, 10);
    if (!end || *end != '\0') return false;
    base_converter_update_from_uint64(bc, val, bc->bit_width);
    return true;
}

bool base_converter_update_from_hex(base_converter_state_t *bc, const char *hex_str) {
    if (!bc || !hex_str) return false;
    char *end = NULL;
    uint64_t val = strtoull(hex_str, &end, 16);
    if (!end || *end != '\0') return false;
    base_converter_update_from_uint64(bc, val, bc->bit_width);
    return true;
}
