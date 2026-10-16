#include "export.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char b64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *export_to_data_url(const piece_table_t *pt, hxd_range_t range) {
    if (!pt) return strdup("");

    hxd_offset_t start = range.length > 0 ? range.start : 0;
    hxd_offset_t len = range.length > 0 ? range.length : pt_get_size(pt);
    if (start >= pt_get_size(pt)) return strdup("");
    if (start + len > pt_get_size(pt)) len = pt_get_size(pt) - start;

    size_t b64_len = ((len + 2) / 3) * 4;
    const char *prefix = "data:application/octet-stream;base64,";
    size_t total_size = strlen(prefix) + b64_len + 1;

    char *result = malloc(total_size);
    strcpy(result, prefix);
    char *out_p = result + strlen(prefix);

    uint8_t buffer[3072];
    hxd_offset_t processed = 0;
    uint8_t triple[3];
    int triple_idx = 0;

    while (processed < len) {
        hxd_size_t chunk = sizeof(buffer);
        if (processed + chunk > len) chunk = len - processed;
        pt_read_bytes(pt, start + processed, buffer, chunk, NULL);

        for (hxd_size_t i = 0; i < chunk; i++) {
            triple[triple_idx++] = buffer[i];
            if (triple_idx == 3) {
                *out_p++ = b64_chars[(triple[0] >> 2) & 0x3F];
                *out_p++ = b64_chars[((triple[0] & 0x03) << 4) | ((triple[1] >> 4) & 0x0F)];
                *out_p++ = b64_chars[((triple[1] & 0x0F) << 2) | ((triple[2] >> 6) & 0x03)];
                *out_p++ = b64_chars[triple[2] & 0x3F];
                triple_idx = 0;
            }
        }
        processed += chunk;
    }

    if (triple_idx == 1) {
        *out_p++ = b64_chars[(triple[0] >> 2) & 0x3F];
        *out_p++ = b64_chars[(triple[0] & 0x03) << 4];
        *out_p++ = '=';
        *out_p++ = '=';
    } else if (triple_idx == 2) {
        *out_p++ = b64_chars[(triple[0] >> 2) & 0x3F];
        *out_p++ = b64_chars[((triple[0] & 0x03) << 4) | ((triple[1] >> 4) & 0x0F)];
        *out_p++ = b64_chars[(triple[1] & 0x0F) << 2];
        *out_p++ = '=';
    }
    *out_p = '\0';

    return result;
}

char *export_to_snippet(const piece_table_t *pt, hxd_range_t range, export_snippet_lang_t lang, int bytes_per_line) {
    if (!pt) return strdup("");
    if (bytes_per_line <= 0 || bytes_per_line > 64) bytes_per_line = 16;

    hxd_offset_t start = range.length > 0 ? range.start : 0;
    hxd_offset_t len = range.length > 0 ? range.length : pt_get_size(pt);
    if (start >= pt_get_size(pt)) return strdup("");
    if (start + len > pt_get_size(pt)) len = pt_get_size(pt) - start;

    size_t est_size = len * 6 + 1024;
    char *out = malloc(est_size);
    out[0] = '\0';
    size_t out_pos = 0;

    switch (lang) {
        case EXPORT_SNIPPET_C:
            out_pos += snprintf(out + out_pos, est_size - out_pos, "const unsigned char data[%llu] = {\n", (unsigned long long)len);
            break;
        case EXPORT_SNIPPET_PYTHON:
            out_pos += snprintf(out + out_pos, est_size - out_pos, "data = bytes([\n");
            break;
        case EXPORT_SNIPPET_JS:
            out_pos += snprintf(out + out_pos, est_size - out_pos, "const data = new Uint8Array([\n");
            break;
        case EXPORT_SNIPPET_GO:
            out_pos += snprintf(out + out_pos, est_size - out_pos, "var data = []byte{\n");
            break;
        case EXPORT_SNIPPET_RUST:
            out_pos += snprintf(out + out_pos, est_size - out_pos, "const DATA: [u8; %llu] = [\n", (unsigned long long)len);
            break;
        case EXPORT_SNIPPET_CSHARP:
            out_pos += snprintf(out + out_pos, est_size - out_pos, "byte[] data = new byte[] {\n");
            break;
        case EXPORT_SNIPPET_HEX_DUMP:
            break;
    }

    uint8_t line_buf[64];
    hxd_offset_t processed = 0;

    while (processed < len) {
        int chunk = (int)((len - processed < (hxd_size_t)bytes_per_line) ? (len - processed) : (hxd_size_t)bytes_per_line);
        pt_read_bytes(pt, start + processed, line_buf, chunk, NULL);

        if (lang == EXPORT_SNIPPET_HEX_DUMP) {
            out_pos += snprintf(out + out_pos, est_size - out_pos, "%08llX  ", (unsigned long long)(start + processed));
            for (int i = 0; i < bytes_per_line; i++) {
                if (i < chunk) {
                    out_pos += snprintf(out + out_pos, est_size - out_pos, "%02X ", line_buf[i]);
                } else {
                    out_pos += snprintf(out + out_pos, est_size - out_pos, "   ");
                }
                if (i == 7) out_pos += snprintf(out + out_pos, est_size - out_pos, " ");
            }
            out_pos += snprintf(out + out_pos, est_size - out_pos, " |");
            for (int i = 0; i < chunk; i++) {
                char c = (line_buf[i] >= 32 && line_buf[i] <= 126) ? (char)line_buf[i] : '.';
                out_pos += snprintf(out + out_pos, est_size - out_pos, "%c", c);
            }
            out_pos += snprintf(out + out_pos, est_size - out_pos, "|\n");
        } else {
            out_pos += snprintf(out + out_pos, est_size - out_pos, "    ");
            for (int i = 0; i < chunk; i++) {
                out_pos += snprintf(out + out_pos, est_size - out_pos, "0x%02X", line_buf[i]);
                if (processed + i + 1 < len) {
                    out_pos += snprintf(out + out_pos, est_size - out_pos, ", ");
                }
            }
            out_pos += snprintf(out + out_pos, est_size - out_pos, "\n");
        }

        processed += chunk;
        if (out_pos + 1024 > est_size) {
            est_size *= 2;
            out = realloc(out, est_size);
        }
    }

    switch (lang) {
        case EXPORT_SNIPPET_C:
        case EXPORT_SNIPPET_CSHARP:
            out_pos += snprintf(out + out_pos, est_size - out_pos, "};\n");
            break;
        case EXPORT_SNIPPET_PYTHON:
        case EXPORT_SNIPPET_JS:
            out_pos += snprintf(out + out_pos, est_size - out_pos, "]);\n");
            break;
        case EXPORT_SNIPPET_GO:
            out_pos += snprintf(out + out_pos, est_size - out_pos, "}\n");
            break;
        case EXPORT_SNIPPET_RUST:
            out_pos += snprintf(out + out_pos, est_size - out_pos, "];\n");
            break;
        case EXPORT_SNIPPET_HEX_DUMP:
            break;
    }

    return out;
}

bool export_to_binary_file(const piece_table_t *pt, const char *path, hxd_range_t range) {
    if (!pt || !path) return false;

    FILE *f = fopen(path, "wb");
    if (!f) return false;

    hxd_offset_t start = range.length > 0 ? range.start : 0;
    hxd_offset_t len = range.length > 0 ? range.length : pt_get_size(pt);
    if (start >= pt_get_size(pt)) {
        fclose(f);
        return true;
    }
    if (start + len > pt_get_size(pt)) len = pt_get_size(pt) - start;

    uint8_t buffer[65536];
    hxd_offset_t processed = 0;

    while (processed < len) {
        hxd_size_t chunk = sizeof(buffer);
        if (processed + chunk > len) chunk = len - processed;
        pt_read_bytes(pt, start + processed, buffer, chunk, NULL);
        if (fwrite(buffer, 1, chunk, f) != chunk) {
            fclose(f);
            return false;
        }
        processed += chunk;
    }

    fclose(f);
    return true;
}
