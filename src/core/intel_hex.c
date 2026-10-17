#include "intel_hex.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    uint32_t address;
    uint32_t length;
    uint8_t *data;
} hex_block_t;

static int hex_char_to_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool parse_hex_byte(const char *s, uint8_t *out) {
    int h1 = hex_char_to_val(s[0]);
    int h2 = hex_char_to_val(s[1]);
    if (h1 < 0 || h2 < 0) return false;
    *out = (uint8_t)((h1 << 4) | h2);
    return true;
}

bool ihex_is_intel_hex(const char *path_or_content, bool is_path) {
    if (!path_or_content) return false;
    char first_chars[256];
    if (is_path) {
        FILE *f = fopen(path_or_content, "r");
        if (!f) return false;
        size_t r = fread(first_chars, 1, sizeof(first_chars) - 1, f);
        fclose(f);
        if (r == 0) return false;
        first_chars[r] = '\0';
    } else {
        strncpy(first_chars, path_or_content, sizeof(first_chars) - 1);
        first_chars[sizeof(first_chars) - 1] = '\0';
    }

    const char *p = first_chars;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != ':') return false;
    p++;
    int hex_digits = 0;
    while (*p && isxdigit((unsigned char)*p) && hex_digits < 10) {
        p++;
        hex_digits++;
    }
    return (hex_digits >= 10);
}

piece_table_t *ihex_parse_string(const char *content) {
    if (!content) return pt_create();

    uint32_t base_address = 0;
    hex_block_t *blocks = NULL;
    size_t block_count = 0;
    size_t block_capacity = 0;

    const char *line = content;
    while (*line) {
        while (*line && isspace((unsigned char)*line)) line++;
        if (!*line) break;

        if (*line != ':') {
            while (*line && *line != '\n') line++;
            continue;
        }

        line++; // Skip ':'
        size_t len = 0;
        while (line[len] && line[len] != '\r' && line[len] != '\n') len++;

        if (len < 10) { // Minimum :LLAAAATTCC = 1 + 10 = 11 chars total (10 after ':')
            line += len;
            continue;
        }

        uint8_t byte_count, rectype, csum;
        uint8_t addr_hi, addr_lo;
        if (!parse_hex_byte(line, &byte_count) ||
            !parse_hex_byte(line + 2, &addr_hi) ||
            !parse_hex_byte(line + 4, &addr_lo) ||
            !parse_hex_byte(line + 6, &rectype)) {
            line += len;
            continue;
        }

        uint16_t record_addr = (addr_hi << 8) | addr_lo;
        uint32_t full_addr = base_address + record_addr;

        // Verify checksum
        uint8_t running_sum = byte_count + addr_hi + addr_lo + rectype;
        uint8_t *data_buf = NULL;
        if (byte_count > 0) {
            data_buf = malloc(byte_count);
        }

        bool valid = true;
        for (int i = 0; i < byte_count; i++) {
            uint8_t d;
            if (!parse_hex_byte(line + 8 + i * 2, &d)) {
                valid = false;
                break;
            }
            if (data_buf) data_buf[i] = d;
            running_sum += d;
        }

        if (valid && parse_hex_byte(line + 8 + byte_count * 2, &csum)) {
            running_sum += csum;
            if (running_sum != 0) {
                valid = false; // Checksum error
            }
        } else {
            valid = false;
        }

        if (!valid) {
            if (data_buf) free(data_buf);
            line += len;
            continue;
        }

        if (rectype == 0x00) { // Data Record
            if (byte_count > 0 && data_buf) {
                // Append or merge with previous block
                bool merged = false;
                if (block_count > 0) {
                    hex_block_t *prev = &blocks[block_count - 1];
                    if (prev->address + prev->length == full_addr) {
                        prev->data = realloc(prev->data, prev->length + byte_count);
                        memcpy(prev->data + prev->length, data_buf, byte_count);
                        prev->length += byte_count;
                        free(data_buf);
                        data_buf = NULL;
                        merged = true;
                    }
                }
                if (!merged) {
                    if (block_count >= block_capacity) {
                        block_capacity = block_capacity ? block_capacity * 2 : 16;
                        blocks = realloc(blocks, block_capacity * sizeof(hex_block_t));
                    }
                    blocks[block_count].address = full_addr;
                    blocks[block_count].length = byte_count;
                    blocks[block_count].data = data_buf;
                    block_count++;
                }
            }
        } else if (rectype == 0x01) { // End of File Record
            if (data_buf) free(data_buf);
            break;
        } else if (rectype == 0x02) { // Extended Segment Address Record
            if (byte_count == 2 && data_buf) {
                base_address = (((uint32_t)data_buf[0] << 8) | data_buf[1]) << 4;
            }
            if (data_buf) free(data_buf);
        } else if (rectype == 0x04) { // Extended Linear Address Record
            if (byte_count == 2 && data_buf) {
                base_address = (((uint32_t)data_buf[0] << 8) | data_buf[1]) << 16;
            }
            if (data_buf) free(data_buf);
        } else {
            if (data_buf) free(data_buf);
        }

        line += len;
    }

    // Now assemble into Piece Table with gap preservation
    piece_table_t *pt = pt_create();
    hxd_offset_t current_cursor = 0;

    for (size_t i = 0; i < block_count; i++) {
        hex_block_t *b = &blocks[i];
        if (b->address > current_cursor) {
            // Gap before this block!
            hxd_size_t gap_len = b->address - current_cursor;
            pt_insert_gap(pt, current_cursor, gap_len);
            current_cursor = b->address;
        }
        pt_insert_bytes(pt, current_cursor, b->data, b->length);
        current_cursor += b->length;
        free(b->data);
    }
    free(blocks);

    return pt;
}

piece_table_t *ihex_parse_file(const char *path) {
    if (!path) return pt_create();
    FILE *f = fopen(path, "rb");
    if (!f) return pt_create();

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(sz + 1);
    size_t r = fread(buf, 1, sz, f);
    buf[r] = '\0';
    fclose(f);

    piece_table_t *pt = ihex_parse_string(buf);
    free(buf);
    return pt;
}

typedef struct {
    char        **lines;
    size_t        count;
    size_t        capacity;
    uint32_t      current_high_addr;
    ihex_format_t fmt;
    int           bpr;
    hxd_offset_t  range_start;
    hxd_offset_t  range_end;
} ihex_export_ctx_t;

static void append_ihex_line(ihex_export_ctx_t *ctx, const char *line) {
    if (ctx->count >= ctx->capacity) {
        ctx->capacity = ctx->capacity ? ctx->capacity * 2 : 64;
        ctx->lines = realloc(ctx->lines, ctx->capacity * sizeof(char *));
    }
    ctx->lines[ctx->count++] = strdup(line);
}

static void ihex_export_span(hxd_offset_t offset, const uint8_t *data, hxd_size_t length, bool is_gap, void *userdata) {
    ihex_export_ctx_t *ctx = (ihex_export_ctx_t *)userdata;
    if (is_gap) return; // Skip sparse gaps!

    hxd_offset_t span_end = offset + length;
    if (span_end <= ctx->range_start || offset >= ctx->range_end) return;

    hxd_offset_t start = (offset < ctx->range_start) ? ctx->range_start : offset;
    hxd_offset_t end = (span_end > ctx->range_end) ? ctx->range_end : span_end;
    const uint8_t *cur_data = data + (start - offset);
    hxd_size_t cur_len = end - start;
    uint32_t cur_addr = (uint32_t)start;

    while (cur_len > 0) {
        // Check if upper 16-bit address changed for I32HEX
        if (ctx->fmt == IHEX_FMT_I32HEX) {
            uint32_t high = cur_addr >> 16;
            if (high != ctx->current_high_addr || ctx->count == 0) {
                ctx->current_high_addr = high;
                uint8_t hi_byte = (high >> 8) & 0xFF;
                uint8_t lo_byte = high & 0xFF;
                uint8_t csum = 0 - (2 + 0 + 0 + 4 + hi_byte + lo_byte);
                char line[32];
                snprintf(line, sizeof(line), ":02000004%02X%02X%02X\r\n", hi_byte, lo_byte, csum);
                append_ihex_line(ctx, line);
            }
        }

        int chunk = (int)((cur_len < (hxd_size_t)ctx->bpr) ? cur_len : (hxd_size_t)ctx->bpr);
        uint16_t low_addr = cur_addr & 0xFFFF;

        uint8_t sum = chunk + ((low_addr >> 8) & 0xFF) + (low_addr & 0xFF) + 0x00;
        char line[600];
        int pos = snprintf(line, sizeof(line), ":%02X%04X00", chunk, low_addr);
        for (int i = 0; i < chunk; i++) {
            uint8_t b = cur_data[i];
            sum += b;
            pos += snprintf(line + pos, sizeof(line) - pos, "%02X", b);
        }
        uint8_t csum = (uint8_t)(0 - sum);
        snprintf(line + pos, sizeof(line) - pos, "%02X\r\n", csum);
        append_ihex_line(ctx, line);

        cur_data += chunk;
        cur_addr += chunk;
        cur_len -= chunk;
    }
}

char *ihex_export_string(const piece_table_t *pt, hxd_range_t range, ihex_format_t fmt, int bytes_per_record) {
    if (!pt) return strdup("");
    if (bytes_per_record <= 0 || bytes_per_record > 255) bytes_per_record = 16;

    ihex_export_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.fmt = fmt;
    ctx.bpr = bytes_per_record;
    ctx.current_high_addr = 0xFFFFFFFF; // sentinel
    ctx.range_start = range.length > 0 ? range.start : 0;
    ctx.range_end = range.length > 0 ? range.start + range.length : pt_get_size(pt);

    pt_iterate_spans(pt, ihex_export_span, &ctx);

    // End of file record :00000001FF
    append_ihex_line(&ctx, ":00000001FF\r\n");

    size_t total_len = 0;
    for (size_t i = 0; i < ctx.count; i++) {
        total_len += strlen(ctx.lines[i]);
    }

    char *result = malloc(total_len + 1);
    result[0] = '\0';
    for (size_t i = 0; i < ctx.count; i++) {
        strcat(result, ctx.lines[i]);
        free(ctx.lines[i]);
    }
    free(ctx.lines);

    return result;
}

bool ihex_export_file(const piece_table_t *pt, const char *path, hxd_range_t range, ihex_format_t fmt, int bytes_per_record) {
    if (!path) return false;
    char *text = ihex_export_string(pt, range, fmt, bytes_per_record);
    if (!text) return false;
    FILE *f = fopen(path, "wb");
    if (!f) {
        free(text);
        return false;
    }
    fwrite(text, 1, strlen(text), f);
    fclose(f);
    free(text);
    return true;
}
