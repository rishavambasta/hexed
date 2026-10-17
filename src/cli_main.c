#include "version.h"
#include "core/document.h"
#include "core/checksums.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("%s - High-Performance Hex Editor by %s\n", HEXED_TITLE_BASE, HEXED_AUTHOR);
        if (argc < 2) {
            printf("Usage: %s [filename] [--version] [--checksums <file>]\n", argv[0]);
        }
        return 0;
    }

    if (strcmp(argv[1], "--checksums") == 0 && argc >= 3) {
        const char *filename = argv[2];
        document_t *doc = doc_open_file(filename);
        if (!doc) {
            fprintf(stderr, "Error: cannot open file %s\n", filename);
            return 1;
        }

        hxd_size_t sz = pt_get_size(doc->pt);
        printf("%s Checksums for %s (%zu bytes):\n", HEXED_TITLE_BASE, filename, (size_t)sz);
        hxd_range_t range = { 0, sz };
        checksum_list_t *list = checksums_calculate(doc->pt, range, doc->endian);
        if (list) {
            for (size_t i = 0; i < list->count; i++) {
                printf("  %-16s %s\n", list->items[i].name, list->items[i].result_hex);
            }
            checksums_free(list);
        }
        doc_free(doc);
        return 0;
    }

    printf("%s: Loading %s...\n", HEXED_TITLE_BASE, argv[1]);
    document_t *doc = doc_open_file(argv[1]);
    if (!doc) {
        fprintf(stderr, "Failed to load document: %s\n", argv[1]);
        return 1;
    }
    printf("Successfully opened '%s' (%zu bytes)\n", doc->display_name ? doc->display_name : "Untitled", (size_t)pt_get_size(doc->pt));
    doc_free(doc);
    return 0;
}
