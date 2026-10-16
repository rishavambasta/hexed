#include "search.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>

static int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool search_parse_pattern(const char *query, search_type_t type, hxd_endianness_t endian, uint8_t **out_pattern, size_t *out_len) {
    return search_parse_pattern_ex(query, type, endian, out_pattern, NULL, out_len);
}

bool search_parse_pattern_ex(const char *query, search_type_t type, hxd_endianness_t endian, uint8_t **out_pattern, uint8_t **out_mask, size_t *out_len) {
    if (!query || !out_pattern || !out_len) return false;
    *out_pattern = NULL;
    if (out_mask) *out_mask = NULL;
    *out_len = 0;

    switch (type) {
        case SEARCH_TYPE_REGEX: {
            size_t qlen = strlen(query);
            if (qlen == 0) return false;
            *out_pattern = (uint8_t *)strdup(query);
            *out_len = qlen;
            return true;
        }

        case SEARCH_TYPE_WILDCARD: {
            size_t qlen = strlen(query);
            uint8_t *pat = malloc(qlen + 1);
            uint8_t *mask = malloc(qlen + 1);
            size_t count = 0;
            const char *p = query;

            while (*p) {
                while (*p && isspace((unsigned char)*p)) p++;
                if (!*p) break;

                if (*p == '?') {
                    // Wildcard byte
                    pat[count] = 0x00;
                    mask[count] = 0x00;
                    count++;
                    p++;
                    if (*p == '?') p++;
                } else if (isxdigit((unsigned char)*p)) {
                    int h1 = hex_val(*p++);
                    int h2 = 0;
                    if (isxdigit((unsigned char)*p)) {
                        h2 = hex_val(*p++);
                    }
                    pat[count] = (uint8_t)((h1 << 4) | h2);
                    mask[count] = 0xFF;
                    count++;
                } else {
                    p++;
                }
            }

            if (count == 0) {
                free(pat);
                free(mask);
                return false;
            }

            *out_pattern = pat;
            if (out_mask) *out_mask = mask;
            else free(mask);
            *out_len = count;
            return true;
        }
        case SEARCH_TYPE_HEX: {
            // Strip whitespace and parse hex pairs
            size_t qlen = strlen(query);
            uint8_t *buf = malloc(qlen);
            size_t b_idx = 0;
            int high_nibble = -1;

            for (size_t i = 0; i < qlen; i++) {
                if (isspace((unsigned char)query[i])) continue;
                int v = hex_val(query[i]);
                if (v < 0) {
                    free(buf);
                    return false;
                }
                if (high_nibble == -1) {
                    high_nibble = v;
                } else {
                    buf[b_idx++] = (uint8_t)((high_nibble << 4) | v);
                    high_nibble = -1;
                }
            }
            if (high_nibble != -1) { // Odd number of hex characters, pad lowest
                buf[b_idx++] = (uint8_t)(high_nibble << 4);
            }
            if (b_idx == 0) {
                free(buf);
                return false;
            }
            *out_pattern = buf;
            *out_len = b_idx;
            return true;
        }

        case SEARCH_TYPE_ASCII: {
            size_t len = strlen(query);
            if (len == 0) return false;
            *out_pattern = (uint8_t *)strdup(query);
            *out_len = len;
            return true;
        }

        case SEARCH_TYPE_UTF16: {
            size_t len = strlen(query);
            if (len == 0) return false;
            uint8_t *buf = malloc(len * 2);
            for (size_t i = 0; i < len; i++) {
                uint16_t c = (uint8_t)query[i];
                if (endian == HXD_ENDIAN_LITTLE) {
                    buf[i * 2]     = (uint8_t)(c & 0xFF);
                    buf[i * 2 + 1] = (uint8_t)((c >> 8) & 0xFF);
                } else {
                    buf[i * 2]     = (uint8_t)((c >> 8) & 0xFF);
                    buf[i * 2 + 1] = (uint8_t)(c & 0xFF);
                }
            }
            *out_pattern = buf;
            *out_len = len * 2;
            return true;
        }

        case SEARCH_TYPE_INT8:
        case SEARCH_TYPE_UINT8: {
            uint8_t *buf = malloc(1);
            buf[0] = (uint8_t)strtoul(query, NULL, 0);
            *out_pattern = buf;
            *out_len = 1;
            return true;
        }

        case SEARCH_TYPE_INT16:
        case SEARCH_TYPE_UINT16: {
            uint16_t v = (uint16_t)strtoul(query, NULL, 0);
            uint8_t *buf = malloc(2);
            if (endian == HXD_ENDIAN_LITTLE) {
                buf[0] = v & 0xFF; buf[1] = (v >> 8) & 0xFF;
            } else {
                buf[0] = (v >> 8) & 0xFF; buf[1] = v & 0xFF;
            }
            *out_pattern = buf;
            *out_len = 2;
            return true;
        }

        case SEARCH_TYPE_INT32:
        case SEARCH_TYPE_UINT32: {
            uint32_t v = (uint32_t)strtoull(query, NULL, 0);
            uint8_t *buf = malloc(4);
            if (endian == HXD_ENDIAN_LITTLE) {
                buf[0] = v & 0xFF; buf[1] = (v >> 8) & 0xFF;
                buf[2] = (v >> 16) & 0xFF; buf[3] = (v >> 24) & 0xFF;
            } else {
                buf[0] = (v >> 24) & 0xFF; buf[1] = (v >> 16) & 0xFF;
                buf[2] = (v >> 8) & 0xFF; buf[3] = v & 0xFF;
            }
            *out_pattern = buf;
            *out_len = 4;
            return true;
        }

        case SEARCH_TYPE_INT64:
        case SEARCH_TYPE_UINT64: {
            uint64_t v = (uint64_t)strtoull(query, NULL, 0);
            uint8_t *buf = malloc(8);
            if (endian == HXD_ENDIAN_LITTLE) {
                for (int i = 0; i < 8; i++) buf[i] = (uint8_t)((v >> (i * 8)) & 0xFF);
            } else {
                for (int i = 0; i < 8; i++) buf[i] = (uint8_t)((v >> ((7 - i) * 8)) & 0xFF);
            }
            *out_pattern = buf;
            *out_len = 8;
            return true;
        }

        case SEARCH_TYPE_FLOAT32: {
            float f = strtof(query, NULL);
            uint32_t v;
            memcpy(&v, &f, sizeof(float));
            uint8_t *buf = malloc(4);
            if (endian == HXD_ENDIAN_LITTLE) {
                buf[0] = v & 0xFF; buf[1] = (v >> 8) & 0xFF;
                buf[2] = (v >> 16) & 0xFF; buf[3] = (v >> 24) & 0xFF;
            } else {
                buf[0] = (v >> 24) & 0xFF; buf[1] = (v >> 16) & 0xFF;
                buf[2] = (v >> 8) & 0xFF; buf[3] = v & 0xFF;
            }
            *out_pattern = buf;
            *out_len = 4;
            return true;
        }

        case SEARCH_TYPE_FLOAT64: {
            double d = strtod(query, NULL);
            uint64_t v;
            memcpy(&v, &d, sizeof(double));
            uint8_t *buf = malloc(8);
            if (endian == HXD_ENDIAN_LITTLE) {
                for (int i = 0; i < 8; i++) buf[i] = (uint8_t)((v >> (i * 8)) & 0xFF);
            } else {
                for (int i = 0; i < 8; i++) buf[i] = (uint8_t)((v >> ((7 - i) * 8)) & 0xFF);
            }
            *out_pattern = buf;
            *out_len = 8;
            return true;
        }

        case SEARCH_TYPE_BINARY: {
            size_t qlen = strlen(query);
            uint8_t *buf = malloc((qlen + 7) / 8 + 1);
            size_t b_idx = 0;
            uint8_t curr = 0;
            int bit_count = 0;

            for (size_t i = 0; i < qlen; i++) {
                if (query[i] == '0' || query[i] == '1') {
                    curr = (curr << 1) | (query[i] - '0');
                    bit_count++;
                    if (bit_count == 8) {
                        buf[b_idx++] = curr;
                        curr = 0;
                        bit_count = 0;
                    }
                }
            }
            if (bit_count > 0) {
                curr <<= (8 - bit_count);
                buf[b_idx++] = curr;
            }
            if (b_idx == 0) {
                free(buf);
                return false;
            }
            *out_pattern = buf;
            *out_len = b_idx;
            return true;
        }
    }

    return false;
}

static bool match_at(const piece_table_t *pt, hxd_offset_t off, const uint8_t *pat, const uint8_t *mask, size_t pat_len, bool match_case) {
    uint8_t buf[256];
    size_t rem = pat_len;
    hxd_offset_t curr_off = off;
    size_t pat_off = 0;

    while (rem > 0) {
        size_t chunk = (rem < sizeof(buf)) ? rem : sizeof(buf);
        hxd_size_t read_bytes = pt_read_bytes(pt, curr_off, buf, chunk, NULL);
        if (read_bytes < chunk) return false;

        for (size_t i = 0; i < chunk; i++) {
            uint8_t b1 = buf[i];
            uint8_t b2 = pat[pat_off + i];
            uint8_t m  = mask ? mask[pat_off + i] : 0xFF;

            if (m == 0x00) continue; // Wildcard matches any byte

            if (!match_case) {
                if (tolower((unsigned char)(b1 & m)) != tolower((unsigned char)(b2 & m))) return false;
            } else {
                if ((b1 & m) != (b2 & m)) return false;
            }
        }

        rem -= chunk;
        curr_off += chunk;
        pat_off += chunk;
    }
    return true;
}

hxd_offset_t search_find_regex(const piece_table_t *pt, hxd_offset_t start_offset, const char *regex_str, bool backward, bool match_case, size_t *out_len) {
    if (!pt || !regex_str || strlen(regex_str) == 0) return HXD_OFFSET_INVALID;
    hxd_size_t doc_size = pt_get_size(pt);
    if (doc_size == 0) return HXD_OFFSET_INVALID;

    regex_t re;
    int cflags = REG_EXTENDED | (match_case ? 0 : REG_ICASE);
    if (regcomp(&re, regex_str, cflags) != 0) return HXD_OFFSET_INVALID;

    size_t scan_size = (doc_size < 10485760) ? (size_t)doc_size : 10485760;
    char *buf = malloc(scan_size + 1);
    if (!buf) {
        regfree(&re);
        return HXD_OFFSET_INVALID;
    }

    pt_read_bytes(pt, 0, (uint8_t *)buf, scan_size, NULL);
    buf[scan_size] = '\0';

    regmatch_t pmatch[1];
    hxd_offset_t found = HXD_OFFSET_INVALID;

    if (!backward) {
        size_t search_start = (start_offset < scan_size) ? (size_t)start_offset : scan_size;
        if (search_start < scan_size) {
            if (regexec(&re, buf + search_start, 1, pmatch, 0) == 0) {
                found = search_start + pmatch[0].rm_so;
                if (out_len) *out_len = pmatch[0].rm_eo - pmatch[0].rm_so;
            }
        }
    } else {
        size_t cur_pos = 0;
        hxd_offset_t last_match = HXD_OFFSET_INVALID;
        size_t last_len = 0;
        while (cur_pos < scan_size && cur_pos <= start_offset) {
            if (regexec(&re, buf + cur_pos, 1, pmatch, 0) == 0) {
                hxd_offset_t m = cur_pos + pmatch[0].rm_so;
                if (m < start_offset) {
                    last_match = m;
                    last_len = pmatch[0].rm_eo - pmatch[0].rm_so;
                    cur_pos = m + (last_len > 0 ? 1 : 1);
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        if (last_match != HXD_OFFSET_INVALID) {
            found = last_match;
            if (out_len) *out_len = last_len;
        }
    }

    free(buf);
    regfree(&re);
    return found;
}

search_results_t *search_find_all_regex(const piece_table_t *pt, const char *regex_str, bool match_case) {
    search_results_t *res = calloc(1, sizeof(search_results_t));
    if (!pt || !regex_str || strlen(regex_str) == 0) return res;

    hxd_size_t doc_size = pt_get_size(pt);
    if (doc_size == 0) return res;

    regex_t re;
    int cflags = REG_EXTENDED | (match_case ? 0 : REG_ICASE);
    if (regcomp(&re, regex_str, cflags) != 0) return res;

    size_t scan_size = (doc_size < 10485760) ? (size_t)doc_size : 10485760;
    char *buf = malloc(scan_size + 1);
    if (!buf) {
        regfree(&re);
        return res;
    }

    pt_read_bytes(pt, 0, (uint8_t *)buf, scan_size, NULL);
    buf[scan_size] = '\0';

    regmatch_t pmatch[1];
    size_t cur = 0;
    while (cur < scan_size) {
        if (regexec(&re, buf + cur, 1, pmatch, 0) == 0) {
            hxd_offset_t off = cur + pmatch[0].rm_so;
            if (res->count >= res->capacity) {
                res->capacity = res->capacity ? res->capacity * 2 : 16;
                res->offsets = realloc(res->offsets, res->capacity * sizeof(hxd_offset_t));
            }
            res->offsets[res->count++] = off;
            size_t mlen = pmatch[0].rm_eo - pmatch[0].rm_so;
            cur = off + (mlen > 0 ? mlen : 1);
        } else {
            break;
        }
    }

    free(buf);
    regfree(&re);
    return res;
}

hxd_offset_t search_find(const piece_table_t *pt, hxd_offset_t start_offset, const uint8_t *pattern, size_t pattern_len, const search_params_t *params) {
    if (!pt) return HXD_OFFSET_INVALID;

    if (params && params->type == SEARCH_TYPE_REGEX) {
        const char *rstr = params->regex_query ? params->regex_query : (const char *)pattern;
        bool backward = (params->direction == SEARCH_DIR_BACKWARD);
        size_t mlen = 0;
        hxd_offset_t found = search_find_regex(pt, start_offset, rstr, backward, params->match_case, &mlen);
        if (found != HXD_OFFSET_INVALID && params) {
            ((search_params_t *)params)->matched_len = mlen;
        }
        return found;
    }

    if (!pattern || pattern_len == 0 || pt_get_size(pt) < pattern_len) {
        return HXD_OFFSET_INVALID;
    }

    hxd_offset_t doc_size = pt_get_size(pt);
    hxd_offset_t range_start = 0;
    hxd_offset_t range_end = doc_size;

    if (params && params->scope.length > 0) {
        range_start = params->scope.start;
        range_end = params->scope.start + params->scope.length;
        if (range_end > doc_size) range_end = doc_size;
    }

    if (range_end < pattern_len || range_start > range_end - pattern_len) {
        return HXD_OFFSET_INVALID;
    }

    bool backward = (params && params->direction == SEARCH_DIR_BACKWARD);
    bool match_case = params ? params->match_case : true;
    const uint8_t *mask = params ? params->wildcard_mask : NULL;

    if (!backward) {
        hxd_offset_t cur = (start_offset < range_start) ? range_start : start_offset;
        while (cur + pattern_len <= range_end) {
            if (match_at(pt, cur, pattern, mask, pattern_len, match_case)) {
                return cur;
            }
            cur++;
        }
    } else {
        hxd_offset_t cur = (start_offset > range_end - pattern_len) ? (range_end - pattern_len) : start_offset;
        while (cur >= range_start) {
            if (match_at(pt, cur, pattern, mask, pattern_len, match_case)) {
                return cur;
            }
            if (cur == 0) break;
            cur--;
        }
    }

    return HXD_OFFSET_INVALID;
}

search_results_t *search_find_all(const piece_table_t *pt, const uint8_t *pattern, size_t pattern_len, const search_params_t *params) {
    if (params && params->type == SEARCH_TYPE_REGEX) {
        const char *rstr = params->regex_query ? params->regex_query : (const char *)pattern;
        return search_find_all_regex(pt, rstr, params->match_case);
    }

    search_results_t *res = calloc(1, sizeof(search_results_t));
    res->capacity = 32;
    res->offsets = malloc(res->capacity * sizeof(hxd_offset_t));

    if (!pt || !pattern || pattern_len == 0) return res;

    hxd_offset_t cur = 0;
    search_params_t p;
    if (params) p = *params;
    else memset(&p, 0, sizeof(p));
    p.direction = SEARCH_DIR_FORWARD;

    while (cur < pt_get_size(pt)) {
        hxd_offset_t found = search_find(pt, cur, pattern, pattern_len, &p);
        if (found == HXD_OFFSET_INVALID) break;

        if (res->count >= res->capacity) {
            res->capacity *= 2;
            res->offsets = realloc(res->offsets, res->capacity * sizeof(hxd_offset_t));
        }
        res->offsets[res->count++] = found;
        cur = found + pattern_len; // advance past match
    }

    return res;
}

void search_results_free(search_results_t *res) {
    if (!res) return;
    free(res->offsets);
    free(res);
}

bool search_replace(piece_table_t *pt, hxd_offset_t match_offset, size_t match_len, const uint8_t *replace_data, size_t replace_len) {
    if (!pt) return false;
    if (match_len > 0) {
        pt_delete_bytes(pt, match_offset, match_len);
    }
    if (replace_len > 0 && replace_data) {
        pt_insert_bytes(pt, match_offset, replace_data, replace_len);
    }
    return true;
}

size_t search_replace_all(piece_table_t *pt, const uint8_t *find_pat, size_t find_len, const uint8_t *rep_data, size_t rep_len, const search_params_t *params) {
    if (!pt || !find_pat || find_len == 0) return 0;

    size_t replacements = 0;
    hxd_offset_t cur = (params && params->scope.length > 0) ? params->scope.start : 0;

    while (cur < pt_get_size(pt)) {
        hxd_offset_t found = search_find(pt, cur, find_pat, find_len, params);
        if (found == HXD_OFFSET_INVALID) break;

        search_replace(pt, found, find_len, rep_data, rep_len);
        replacements++;
        cur = found + rep_len;
    }

    return replacements;
}
