#include "../src/core/piece_table.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void) {
    printf("Testing piece_table...\n");

    // Test 1: Memory open
    const char *hello = "Hello World!";
    piece_table_t *pt = pt_open_memory((const uint8_t *)hello, strlen(hello));
    assert(pt_get_size(pt) == 12);

    uint8_t buf[32];
    memset(buf, 0, sizeof(buf));
    bool is_gap[32];
    hxd_size_t read_bytes = pt_read_bytes(pt, 0, buf, 12, is_gap);
    assert(read_bytes == 12);
    assert(memcmp(buf, "Hello World!", 12) == 0);
    assert(is_gap[0] == false);

    // Test 2: Overwrite
    pt_overwrite_bytes(pt, 6, (const uint8_t *)"There", 5);
    read_bytes = pt_read_bytes(pt, 0, buf, 12, is_gap);
    assert(read_bytes == 12);
    assert(memcmp(buf, "Hello There!", 12) == 0);

    // Test 3: Insert
    pt_insert_bytes(pt, 5, (const uint8_t *)", Brave", 7);
    assert(pt_get_size(pt) == 19);
    read_bytes = pt_read_bytes(pt, 0, buf, 19, is_gap);
    assert(read_bytes == 19);
    assert(memcmp(buf, "Hello, Brave There!", 19) == 0);

    // Test 4: Delete
    pt_delete_bytes(pt, 5, 7);
    assert(pt_get_size(pt) == 12);
    read_bytes = pt_read_bytes(pt, 0, buf, 12, is_gap);
    assert(read_bytes == 12);
    assert(memcmp(buf, "Hello There!", 12) == 0);

    // Test 5: Sparse Gap
    pt_make_gap(pt, 5, 2);
    assert(pt_get_size(pt) == 12);
    read_bytes = pt_read_bytes(pt, 0, buf, 12, is_gap);
    assert(read_bytes == 12);
    assert(is_gap[4] == false);
    assert(is_gap[5] == true);
    assert(is_gap[6] == true);
    assert(is_gap[7] == false);

    // Test 6: Fill
    pt_fill_bytes(pt, 5, 2, 'X');
    read_bytes = pt_read_bytes(pt, 0, buf, 12, is_gap);
    assert(read_bytes == 12);
    assert(buf[5] == 'X' && buf[6] == 'X');
    assert(is_gap[5] == false);

    // Test 7: Clone
    piece_table_t *clone = pt_clone(pt);
    assert(pt_get_size(clone) == 12);
    pt_insert_bytes(clone, 0, (const uint8_t *)"Start: ", 7);
    assert(pt_get_size(clone) == 19);
    assert(pt_get_size(pt) == 12);

    pt_free(clone);
    pt_free(pt);

    printf("piece_table tests passed successfully!\n");
    return 0;
}
