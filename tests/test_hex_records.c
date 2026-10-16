#include "../src/core/intel_hex.h"
#include "../src/core/srec.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

int main(void) {
    printf("Testing Intel HEX and Motorola S-Record...\n");

    // Intel HEX sample: 4 bytes at 0x0000, 4 bytes at 0x0010 (12-byte gap between 0x0004 and 0x0010)
    const char *ihex_sample =
        ":04000000AABBCCDDEE\r\n"
        ":040010001122334442\r\n"
        ":00000001FF\r\n";

    assert(ihex_is_intel_hex(ihex_sample, false) == true);
    piece_table_t *pt = ihex_parse_string(ihex_sample);
    assert(pt != NULL);
    assert(pt_get_size(pt) == 0x14); // 0x10 + 4 = 20 bytes

    // Check first 4 bytes
    uint8_t buf[32];
    bool is_gap[32];
    pt_read_bytes(pt, 0, buf, 4, is_gap);
    assert(buf[0] == 0xAA && buf[1] == 0xBB && buf[2] == 0xCC && buf[3] == 0xDD);
    assert(is_gap[0] == false);

    // Check gap at 0x04..0x0F
    pt_read_bytes(pt, 4, buf, 12, is_gap);
    for (int i = 0; i < 12; i++) {
        assert(is_gap[i] == true);
    }

    // Check 4 bytes at 0x10
    pt_read_bytes(pt, 0x10, buf, 4, is_gap);
    assert(buf[0] == 0x11 && buf[1] == 0x22 && buf[2] == 0x33 && buf[3] == 0x44);
    assert(is_gap[0] == false);

    // Export to Intel HEX string and verify
    hxd_range_t full_range = {0, 0};
    char *exported_ihex = ihex_export_string(pt, full_range, IHEX_FMT_I32HEX, 16);
    assert(exported_ihex != NULL);
    assert(strstr(exported_ihex, "AABBCCDD") != NULL);
    assert(strstr(exported_ihex, "11223344") != NULL);
    free(exported_ihex);
    pt_free(pt);

    // Motorola S-Record sample: S19 with S0 header, 4 bytes at 0x0000, 4 bytes at 0x0010
    const char *srec_sample =
        "S009000054455354B6\r\n"
        "S1070000AABBCCDDEA\r\n"
        "S1070010112233443E\r\n"
        "S9030000FC\r\n";

    assert(srec_is_srec(srec_sample, false) == true);
    piece_table_t *pt_srec = srec_parse_string(srec_sample);
    assert(pt_srec != NULL);
    assert(pt_get_size(pt_srec) == 0x14);

    pt_read_bytes(pt_srec, 0, buf, 4, is_gap);
    assert(buf[0] == 0xAA && buf[1] == 0xBB && buf[2] == 0xCC && buf[3] == 0xDD);
    assert(is_gap[0] == false);

    pt_read_bytes(pt_srec, 4, buf, 12, is_gap);
    for (int i = 0; i < 12; i++) {
        assert(is_gap[i] == true);
    }

    char *exported_srec = srec_export_string(pt_srec, full_range, SREC_FMT_S19, "TEST", 16);
    assert(exported_srec != NULL);
    assert(strstr(exported_srec, "AABBCCDD") != NULL);
    assert(strstr(exported_srec, "11223344") != NULL);
    free(exported_srec);
    pt_free(pt_srec);

    printf("Intel HEX and Motorola S-Record tests passed!\n");
    return 0;
}
