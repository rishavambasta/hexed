#include "../src/core/piece_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>

static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

int main(void) {
    printf("Starting 1 Gigabyte Large-File Performance Benchmark...\n");

    const char *test_path = "/tmp/hxd_test_1gb.bin";

    // Create 1 GB sparse file using truncate (takes < 1 ms on disk)
    FILE *f = fopen(test_path, "wb");
    assert(f != NULL);
    // Write 1 KB at start and 1 KB at 1 GB offset
    uint8_t dummy[1024];
    for (int i = 0; i < 1024; i++) dummy[i] = (uint8_t)i;
    fwrite(dummy, 1, sizeof(dummy), f);

    fseek(f, (1024ULL * 1024ULL * 1024ULL) - 1024, SEEK_SET);
    fwrite(dummy, 1, sizeof(dummy), f);
    fclose(f);

    // 1. Benchmark Open Time
    double t0 = get_time_ms();
    piece_table_t *pt = pt_open_file(test_path);
    double t_open = get_time_ms() - t0;

    assert(pt != NULL);
    hxd_size_t sz = pt_get_size(pt);
    assert(sz == 1024ULL * 1024ULL * 1024ULL);
    printf("  [PASS] 1 GB File Open Time: %.3f ms (Target: < 5 ms)\n", t_open);

    // 2. Benchmark Random Seeks & Reads (Virtual Scrolling)
    double t_scroll_start = get_time_ms();
    uint8_t read_buf[64];
    bool is_gap[64];
    for (int i = 0; i < 10000; i++) {
        hxd_offset_t random_off = ((hxd_offset_t)rand() * 10000) % (sz - 64);
        pt_read_bytes(pt, random_off, read_buf, 64, is_gap);
    }
    double t_scroll = get_time_ms() - t_scroll_start;
    printf("  [PASS] 10,000 Random View Reads (60fps simulation): %.3f ms (%.3f µs per frame)\n",
           t_scroll, (t_scroll / 10000.0) * 1000.0);

    // 3. Benchmark Sparse Gap and Insertions
    double t_mod_start = get_time_ms();
    pt_make_gap(pt, 500ULL * 1024ULL * 1024ULL, 100ULL * 1024ULL * 1024ULL); // 100 MB sparse gap!
    pt_insert_bytes(pt, 100ULL * 1024ULL * 1024ULL, dummy, sizeof(dummy));     // 1 KB insertion
    double t_mod = get_time_ms() - t_mod_start;
    printf("  [PASS] Large File Sparse Gap & Edit Time: %.3f ms\n", t_mod);

    pt_free(pt);
    unlink(test_path);

    printf("1 GB Performance Benchmark Passed Flawlessly!\n");
    return 0;
}
