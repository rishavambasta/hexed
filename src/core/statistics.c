#include "statistics.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void statistics_calculate(const piece_table_t *pt, hxd_range_t range, statistics_result_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));

    if (!pt) return;

    hxd_offset_t start = range.length > 0 ? range.start : 0;
    hxd_offset_t total_len = range.length > 0 ? range.length : pt_get_size(pt);
    if (start >= pt_get_size(pt)) return;
    if (start + total_len > pt_get_size(pt)) total_len = pt_get_size(pt) - start;

    out->total_bytes = total_len;
    if (total_len == 0) return;

    out->min_val = 255;
    out->max_val = 0;

    uint8_t buffer[65536];
    hxd_offset_t processed = 0;
    uint64_t sum_all = 0;

    while (processed < total_len) {
        hxd_size_t chunk = sizeof(buffer);
        if (processed + chunk > total_len) {
            chunk = total_len - processed;
        }

        pt_read_bytes(pt, start + processed, buffer, chunk, NULL);

        for (hxd_size_t i = 0; i < chunk; i++) {
            uint8_t b = buffer[i];
            out->counts[b]++;
            sum_all += b;

            if (b < out->min_val) out->min_val = b;
            if (b > out->max_val) out->max_val = b;

            if (b == 0x00) {
                out->null_bytes++;
            } else if (b >= 32 && b <= 126) {
                out->printable_bytes++;
            } else if (b < 32 || b == 127) {
                out->control_bytes++;
            } else {
                out->high_bytes++;
            }
        }

        processed += chunk;
    }

    out->mean = (double)sum_all / (double)total_len;

    // Shannon entropy
    double entropy = 0.0;
    double log2_inv = 1.0 / log(2.0);

    for (int i = 0; i < 256; i++) {
        if (out->counts[i] > 0) {
            double p = (double)out->counts[i] / (double)total_len;
            entropy -= p * (log(p) * log2_inv);
        }
    }

    out->entropy = entropy;
}
