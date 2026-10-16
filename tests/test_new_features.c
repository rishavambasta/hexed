#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/core/piece_table.h"
#include "../src/core/document.h"
#include "../src/core/checksums.h"
#include "../src/core/search.h"

static void test_sha256_and_crc32(void) {
    printf("Testing SHA-256 and CRC-32 calculation on selections...\n");

    piece_table_t *pt = pt_create();
    const char *text = "123456789";
    pt_insert_bytes(pt, 0, (const uint8_t *)text, strlen(text));

    // Full range [0, 9)
    hxd_range_t range = { .start = 0, .length = 9 };
    checksum_list_t *list = checksums_calculate(pt, range, HXD_ENDIAN_LITTLE);
    assert(list != NULL);

    const char *crc32_hex = NULL;
    const char *sha256_hex = NULL;
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].id, "crc32") == 0) {
            crc32_hex = list->items[i].result_hex;
        } else if (strcmp(list->items[i].id, "sha256") == 0) {
            sha256_hex = list->items[i].result_hex;
        }
    }

    assert(crc32_hex != NULL);
    assert(sha256_hex != NULL);

    // Known CRC-32 for "123456789" is CBF43926
    assert(strcasecmp(crc32_hex, "CBF43926") == 0);

    // Known SHA-256 for "123456789"
    assert(strcasecmp(sha256_hex, "15e2b0d3c33891ebb0f1ef609ec419420c20e320ce94c65fbc8c3312448eb225") == 0);

    checksums_free(list);

    // Test sub-range selection [1, 5) -> "2345"
    hxd_range_t sub_range = { .start = 1, .length = 4 };
    list = checksums_calculate(pt, sub_range, HXD_ENDIAN_LITTLE);
    assert(list != NULL);
    crc32_hex = NULL;
    sha256_hex = NULL;
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].id, "crc32") == 0) crc32_hex = list->items[i].result_hex;
        if (strcmp(list->items[i].id, "sha256") == 0) sha256_hex = list->items[i].result_hex;
    }
    assert(crc32_hex != NULL && strlen(crc32_hex) > 0);
    assert(sha256_hex != NULL && strlen(sha256_hex) == 64);
    checksums_free(list);

    pt_free(pt);
    printf("  [PASS] SHA-256 and CRC-32 checksum calculations verified!\n");
}

static void test_diff_logic(void) {
    printf("Testing diff and comparison logic...\n");

    // File 1: 100 bytes of 0xAA
    piece_table_t *pt1 = pt_create();
    uint8_t b1[100];
    memset(b1, 0xAA, sizeof(b1));
    pt_insert_bytes(pt1, 0, b1, sizeof(b1));

    // File 2: initially identical
    piece_table_t *pt2 = pt_create();
    pt_insert_bytes(pt2, 0, b1, sizeof(b1));

    // Verify 100% identical comparison
    hxd_size_t sz1 = pt_get_size(pt1);
    hxd_size_t sz2 = pt_get_size(pt2);
    assert(sz1 == 100 && sz2 == 100);

    // Now introduce contiguous differences in File 2 at offset 20..24 (5 bytes)
    uint8_t diff_bytes[5] = { 0x11, 0x22, 0x33, 0x44, 0x55 };
    pt_overwrite_bytes(pt2, 20, diff_bytes, 5);

    // Also introduce difference at offset 80 (1 byte)
    uint8_t diff_single[1] = { 0xFF };
    pt_overwrite_bytes(pt2, 80, diff_single, 1);

    // Append 10 extra bytes to pt2 (past EOF of pt1)
    uint8_t extra[10];
    memset(extra, 0x77, sizeof(extra));
    pt_insert_bytes(pt2, 100, extra, sizeof(extra));

    // Simulate the diff scanning logic
    hxd_size_t max_sz = (pt_get_size(pt1) > pt_get_size(pt2)) ? pt_get_size(pt1) : pt_get_size(pt2);
    assert(max_sz == 110);

    size_t diff_blocks = 0;
    hxd_size_t total_diff_bytes = 0;
    bool in_diff = false;
    hxd_offset_t d_start = 0;
    hxd_size_t d_len = 0;

    for (hxd_offset_t i = 0; i < max_sz; i++) {
        uint8_t v1 = 0, v2 = 0;
        bool ex1 = (i < pt_get_size(pt1));
        bool ex2 = (i < pt_get_size(pt2));
        if (ex1) pt_read_bytes(pt1, i, &v1, 1, NULL);
        if (ex2) pt_read_bytes(pt2, i, &v2, 1, NULL);

        bool differs = (v1 != v2 || ex1 != ex2);
        if (differs) {
            total_diff_bytes++;
            if (!in_diff) {
                in_diff = true;
                d_start = i;
                d_len = 1;
            } else {
                d_len++;
            }
        } else {
            if (in_diff) {
                diff_blocks++;
                if (diff_blocks == 1) {
                    assert(d_start == 20);
                    assert(d_len == 5);
                } else if (diff_blocks == 2) {
                    assert(d_start == 80);
                    assert(d_len == 1);
                }
                in_diff = false;
            }
        }
    }
    if (in_diff) {
        diff_blocks++;
        // Block 3: offset 100, length 10 (past EOF of file 1)
        assert(d_start == 100);
        assert(d_len == 10);
    }

    assert(diff_blocks == 3);
    assert(total_diff_bytes == 16); // 5 + 1 + 10 = 16 bytes

    pt_free(pt1);
    pt_free(pt2);
    printf("  [PASS] Diff block coalescing and comparison logic verified!\n");
}

static void test_select_range_logic(void) {
    printf("Testing select range logic, parsing, and boundary clamping...\n");

    document_t *doc = doc_create_new();
    uint8_t sample[500];
    memset(sample, 0x42, sizeof(sample));
    doc_paste(doc, sample, sizeof(sample));

    assert(pt_get_size(doc->pt) == 500);

    // Initial selection
    doc_set_selection(doc, 50, 100);
    assert(doc->selection.start == 50);
    assert(doc->selection.length == 100);

    // Test selection union / extend logic
    // Existing: [50, 150), New: [120, 220)
    uint64_t start = 120;
    uint64_t len = 100;
    if (doc->selection.length > 0) {
        uint64_t u_start = (start < doc->selection.start) ? start : doc->selection.start;
        uint64_t cur_end = start + len;
        uint64_t ex_end = doc->selection.start + doc->selection.length;
        uint64_t u_end = (cur_end > ex_end) ? cur_end : ex_end;
        start = u_start;
        len = u_end - u_start;
    }
    assert(start == 50);
    assert(len == 170); // 50 to 220

    // Test bounds clamping
    hxd_size_t doc_size = pt_get_size(doc->pt); // 500
    uint64_t o_start = 450;
    uint64_t o_len = 100;
    if (o_start > doc_size) o_start = doc_size;
    if (o_start + o_len > doc_size) o_len = doc_size - o_start;
    assert(o_start == 450);
    assert(o_len == 50);

    // Test start past EOF clamping
    o_start = 600;
    o_len = 50;
    if (o_start > doc_size) o_start = doc_size;
    if (o_start + o_len > doc_size) o_len = (doc_size > o_start) ? doc_size - o_start : 0;
    assert(o_start == 500);
    assert(o_len == 0);

    doc_free(doc);
    printf("  [PASS] Select range logic and boundary clamping verified!\n");
}

static void test_bitwise_operations(void) {
    printf("Testing bitwise operations (NOT, AND, OR, XOR) and undo/redo...\n");

    document_t *doc = doc_create_new();
    uint8_t data[8] = { 0x00, 0xFF, 0x0F, 0xF0, 0xAA, 0x55, 0x12, 0x34 };
    doc_paste(doc, data, 8);

    // 1. NOT operation on entire file
    doc_bitwise_op(doc, 0, 8, HXD_BITOP_NOT, NULL, 0);
    uint8_t out[8];
    pt_read_bytes(doc->pt, 0, out, 8, NULL);
    assert(out[0] == 0xFF);
    assert(out[1] == 0x00);
    assert(out[2] == 0xF0);
    assert(out[3] == 0x0F);
    assert(out[4] == 0x55);
    assert(out[5] == 0xAA);
    assert(out[6] == (uint8_t)~0x12);
    assert(out[7] == (uint8_t)~0x34);

    // Undo NOT
    assert(doc_can_undo(doc));
    doc_undo(doc);
    pt_read_bytes(doc->pt, 0, out, 8, NULL);
    assert(memcmp(out, data, 8) == 0);

    // Redo NOT
    assert(doc_can_redo(doc));
    doc_redo(doc);
    pt_read_bytes(doc->pt, 0, out, 8, NULL);
    assert(out[0] == 0xFF);

    // Undo again to get back to original data
    doc_undo(doc);

    // 2. AND with 0x0F on range [4, 4) -> { 0xAA, 0x55, 0x12, 0x34 }
    uint8_t op_and = 0x0F;
    doc_bitwise_op(doc, 4, 4, HXD_BITOP_AND, &op_and, 1);
    pt_read_bytes(doc->pt, 0, out, 8, NULL);
    assert(out[0] == 0x00 && out[1] == 0xFF); // unchanged
    assert(out[4] == (0xAA & 0x0F)); // 0x0A
    assert(out[5] == (0x55 & 0x0F)); // 0x05
    assert(out[6] == (0x12 & 0x0F)); // 0x02
    assert(out[7] == (0x34 & 0x0F)); // 0x04

    // 3. OR with 0x80 on range [0, 2) -> { 0x00, 0xFF }
    uint8_t op_or = 0x80;
    doc_bitwise_op(doc, 0, 2, HXD_BITOP_OR, &op_or, 1);
    pt_read_bytes(doc->pt, 0, out, 8, NULL);
    assert(out[0] == 0x80);
    assert(out[1] == 0xFF);

    // 4. XOR with 0xFF on range [0, 8)
    uint8_t op_xor = 0xFF;
    doc_bitwise_op(doc, 0, 8, HXD_BITOP_XOR, &op_xor, 1);
    pt_read_bytes(doc->pt, 0, out, 8, NULL);
    assert(out[0] == (0x80 ^ 0xFF));
    assert(out[1] == (0xFF ^ 0xFF)); // 0x00

    doc_free(doc);
    printf("  [PASS] Bitwise operations and undo/redo history verified!\n");
}

static void test_pattern_wildcard_search(void) {
    printf("Testing pattern and wildcard mask search...\n");

    piece_table_t *pt = pt_create();
    uint8_t haystack[] = {
        0x90, 0x90, 0x48, 0x89, 0x5C, 0x24, 0x08, 0xCC,
        0x48, 0x89, 0x74, 0x24, 0x10, 0xC3, 0x90
    };
    pt_insert_bytes(pt, 0, haystack, sizeof(haystack));

    // Parse wildcard query "48 89 ?? 24 ??"
    uint8_t *pat = NULL;
    uint8_t *mask = NULL;
    size_t pat_len = 0;
    bool ok = search_parse_pattern_ex("48 89 ?? 24 ??", SEARCH_TYPE_WILDCARD, HXD_ENDIAN_LITTLE, &pat, &mask, &pat_len);
    assert(ok && pat != NULL && mask != NULL && pat_len == 5);
    assert(pat[0] == 0x48 && mask[0] == 0xFF);
    assert(pat[1] == 0x89 && mask[1] == 0xFF);
    assert(mask[2] == 0x00); // wildcard
    assert(pat[3] == 0x24 && mask[3] == 0xFF);
    assert(mask[4] == 0x00); // wildcard

    search_params_t params = {
        .type = SEARCH_TYPE_WILDCARD,
        .direction = SEARCH_DIR_FORWARD,
        .wildcard_mask = mask
    };

    hxd_offset_t found1 = search_find(pt, 0, pat, pat_len, &params);
    assert(found1 == 2); // 0x48 at index 2

    hxd_offset_t found2 = search_find(pt, 3, pat, pat_len, &params);
    assert(found2 == 8); // 0x48 at index 8

    search_results_t *all = search_find_all(pt, pat, pat_len, &params);
    assert(all != NULL);
    assert(all->count == 2);
    assert(all->offsets[0] == 2);
    assert(all->offsets[1] == 8);
    search_results_free(all);

    free(pat);
    free(mask);
    pt_free(pt);
    printf("  [PASS] Pattern and wildcard mask search verified!\n");
}

static void test_regex_search(void) {
    printf("Testing POSIX regex search on binary/text content...\n");

    piece_table_t *pt = pt_create();
    const char *data = "Header 123-4567 and some junk 888-9999 then End";
    pt_insert_bytes(pt, 0, (const uint8_t *)data, strlen(data));

    size_t matched_len = 0;
    // Match phone format: [0-9]{3}-[0-9]{4}
    hxd_offset_t off1 = search_find_regex(pt, 0, "[0-9]{3}-[0-9]{4}", false, true, &matched_len);
    assert(off1 == 7);
    assert(matched_len == 8);

    hxd_offset_t off2 = search_find_regex(pt, off1 + matched_len, "[0-9]{3}-[0-9]{4}", false, true, &matched_len);
    assert(off2 == 30);
    assert(matched_len == 8);

    // Find all
    search_results_t *all = search_find_all_regex(pt, "[0-9]{3}-[0-9]{4}", true);
    assert(all != NULL);
    assert(all->count == 2);
    assert(all->offsets[0] == 7);
    assert(all->offsets[1] == 30);
    search_results_free(all);

    pt_free(pt);
    printf("  [PASS] POSIX regex search verified!\n");
}

int main(void) {
    printf("=== Testing New Hex Editor Features ===\n");
    test_sha256_and_crc32();
    test_diff_logic();
    test_select_range_logic();
    test_bitwise_operations();
    test_pattern_wildcard_search();
    test_regex_search();
    printf("=== All New Features Verified Successfully! ===\n");
    return 0;
}
