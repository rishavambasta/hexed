#include "data_inspector.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* IEEE 754 half precision float decoder */
static float decode_float16(uint16_t h) {
    uint32_t sign = (h >> 15) & 0x0001;
    uint32_t exp  = (h >> 10) & 0x001F;
    uint32_t mant = h & 0x03FF;

    if (exp == 0) {
        if (mant == 0) return sign ? -0.0f : 0.0f;
        // Subnormal
        while (!(mant & 0x0400)) {
            mant <<= 1;
            exp--;
        }
        exp++;
        mant &= ~0x0400;
        exp = exp + (127 - 15);
        mant = mant << 13;
    } else if (exp == 31) {
        if (mant == 0) return sign ? -INFINITY : INFINITY;
        return NAN;
    } else {
        exp = exp + (127 - 15);
        mant = mant << 13;
    }

    uint32_t f_bits = (sign << 31) | (exp << 23) | mant;
    float res;
    memcpy(&res, &f_bits, sizeof(float));
    return res;
}

static uint16_t encode_float16(float f) {
    uint32_t f_bits;
    memcpy(&f_bits, &f, sizeof(float));
    uint32_t sign = (f_bits >> 31) & 0x1;
    int32_t  exp  = ((f_bits >> 23) & 0xFF) - 127 + 15;
    uint32_t mant = (f_bits >> 13) & 0x3FF;

    if (exp <= 0) return (uint16_t)(sign << 15);
    if (exp >= 31) return (uint16_t)((sign << 15) | 0x7C00);
    return (uint16_t)((sign << 15) | ((exp & 0x1F) << 10) | (mant & 0x3FF));
}

static uint16_t read16(const uint8_t *b, hxd_endianness_t endian) {
    if (endian == HXD_ENDIAN_LITTLE) return (uint16_t)(b[0] | (b[1] << 8));
    else return (uint16_t)((b[0] << 8) | b[1]);
}

static uint32_t read24(const uint8_t *b, hxd_endianness_t endian) {
    if (endian == HXD_ENDIAN_LITTLE) return (uint32_t)(b[0] | (b[1] << 8) | (b[2] << 16));
    else return (uint32_t)((b[0] << 16) | (b[1] << 8) | b[2]);
}

static uint32_t read32(const uint8_t *b, hxd_endianness_t endian) {
    if (endian == HXD_ENDIAN_LITTLE) return (uint32_t)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
    else return (uint32_t)((b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]);
}

static uint64_t read64(const uint8_t *b, hxd_endianness_t endian) {
    uint64_t v = 0;
    if (endian == HXD_ENDIAN_LITTLE) {
        for (int i = 7; i >= 0; i--) v = (v << 8) | b[i];
    } else {
        for (int i = 0; i < 8; i++) v = (v << 8) | b[i];
    }
    return v;
}

void inspector_compute(const piece_table_t *pt, hxd_offset_t offset, hxd_endianness_t endian, bool display_hex, inspector_results_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->endian = endian;
    out->display_hex = display_hex;

    if (!pt || offset >= pt_get_size(pt)) return;

    uint8_t raw[16];
    bool is_gap[16];
    hxd_size_t avail = pt_read_bytes(pt, offset, raw, sizeof(raw), is_gap);
    if (avail == 0) return;

    #define PUSH_ENTRY(eid, ename, edesc, req_len) \
        inspector_entry_t *e = &out->entries[out->count++]; \
        e->id = eid; e->name = ename; e->description = edesc; e->bytes_required = req_len; \
        e->is_valid = (avail >= req_len);

    // 1. Binary (8-bit)
    {
        PUSH_ENTRY("BINARY", "Binary", "8-bit binary representation (bits 0-7)", 1);
        if (e->is_valid) {
            for (int b = 7; b >= 0; b--) {
                e->value[7 - b] = (raw[0] & (1 << b)) ? '1' : '0';
            }
            e->value[8] = '\0';
        }
    }

    // 2. Int8
    {
        PUSH_ENTRY("INT8", "Int8", "Signed 8-bit integer (-128 to 127)", 1);
        if (e->is_valid) {
            int8_t v = (int8_t)raw[0];
            if (display_hex) snprintf(e->value, sizeof(e->value), "0x%02X (%d)", (uint8_t)v, v);
            else snprintf(e->value, sizeof(e->value), "%d", v);
        }
    }

    // 3. UInt8
    {
        PUSH_ENTRY("UINT8", "UInt8", "Unsigned 8-bit integer (0 to 255)", 1);
        if (e->is_valid) {
            uint8_t v = raw[0];
            if (display_hex) snprintf(e->value, sizeof(e->value), "0x%02X (%u)", v, v);
            else snprintf(e->value, sizeof(e->value), "%u", v);
        }
    }

    // 4. Int16
    {
        PUSH_ENTRY("INT16", "Int16", "Signed 16-bit integer (-32,768 to 32,767)", 2);
        if (e->is_valid) {
            int16_t v = (int16_t)read16(raw, endian);
            if (display_hex) snprintf(e->value, sizeof(e->value), "0x%04X (%d)", (uint16_t)v, v);
            else snprintf(e->value, sizeof(e->value), "%d", v);
        }
    }

    // 5. UInt16
    {
        PUSH_ENTRY("UINT16", "UInt16", "Unsigned 16-bit integer (0 to 65,535)", 2);
        if (e->is_valid) {
            uint16_t v = read16(raw, endian);
            if (display_hex) snprintf(e->value, sizeof(e->value), "0x%04X (%u)", v, v);
            else snprintf(e->value, sizeof(e->value), "%u", v);
        }
    }

    // 6. Int24
    {
        PUSH_ENTRY("INT24", "Int24", "Signed 24-bit integer (-8,388,608 to 8,388,607)", 3);
        if (e->is_valid) {
            uint32_t uv = read24(raw, endian);
            int32_t v = (uv & 0x800000) ? (int32_t)(uv | 0xFF000000) : (int32_t)uv;
            if (display_hex) snprintf(e->value, sizeof(e->value), "0x%06X (%d)", uv & 0xFFFFFF, v);
            else snprintf(e->value, sizeof(e->value), "%d", v);
        }
    }

    // 7. UInt24
    {
        PUSH_ENTRY("UINT24", "UInt24", "Unsigned 24-bit integer (0 to 16,777,215)", 3);
        if (e->is_valid) {
            uint32_t v = read24(raw, endian);
            if (display_hex) snprintf(e->value, sizeof(e->value), "0x%06X (%u)", v, v);
            else snprintf(e->value, sizeof(e->value), "%u", v);
        }
    }

    // 8. Int32
    {
        PUSH_ENTRY("INT32", "Int32", "Signed 32-bit integer (-2,147,483,648 to 2,147,483,647)", 4);
        if (e->is_valid) {
            int32_t v = (int32_t)read32(raw, endian);
            if (display_hex) snprintf(e->value, sizeof(e->value), "0x%08X (%d)", (uint32_t)v, v);
            else snprintf(e->value, sizeof(e->value), "%d", v);
        }
    }

    // 9. UInt32
    {
        PUSH_ENTRY("UINT32", "UInt32", "Unsigned 32-bit integer (0 to 4,294,967,295)", 4);
        if (e->is_valid) {
            uint32_t v = read32(raw, endian);
            if (display_hex) snprintf(e->value, sizeof(e->value), "0x%08X (%u)", v, v);
            else snprintf(e->value, sizeof(e->value), "%u", v);
        }
    }

    // 10. Int64
    {
        PUSH_ENTRY("INT64", "Int64", "Signed 64-bit integer", 8);
        if (e->is_valid) {
            int64_t v = (int64_t)read64(raw, endian);
            if (display_hex) snprintf(e->value, sizeof(e->value), "0x%016llX (%lld)", (unsigned long long)v, (long long)v);
            else snprintf(e->value, sizeof(e->value), "%lld", (long long)v);
        }
    }

    // 11. UInt64
    {
        PUSH_ENTRY("UINT64", "UInt64", "Unsigned 64-bit integer", 8);
        if (e->is_valid) {
            uint64_t v = read64(raw, endian);
            if (display_hex) snprintf(e->value, sizeof(e->value), "0x%016llX (%llu)", (unsigned long long)v, (unsigned long long)v);
            else snprintf(e->value, sizeof(e->value), "%llu", (unsigned long long)v);
        }
    }

    // 12. Float16
    {
        PUSH_ENTRY("HALF2", "Float16", "IEEE 754 half-precision floating point (16-bit)", 2);
        if (e->is_valid) {
            uint16_t h = read16(raw, endian);
            float f = decode_float16(h);
            snprintf(e->value, sizeof(e->value), "%.6g", f);
        }
    }

    // 13. Float32
    {
        PUSH_ENTRY("FLOAT4", "Float32", "IEEE 754 single-precision floating point (32-bit)", 4);
        if (e->is_valid) {
            uint32_t u = read32(raw, endian);
            float f;
            memcpy(&f, &u, sizeof(float));
            snprintf(e->value, sizeof(e->value), "%.8g", f);
        }
    }

    // 14. Float64
    {
        PUSH_ENTRY("DOUBLE8", "Float64", "IEEE 754 double-precision floating point (64-bit)", 8);
        if (e->is_valid) {
            uint64_t u = read64(raw, endian);
            double d;
            memcpy(&d, &u, sizeof(double));
            snprintf(e->value, sizeof(e->value), "%.16g", d);
        }
    }

    // 15. Rational (32-bit num / 32-bit den)
    {
        PUSH_ENTRY("RATIONAL", "Rational", "Unsigned 32-bit numerator/denominator fraction", 8);
        if (e->is_valid) {
            uint32_t num = read32(raw, endian);
            uint32_t den = read32(raw + 4, endian);
            snprintf(e->value, sizeof(e->value), "%u / %u", num, den);
        }
    }

    // 16. SRational (signed 32-bit num / signed 32-bit den)
    {
        PUSH_ENTRY("SRATIONAL", "SRational", "Signed 32-bit numerator/denominator fraction", 8);
        if (e->is_valid) {
            int32_t num = (int32_t)read32(raw, endian);
            int32_t den = (int32_t)read32(raw + 4, endian);
            snprintf(e->value, sizeof(e->value), "%d / %d", num, den);
        }
    }

    // 17. ASCII / ANSI
    {
        PUSH_ENTRY("ASCII_ANSI", "ASCII/ANSI", "7-bit ASCII / Windows-1252 (1 byte)", 1);
        if (e->is_valid) {
            uint8_t c = raw[0];
            if (c >= 32 && c <= 126) snprintf(e->value, sizeof(e->value), "'%c'", c);
            else snprintf(e->value, sizeof(e->value), "0x%02X (.)", c);
        }
    }

    // 18. UTF-8
    {
        PUSH_ENTRY("UTF8", "UTF-8", "Variable-width Unicode (1-4 bytes per char)", 1);
        if (e->is_valid) {
            uint8_t b0 = raw[0];
            if ((b0 & 0x80) == 0) {
                if (b0 >= 32 && b0 <= 126) snprintf(e->value, sizeof(e->value), "'%c' (U+%04X)", b0, b0);
                else snprintf(e->value, sizeof(e->value), "U+%04X", b0);
            } else if ((b0 & 0xE0) == 0xC0 && avail >= 2) {
                uint32_t cp = ((b0 & 0x1F) << 6) | (raw[1] & 0x3F);
                snprintf(e->value, sizeof(e->value), "U+%04X", cp);
            } else if ((b0 & 0xF0) == 0xE0 && avail >= 3) {
                uint32_t cp = ((b0 & 0x0F) << 12) | ((raw[1] & 0x3F) << 6) | (raw[2] & 0x3F);
                snprintf(e->value, sizeof(e->value), "U+%04X", cp);
            } else if ((b0 & 0xF8) == 0xF0 && avail >= 4) {
                uint32_t cp = ((b0 & 0x07) << 18) | ((raw[1] & 0x3F) << 12) | ((raw[2] & 0x3F) << 6) | (raw[3] & 0x3F);
                snprintf(e->value, sizeof(e->value), "U+%06X", cp);
            } else {
                snprintf(e->value, sizeof(e->value), "Invalid UTF-8");
            }
        }
    }

    // 19. UTF-16
    {
        PUSH_ENTRY("UTF16", "UTF-16", "16-bit Unicode (2 bytes per char)", 2);
        if (e->is_valid) {
            uint16_t cp = read16(raw, endian);
            if (cp >= 32 && cp <= 126) snprintf(e->value, sizeof(e->value), "'%c' (U+%04X)", (char)cp, cp);
            else snprintf(e->value, sizeof(e->value), "U+%04X", cp);
        }
    }

    // 20. IPv4 Address
    {
        PUSH_ENTRY("IPV4", "IPv4 Address", "IPv4 address (32-bit) in dotted decimal", 4);
        if (e->is_valid) {
            snprintf(e->value, sizeof(e->value), "%u.%u.%u.%u", raw[0], raw[1], raw[2], raw[3]);
        }
    }

    // 21. IPv6 Address
    {
        PUSH_ENTRY("IPV6", "IPv6 Address", "IPv6 address (128-bit) hexadecimal", 16);
        if (e->is_valid) {
            snprintf(e->value, sizeof(e->value), "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                     raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7],
                     raw[8], raw[9], raw[10], raw[11], raw[12], raw[13], raw[14], raw[15]);
        }
    }

    // 22. MAC Address
    {
        PUSH_ENTRY("MAC_ADDRESS", "MAC Address", "MAC-48/EUI-48 address (48-bit)", 6);
        if (e->is_valid) {
            snprintf(e->value, sizeof(e->value), "%02X:%02X:%02X:%02X:%02X:%02X",
                     raw[0], raw[1], raw[2], raw[3], raw[4], raw[5]);
        }
    }

    // 23. GUID / UUID
    {
        PUSH_ENTRY("GUID", "GUID/UUID", "128-bit GUID in 8-4-4-4-12 format", 16);
        if (e->is_valid) {
            uint32_t d1 = read32(raw, endian);
            uint16_t d2 = read16(raw + 4, endian);
            uint16_t d3 = read16(raw + 6, endian);
            snprintf(e->value, sizeof(e->value), "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                     d1, d2, d3, raw[8], raw[9], raw[10], raw[11], raw[12], raw[13], raw[14], raw[15]);
        }
    }

    // 24. RGB (8-8-8)
    {
        PUSH_ENTRY("RGB", "RGB (8-8-8)", "24-bit RGB color (Red, Green, Blue)", 3);
        if (e->is_valid) {
            e->has_color = true;
            e->color_r = raw[0];
            e->color_g = raw[1];
            e->color_b = raw[2];
            e->color_a = 255;
            snprintf(e->value, sizeof(e->value), "rgb(%u, %u, %u)", raw[0], raw[1], raw[2]);
        }
    }

    // 25. RGBA (8-8-8-8)
    {
        PUSH_ENTRY("RGBA", "RGBA (8-8-8-8)", "32-bit RGBA color with alpha", 4);
        if (e->is_valid) {
            e->has_color = true;
            e->color_r = raw[0];
            e->color_g = raw[1];
            e->color_b = raw[2];
            e->color_a = raw[3];
            snprintf(e->value, sizeof(e->value), "rgba(%u, %u, %u, %.2f)", raw[0], raw[1], raw[2], raw[3] / 255.0f);
        }
    }

    #undef PUSH_ENTRY
}

bool inspector_write_value(piece_table_t *pt, hxd_offset_t offset, const char *type_id, const char *input_str, hxd_endianness_t endian) {
    if (!pt || !type_id || !input_str) return false;

    uint8_t bytes[16];
    size_t len = 0;

    if (strcmp(type_id, "BINARY") == 0) {
        uint8_t b = 0;
        int count = 0;
        for (size_t i = 0; input_str[i] && count < 8; i++) {
            if (input_str[i] == '0' || input_str[i] == '1') {
                b = (b << 1) | (input_str[i] - '0');
                count++;
            }
        }
        bytes[0] = b;
        len = 1;
    } else if (strcmp(type_id, "INT8") == 0) {
        int v = (int)strtol(input_str, NULL, 0);
        bytes[0] = (uint8_t)(int8_t)v;
        len = 1;
    } else if (strcmp(type_id, "UINT8") == 0) {
        unsigned long v = strtoul(input_str, NULL, 0);
        bytes[0] = (uint8_t)v;
        len = 1;
    } else if (strcmp(type_id, "INT16") == 0) {
        int16_t v = (int16_t)strtol(input_str, NULL, 0);
        if (endian == HXD_ENDIAN_LITTLE) { bytes[0] = v & 0xFF; bytes[1] = (v >> 8) & 0xFF; }
        else { bytes[0] = (v >> 8) & 0xFF; bytes[1] = v & 0xFF; }
        len = 2;
    } else if (strcmp(type_id, "UINT16") == 0) {
        uint16_t v = (uint16_t)strtoul(input_str, NULL, 0);
        if (endian == HXD_ENDIAN_LITTLE) { bytes[0] = v & 0xFF; bytes[1] = (v >> 8) & 0xFF; }
        else { bytes[0] = (v >> 8) & 0xFF; bytes[1] = v & 0xFF; }
        len = 2;
    } else if (strcmp(type_id, "INT32") == 0) {
        int32_t v = (int32_t)strtoll(input_str, NULL, 0);
        if (endian == HXD_ENDIAN_LITTLE) {
            bytes[0] = v & 0xFF; bytes[1] = (v >> 8) & 0xFF;
            bytes[2] = (v >> 16) & 0xFF; bytes[3] = (v >> 24) & 0xFF;
        } else {
            bytes[0] = (v >> 24) & 0xFF; bytes[1] = (v >> 16) & 0xFF;
            bytes[2] = (v >> 8) & 0xFF; bytes[3] = v & 0xFF;
        }
        len = 4;
    } else if (strcmp(type_id, "UINT32") == 0) {
        uint32_t v = (uint32_t)strtoull(input_str, NULL, 0);
        if (endian == HXD_ENDIAN_LITTLE) {
            bytes[0] = v & 0xFF; bytes[1] = (v >> 8) & 0xFF;
            bytes[2] = (v >> 16) & 0xFF; bytes[3] = (v >> 24) & 0xFF;
        } else {
            bytes[0] = (v >> 24) & 0xFF; bytes[1] = (v >> 16) & 0xFF;
            bytes[2] = (v >> 8) & 0xFF; bytes[3] = v & 0xFF;
        }
        len = 4;
    } else if (strcmp(type_id, "INT64") == 0 || strcmp(type_id, "UINT64") == 0) {
        uint64_t v = (uint64_t)strtoull(input_str, NULL, 0);
        if (endian == HXD_ENDIAN_LITTLE) {
            for (int i = 0; i < 8; i++) bytes[i] = (uint8_t)((v >> (i * 8)) & 0xFF);
        } else {
            for (int i = 0; i < 8; i++) bytes[i] = (uint8_t)((v >> ((7 - i) * 8)) & 0xFF);
        }
        len = 8;
    } else if (strcmp(type_id, "FLOAT4") == 0) {
        float f = strtof(input_str, NULL);
        uint32_t u;
        memcpy(&u, &f, sizeof(float));
        if (endian == HXD_ENDIAN_LITTLE) {
            bytes[0] = u & 0xFF; bytes[1] = (u >> 8) & 0xFF;
            bytes[2] = (u >> 16) & 0xFF; bytes[3] = (u >> 24) & 0xFF;
        } else {
            bytes[0] = (u >> 24) & 0xFF; bytes[1] = (u >> 16) & 0xFF;
            bytes[2] = (u >> 8) & 0xFF; bytes[3] = u & 0xFF;
        }
        len = 4;
    } else if (strcmp(type_id, "DOUBLE8") == 0) {
        double d = strtod(input_str, NULL);
        uint64_t u;
        memcpy(&u, &d, sizeof(double));
        if (endian == HXD_ENDIAN_LITTLE) {
            for (int i = 0; i < 8; i++) bytes[i] = (uint8_t)((u >> (i * 8)) & 0xFF);
        } else {
            for (int i = 0; i < 8; i++) bytes[i] = (uint8_t)((u >> ((7 - i) * 8)) & 0xFF);
        }
        len = 8;
    } else if (strcmp(type_id, "HALF2") == 0) {
        float f = strtof(input_str, NULL);
        uint16_t h = encode_float16(f);
        if (endian == HXD_ENDIAN_LITTLE) { bytes[0] = h & 0xFF; bytes[1] = (h >> 8) & 0xFF; }
        else { bytes[0] = (h >> 8) & 0xFF; bytes[1] = h & 0xFF; }
        len = 2;
    } else if (strcmp(type_id, "ASCII_ANSI") == 0) {
        bytes[0] = (uint8_t)input_str[0];
        len = 1;
    } else if (strcmp(type_id, "IPV4") == 0) {
        unsigned int b0, b1, b2, b3;
        if (sscanf(input_str, "%u.%u.%u.%u", &b0, &b1, &b2, &b3) == 4) {
            bytes[0] = b0; bytes[1] = b1; bytes[2] = b2; bytes[3] = b3;
            len = 4;
        }
    } else {
        return false;
    }

    if (len > 0) {
        return pt_overwrite_bytes(pt, offset, bytes, len);
    }
    return false;
}
