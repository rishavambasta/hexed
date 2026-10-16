#ifndef HXD_CHECKSUMS_H
#define HXD_CHECKSUMS_H

#include "types.h"
#include "piece_table.h"

typedef struct {
    const char *id;
    const char *group;
    const char *name;
    const char *description;
    char        result_hex[128];
    char        result_dec[128];
} checksum_result_t;

typedef struct {
    checksum_result_t *items;
    size_t             count;
} checksum_list_t;

checksum_list_t *checksums_calculate(const piece_table_t *pt, hxd_range_t range, hxd_endianness_t endian);
void             checksums_free(checksum_list_t *list);

#endif // HXD_CHECKSUMS_H
