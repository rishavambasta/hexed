#ifndef HXD_SEARCH_H
#define HXD_SEARCH_H

#include "types.h"
#include "piece_table.h"

typedef enum {
    SEARCH_TYPE_HEX = 0,
    SEARCH_TYPE_ASCII,
    SEARCH_TYPE_UTF16,
    SEARCH_TYPE_INT8,
    SEARCH_TYPE_UINT8,
    SEARCH_TYPE_INT16,
    SEARCH_TYPE_UINT16,
    SEARCH_TYPE_INT32,
    SEARCH_TYPE_UINT32,
    SEARCH_TYPE_INT64,
    SEARCH_TYPE_UINT64,
    SEARCH_TYPE_FLOAT32,
    SEARCH_TYPE_FLOAT64,
    SEARCH_TYPE_BINARY
} search_type_t;

typedef enum {
    SEARCH_DIR_FORWARD = 0,
    SEARCH_DIR_BACKWARD = 1
} search_direction_t;

typedef struct {
    search_type_t      type;
    search_direction_t direction;
    hxd_endianness_t   endian;
    bool               match_case;
    bool               whole_word;
    hxd_range_t        scope; // If length > 0, restrict search to this range
} search_params_t;

typedef struct {
    hxd_offset_t *offsets;
    size_t        count;
    size_t        capacity;
} search_results_t;

/* Parse query string into raw byte pattern according to search parameters */
bool             search_parse_pattern(const char *query, search_type_t type, hxd_endianness_t endian, uint8_t **out_pattern, size_t *out_len);

/* Find next occurrence */
hxd_offset_t     search_find(const piece_table_t *pt, hxd_offset_t start_offset, const uint8_t *pattern, size_t pattern_len, const search_params_t *params);

/* Find all occurrences */
search_results_t *search_find_all(const piece_table_t *pt, const uint8_t *pattern, size_t pattern_len, const search_params_t *params);
void             search_results_free(search_results_t *res);

/* Replace next and replace all */
bool             search_replace(piece_table_t *pt, hxd_offset_t match_offset, size_t match_len, const uint8_t *replace_data, size_t replace_len);
size_t           search_replace_all(piece_table_t *pt, const uint8_t *find_pat, size_t find_len, const uint8_t *rep_data, size_t rep_len, const search_params_t *params);

#endif // HXD_SEARCH_H
