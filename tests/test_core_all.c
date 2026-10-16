#include "../src/core/document.h"
#include "../src/core/search.h"
#include "../src/core/checksums.h"
#include "../src/core/data_inspector.h"
#include "../src/core/base_converter.h"
#include "../src/core/statistics.h"
#include "../src/core/export.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

int main(void) {
    printf("Testing all core components...\n");

    // Test Document & Editing
    document_t *doc = doc_create_new();
    assert(doc != NULL);
    assert(pt_get_size(doc->pt) == 0);

    // Type "Hello World" in ASCII
    const char *text = "Hello World";
    for (size_t i = 0; i < strlen(text); i++) {
        doc_type_ascii_char(doc, text[i]);
    }
    assert(pt_get_size(doc->pt) == 11);

    // Test Undo without grouping
    doc->history->group_same = false;
    doc_type_ascii_char(doc, '!');
    assert(pt_get_size(doc->pt) == 12);
    assert(doc_can_undo(doc) == true);
    doc_undo(doc);
    assert(pt_get_size(doc->pt) == 11);
    doc_redo(doc);
    assert(pt_get_size(doc->pt) == 12);
    doc_undo(doc);
    assert(pt_get_size(doc->pt) == 11);

    // Test Search
    search_params_t sp;
    memset(&sp, 0, sizeof(sp));
    sp.match_case = true;
    uint8_t *pat = NULL;
    size_t pat_len = 0;
    assert(search_parse_pattern("World", SEARCH_TYPE_ASCII, HXD_ENDIAN_LITTLE, &pat, &pat_len) == true);
    hxd_offset_t found = search_find(doc->pt, 0, pat, pat_len, &sp);
    assert(found == 6);
    free(pat);

    // Test Checksums
    hxd_range_t full = {0, 0};
    checksum_list_t *cs = checksums_calculate(doc->pt, full, HXD_ENDIAN_LITTLE);
    assert(cs != NULL);
    assert(cs->count > 10);
    // Print a few checksums
    for (size_t i = 0; i < cs->count; i++) {
        if (strcmp(cs->items[i].id, "crc32") == 0) {
            assert(strlen(cs->items[i].result_hex) == 8);
        }
    }
    checksums_free(cs);

    // Test Data Inspector
    inspector_results_t insp;
    inspector_compute(doc->pt, 0, HXD_ENDIAN_LITTLE, false, &insp);
    assert(insp.count == 25);
    assert(insp.entries[0].is_valid == true); // Binary
    assert(insp.entries[1].is_valid == true); // Int8
    assert(strcmp(insp.entries[1].value, "72") == 0); // 'H' is 72

    // Test Base Converter
    base_converter_state_t bc;
    base_converter_init(&bc);
    base_converter_update_from_hex(&bc, "1A2B");
    assert(strcmp(bc.dec, "6699") == 0);
    assert(strcmp(bc.hex, "00001A2B") == 0);

    // Test Statistics
    statistics_result_t st;
    statistics_calculate(doc->pt, full, &st);
    assert(st.total_bytes == 11);
    assert(st.printable_bytes == 11);
    assert(st.entropy > 2.0);

    // Test Export
    char *c_code = export_to_snippet(doc->pt, full, EXPORT_SNIPPET_C, 16);
    assert(strstr(c_code, "0x48") != NULL); // 'H' is 0x48
    free(c_code);

    char *data_url = export_to_data_url(doc->pt, full);
    assert(strstr(data_url, "data:application/octet-stream;base64,") != NULL);
    free(data_url);

    doc_free(doc);
    printf("All core tests passed successfully!\n");
    return 0;
}
