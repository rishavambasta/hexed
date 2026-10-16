#ifndef HXD_SREC_H
#define HXD_SREC_H

#include "types.h"
#include "piece_table.h"

typedef enum {
    SREC_FMT_S19 = 0, // 16-bit address
    SREC_FMT_S28 = 1, // 24-bit address
    SREC_FMT_S37 = 2  // 32-bit address
} srec_format_t;

/* Check if content or file starts with Motorola S-record */
bool           srec_is_srec(const char *path_or_content, bool is_path);

/* Parse Motorola S-record file into piece table with gap preservation */
piece_table_t *srec_parse_file(const char *path);
piece_table_t *srec_parse_string(const char *content);

/* Export to Motorola S-record string or file */
char          *srec_export_string(const piece_table_t *pt, hxd_range_t range, srec_format_t fmt, const char *header, int bytes_per_record);
bool           srec_export_file(const piece_table_t *pt, const char *path, hxd_range_t range, srec_format_t fmt, const char *header, int bytes_per_record);

#endif // HXD_SREC_H
