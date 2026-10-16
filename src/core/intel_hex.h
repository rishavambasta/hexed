#ifndef HXD_INTEL_HEX_H
#define HXD_INTEL_HEX_H

#include "types.h"
#include "piece_table.h"

typedef enum {
    IHEX_FMT_I8HEX  = 0, // 16-bit address only (HEX-80)
    IHEX_FMT_I16HEX = 1, // 20-bit segmented address (Extended Segment)
    IHEX_FMT_I32HEX = 2  // 32-bit linear address (Extended Linear)
} ihex_format_t;

/* Parse Intel HEX file into a piece table with gap preservation */
piece_table_t *ihex_parse_file(const char *path);
piece_table_t *ihex_parse_string(const char *content);

/* Check if a file looks like an Intel HEX file */
bool           ihex_is_intel_hex(const char *path_or_content, bool is_path);

/* Export piece table to Intel HEX text or file */
char          *ihex_export_string(const piece_table_t *pt, hxd_range_t range, ihex_format_t fmt, int bytes_per_record);
bool           ihex_export_file(const piece_table_t *pt, const char *path, hxd_range_t range, ihex_format_t fmt, int bytes_per_record);

#endif // HXD_INTEL_HEX_H
