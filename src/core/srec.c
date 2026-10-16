#include "srec.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    uint32_t address;
    uint32_t length;
    uint8_t *data;
} srec_block_t;

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

bool srec_is_srec(const char *path_or_content, bool is_path) {
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
    return (*p == 'S' && isdigit((unsigned char)*(p + 1)));
}

piece_table_t *srec_parse_string(const char *content) {
    if (!content) return pt_create();

    srec_block_t *blocks = NULL;
    size_t block_count = 0;
    size_t block_capacity = 0;

    const char *line = content;
    while (*line) {
        while (*line && isspace((unsigned char)*line)) line++;
        if (!*line) break;

        if (*line != 'S') {
            while (*line && *line != '\n') line++;
            continue;
        }

        char type = line[1];
        line += 2; // Skip 'S' and type

        size_t len = 0;
        while (line[len] && line[len] != '\r' && line[len] != '\n') len++;

        if (len < 4) {
            line += len;
            continue;
        }

        uint8_t count = 0;
        if (!parse_hex_byte(line, &count)) {
            line += len;
            continue;
        }

        int addr_len = 0;
        bool is_data = false;
        if (type == '1') { addr_len = 2; is_data = true; }
        else if (type == '2') { addr_len = 3; is_data = true; }
        else if (type == '3') { addr_len = 4; is_data = true; }
        else if (type == '0' || type == '5' || type == '6' || type == '7' || type == '8' || type == '9') {
            // Header, count, or termination record
            line += len;
            continue;
        } else {
            line += len;
            continue;
        }

        if (count < addr_len + 1) {
            line += len;
            continue;
        }

        uint32_t address = 0;
        uint8_t sum = count;
        bool valid = true;

        for (int i = 0; i < addr_len; i++) {
            uint8_t ab;
            if (!parse_hex_byte(line + 2 + i * 2, &ab)) {
                valid = false;
                break;
            }
            address = (address << 8) | ab;
            sum += ab;
        }

        if (!valid) {
            line += len;
            continue;
        }

        int data_len = count - addr_len - 1;
        uint8_t *data_buf = data_len > 0 ? malloc(data_len) : NULL;

        for (int i = 0; i < data_len; i++) {
            uint8_t db;
            if (!parse_hex_byte(line + 2 + (addr_len + i) * 2, &db)) {
                valid = false;
                break;
            }
            if (data_buf) data_buf[i] = db;
            sum += db;
        }

        uint8_t csum = 0;
        if (valid && parse_hex_byte(line + 2 + (addr_len + data_len) * 2, &csum)) {
            if ((uint8_t)(~sum) != csum) {
                valid = false;
            }
        } else {
            valid = false;
        }

        if (!valid) {
            if (data_buf) free(data_buf);
            line += len;
            continue;
        }

        if (is_data && data_len > 0 && data_buf) {
            bool merged = false;
            if (block_count > 0) {
                srec_block_t *prev = &blocks[block_count - 1];
                if (prev->address + prev->length == address) {
                    prev->data = realloc(prev->data, prev->length + data_len);
                    memcpy(prev->data + prev->length, data_buf, data_len);
                    prev->length += data_len;
                    free(data_buf);
                    data_buf = NULL;
                    merged = true;
                }
            }
            if (!merged) {
                if (block_count >= block_capacity) {
                    block_capacity = block_capacity ? block_capacity * 2 : 16;
                    blocks = realloc(blocks, block_capacity * sizeof(srec_block_t));
                }
                blocks[block_count].address = address;
                blocks[block_count].length = data_len;
                blocks[block_count].data = data_buf;
                block_count++;
            }
        } else if (data_buf) {
            free(data_buf);
        }

        line += len;
    }

    // Assemble piece table with gap preservation
    piece_table_t *pt = pt_create();
    hxd_offset_t current_cursor = 0;

    for (size_t i = 0; i < block_count; i++) {
        srec_block_t *b = &blocks[i];
        if (b->address > current_cursor) {
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

piece_table_t *srec_parse_file(const char *path) {
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

    piece_table_t *pt = srec_parse_string(buf);
    free(buf);
    return pt;
}

typedef struct {
    char        **lines;
    size_t        count;
    size_t        capacity;
    srec_format_t fmt;
    int           bpr;
    hxd_offset_t  range_start;
    hxd_offset_t  range_end;
} srec_export_ctx_t;

static void append_srec_line(srec_export_ctx_t *ctx, const char *line) {
    if (ctx->count >= ctx->capacity) {
        ctx->capacity = ctx->capacity ? ctx->capacity * 2 : 64;
        ctx->lines = realloc(ctx->lines, ctx->capacity * sizeof(char *));
    }
    ctx->lines[ctx->count++] = strdup(line);
}

static void srec_export_span(hxd_offset_t offset, const uint8_t *data, hxd_size_t length, bool is_gap, void *userdata) {
    srec_export_ctx_t *ctx = (srec_export_ctx_t *)userdata;
    if (is_gap) return; // Skip sparse gaps!

    hxd_offset_t span_end = offset + length;
    if (span_end <= ctx->range_start || offset >= ctx->range_end) return;

    hxd_offset_t start = (offset < ctx->range_start) ? ctx->range_start : offset;
    hxd_offset_t end = (span_end > ctx->range_end) ? ctx->range_end : span_end;
    const uint8_t *cur_data = data + (start - offset);
    hxd_size_t cur_len = end - start;
    uint32_t cur_addr = (uint32_t)start;

    int addr_bytes = (ctx->fmt == SREC_FMT_S19) ? 2 : ((ctx->fmt == SREC_FMT_S28) ? 3 : 4);
    char type_char = (ctx->fmt == SREC_FMT_S19) ? '1' : ((ctx->fmt == SREC_FMT_S28) ? '2' : '3');

    while (cur_len > 0) {
        int chunk = (int)((cur_len < (hxd_size_t)ctx->bpr) ? cur_len : (hxd_size_t)ctx->bpr);
        uint8_t count = (uint8_t)(addr_bytes + chunk + 1);
        uint8_t sum = count;

        char line[600];
        int pos = snprintf(line, sizeof(line), "S%c%02X", type_char, count);

        // Address bytes
        for (int i = addr_bytes - 1; i >= 0; i--) {
            uint8_t ab = (cur_addr >> (i * 8)) & 0xFF;
            sum += ab;
            pos += snprintf(line + pos, sizeof(line) - pos, "%02X", ab);
        }

        // Data bytes
        for (int i = 0; i < chunk; i++) {
            uint8_t db = cur_data[i];
            sum += db;
            pos += snprintf(line + pos, sizeof(line) - pos, "%02X", db);
        }

        uint8_t csum = (uint8_t)(~sum);
        snprintf(line + pos, sizeof(line) - pos, "%02X\r\n", csum);
        append_srec_line(ctx, line);

        cur_data += chunk;
        cur_addr += chunk;
        cur_len -= chunk;
    }
}

char *srec_export_string(const piece_table_t *pt, hxd_range_t range, srec_format_t fmt, const char *header, int bytes_per_record) {
    if (!pt) return strdup("");
    if (bytes_per_record <= 0 || bytes_per_record > 255) bytes_per_record = 16;

    srec_export_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.fmt = fmt;
    ctx.bpr = bytes_per_record;
    ctx.range_start = range.length > 0 ? range.start : 0;
    ctx.range_end = range.length > 0 ? range.start + range.length : pt_get_size(pt);

    // Optional S0 header
    if (header && strlen(header) > 0) {
        size_t hlen = strlen(header);
        if (hlen > 32) hlen = 32;
        uint8_t count = (uint8_t)(2 + hlen + 1);
        uint8_t sum = count;
        char line[200];
        int pos = snprintf(line, sizeof(line), "S0%02X0000", count);
        for (size_t i = 0; i < hlen; i++) {
            uint8_t b = (uint8_t)header[i];
            sum += b;
            pos += snprintf(line + pos, sizeof(line) - pos, "%02X", b);
        }
        uint8_t csum = (uint8_t)(~sum);
        snprintf(line + pos, sizeof(line) - pos, "%02X\r\n", csum);
        append_srec_line(&ctx, line);
    }

    pt_iterate_spans(pt, srec_export_span, &ctx);

    // Termination record (S9, S8, or S7)
    if (fmt == SREC_FMT_S19) {
        append_srec_line(&ctx, "S9030000FC\r\n");
    } else if (fmt == SREC_FMT_S28) {
        append_srec_line(&ctx, "S804000000FB\r\n");
    } else {
        append_srec_line(&ctx, "S70500000000FA\r\n");
    }

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

bool srec_export_file(const piece_table_t *pt, const char *path, hxd_range_t range, srec_format_t fmt, const char *header, int bytes_per_record) {
    if (!path) return false;
    char *text = srec_export_string(pt, range, fmt, header, bytes_per_record);
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
