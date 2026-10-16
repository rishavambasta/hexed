#ifndef HXD_EXPORT_H
#define HXD_EXPORT_H

#include "types.h"
#include "piece_table.h"
#include "intel_hex.h"
#include "srec.h"

typedef enum {
    EXPORT_SNIPPET_C = 0,
    EXPORT_SNIPPET_HEX_DUMP,
    EXPORT_SNIPPET_PYTHON,
    EXPORT_SNIPPET_JS,
    EXPORT_SNIPPET_GO,
    EXPORT_SNIPPET_RUST,
    EXPORT_SNIPPET_CSHARP
} export_snippet_lang_t;

/* Export to Data URL (base64) */
char *export_to_data_url(const piece_table_t *pt, hxd_range_t range);

/* Export to code snippet */
char *export_to_snippet(const piece_table_t *pt, hxd_range_t range, export_snippet_lang_t lang, int bytes_per_line);

/* Export to binary file */
bool  export_to_binary_file(const piece_table_t *pt, const char *path, hxd_range_t range);

#endif // HXD_EXPORT_H
