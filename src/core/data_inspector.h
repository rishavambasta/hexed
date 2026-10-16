#ifndef HXD_DATA_INSPECTOR_H
#define HXD_DATA_INSPECTOR_H

#include "types.h"
#include "piece_table.h"

typedef struct {
    const char *id;
    const char *name;
    const char *description;
    size_t      bytes_required;
    char        value[128];
    bool        is_valid;
    bool        has_color;
    uint8_t     color_r, color_g, color_b, color_a;
} inspector_entry_t;

typedef struct {
    inspector_entry_t entries[25];
    size_t            count;
    hxd_endianness_t  endian;
    bool              display_hex;
} inspector_results_t;

void inspector_compute(const piece_table_t *pt, hxd_offset_t offset, hxd_endianness_t endian, bool display_hex, inspector_results_t *out);

/* Convert user edited string back to binary bytes for a given type ID */
bool inspector_write_value(piece_table_t *pt, hxd_offset_t offset, const char *type_id, const char *input_str, hxd_endianness_t endian);

#endif // HXD_DATA_INSPECTOR_H
