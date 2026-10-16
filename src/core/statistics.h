#ifndef HXD_STATISTICS_H
#define HXD_STATISTICS_H

#include "types.h"
#include "piece_table.h"

typedef struct {
    uint64_t counts[256];
    uint64_t total_bytes;
    uint64_t null_bytes;
    uint64_t printable_bytes;
    uint64_t control_bytes;
    uint64_t high_bytes;
    double   entropy; // Shannon entropy (0.0 to 8.0)
    double   mean;
    uint8_t  min_val;
    uint8_t  max_val;
} statistics_result_t;

void statistics_calculate(const piece_table_t *pt, hxd_range_t range, statistics_result_t *out);

#endif // HXD_STATISTICS_H
