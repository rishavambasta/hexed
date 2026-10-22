#include "document.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int s_untitled_counter = 1;

static int hex_char_to_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

document_t *doc_create_new(void) {
    document_t *doc = calloc(1, sizeof(document_t));
    doc->pt = pt_create();
    doc->history = history_create();
    doc->bytes_per_row = 16;
    doc->endian = HXD_ENDIAN_LITTLE;
    doc->edit_mode = HXD_MODE_OVERWRITE;

    char title[64];
    snprintf(title, sizeof(title), "Untitled %d", s_untitled_counter++);
    doc->display_name = strdup(title);

    // Initial action
    history_record(doc->history, HXD_ACTION_NEW, "New Document", 0, 0, doc->pt);
    doc->is_modified = false;
    doc->saved_history_index = doc->history->current_index;
    doc->history->saved_index = doc->saved_history_index;
    doc->saved_pt = pt_clone(doc->pt);
    return doc;
}

document_t *doc_open_file_ex(const char *path, bool convert_ihex) {
    if (!path) return NULL;

    document_t *doc = calloc(1, sizeof(document_t));
    doc->filepath = strdup(path);

    // Extract base filename for display
    const char *slash = strrchr(path, '/');
#ifdef _WIN32
    const char *bslash = strrchr(path, '\\');
    if (bslash && (!slash || bslash > slash)) slash = bslash;
#endif
    doc->display_name = strdup(slash ? slash + 1 : path);

    // Check format: Intel HEX, SREC, or Binary
    if (ihex_is_intel_hex(path, true)) {
        if (convert_ihex) {
            doc->pt = ihex_parse_file(path);
            doc->is_raw_hex_format = false;
        } else {
            doc->pt = pt_open_file(path);
            doc->is_raw_hex_format = true;
        }
    } else if (srec_is_srec(path, true)) {
        doc->pt = srec_parse_file(path);
        doc->is_raw_hex_format = false;
    } else {
        doc->pt = pt_open_file(path);
        doc->is_raw_hex_format = false;
    }

    doc->history = history_create();
    doc->bytes_per_row = 16;
    doc->endian = HXD_ENDIAN_LITTLE;
    doc->edit_mode = HXD_MODE_OVERWRITE;

    history_record(doc->history, HXD_ACTION_OPEN, "Open File", 0, pt_get_size(doc->pt), doc->pt);
    doc->is_modified = false;
    doc->saved_history_index = doc->history->current_index;
    doc->history->saved_index = doc->saved_history_index;
    doc->saved_pt = pt_clone(doc->pt);
    return doc;
}

document_t *doc_open_file(const char *path) {
    return doc_open_file_ex(path, true);
}

bool doc_save(document_t *doc) {
    if (!doc || !doc->filepath) return false;
    return doc_save_as(doc, doc->filepath);
}

bool doc_save_as(document_t *doc, const char *new_path) {
    if (!doc || !new_path) return false;

    // Check extension for Intel HEX or SREC
    const char *ext = strrchr(new_path, '.');
    bool ok = false;
    hxd_range_t full = {0, 0};

    if (!doc->is_raw_hex_format && ext && (strcasecmp(ext, ".hex") == 0 || strcasecmp(ext, ".ihex") == 0)) {
        ok = ihex_export_file(doc->pt, new_path, full, IHEX_FMT_I32HEX, doc->bytes_per_row);
    } else if (!doc->is_raw_hex_format && ext && (strcasecmp(ext, ".srec") == 0 || strcasecmp(ext, ".s19") == 0 || strcasecmp(ext, ".s28") == 0 || strcasecmp(ext, ".s37") == 0)) {
        ok = srec_export_file(doc->pt, new_path, full, SREC_FMT_S19, "HXD", doc->bytes_per_row);
    } else {
        ok = pt_save_file(doc->pt, new_path);
    }

    if (ok) {
        char *saved_path = strdup(new_path);
        if (doc->filepath) free(doc->filepath);
        doc->filepath = saved_path;

        const char *slash = strrchr(doc->filepath, '/');
#ifdef _WIN32
        const char *bslash = strrchr(doc->filepath, '\\');
        if (bslash && (!slash || bslash > slash)) slash = bslash;
#endif
        if (doc->display_name) free(doc->display_name);
        doc->display_name = strdup(slash ? slash + 1 : doc->filepath);

        doc->is_modified = false;
        doc->saved_history_index = doc->history->current_index;
        doc->history->saved_index = doc->saved_history_index;

        if (!doc->is_raw_hex_format && (!ext || (strcasecmp(ext, ".hex") != 0 && strcasecmp(ext, ".ihex") != 0 && strcasecmp(ext, ".srec") != 0 && strcasecmp(ext, ".s19") != 0 && strcasecmp(ext, ".s28") != 0 && strcasecmp(ext, ".s37") != 0))) {
            piece_table_t *new_pt = pt_open_file(doc->filepath);
            if (new_pt) {
                pt_free(doc->pt);
                doc->pt = new_pt;
            }
        }

        if (doc->saved_pt) pt_free(doc->saved_pt);
        doc->saved_pt = pt_clone(doc->pt);
    }

    return ok;
}

void doc_free(document_t *doc) {
    if (!doc) return;
    if (doc->pt) pt_free(doc->pt);
    if (doc->saved_pt) pt_free(doc->saved_pt);
    if (doc->history) history_free(doc->history);
    if (doc->filepath) free(doc->filepath);
    if (doc->display_name) free(doc->display_name);
    if (doc->bookmarks) {
        for (size_t i = 0; i < doc->bookmark_count; i++) {
            if (doc->bookmarks[i].name) free(doc->bookmarks[i].name);
        }
        free(doc->bookmarks);
    }
    free(doc);
}

void doc_set_cursor(document_t *doc, hxd_offset_t offset, bool in_ascii) {
    if (!doc) return;
    hxd_size_t sz = pt_get_size(doc->pt);
    if (offset > sz) offset = sz;
    doc->cursor_offset = offset;
    doc->cursor_nibble = 0;
    doc->cursor_in_ascii = in_ascii;
}

void doc_set_selection(document_t *doc, hxd_offset_t start, hxd_size_t length) {
    if (!doc) return;
    hxd_size_t sz = pt_get_size(doc->pt);
    if (start > sz) start = sz;
    if (start + length > sz) length = sz - start;
    doc->selection.start = start;
    doc->selection.length = length;
}

void doc_clear_selection(document_t *doc) {
    if (!doc) return;
    doc->selection.start = doc->cursor_offset;
    doc->selection.length = 0;
}

void doc_select_all(document_t *doc) {
    if (!doc) return;
    doc->selection.start = 0;
    doc->selection.length = pt_get_size(doc->pt);
}

void doc_invert_selection(document_t *doc) {
    if (!doc) return;
    hxd_size_t sz = pt_get_size(doc->pt);
    if (doc->selection.length == 0) {
        doc_select_all(doc);
    } else {
        hxd_offset_t sel_end = doc->selection.start + doc->selection.length;
        if (doc->selection.start > 0) {
            doc->selection.start = 0;
            doc->selection.length = doc->selection.start;
        } else if (sel_end < sz) {
            doc->selection.start = sel_end;
            doc->selection.length = sz - sel_end;
        }
    }
}

bool doc_type_hex_digit(document_t *doc, char hex_char) {
    if (!doc) return false;
    int v = hex_char_to_val(hex_char);
    if (v < 0) return false;

    hxd_size_t sz = pt_get_size(doc->pt);
    uint8_t curr_byte = 0;
    bool is_gap = false;

    if (doc->edit_mode == HXD_MODE_INSERT) {
        if (doc->cursor_nibble == 0) {
            // Insert new byte with high nibble set
            uint8_t new_b = (uint8_t)(v << 4);
            pt_insert_bytes(doc->pt, doc->cursor_offset, &new_b, 1);
            doc->cursor_nibble = 1;
            doc->is_modified = true;
            history_record(doc->history, HXD_ACTION_INSERT, "Insert Hex", doc->cursor_offset, 1, doc->pt);
            return true;
        } else {
            // Update low nibble of newly inserted byte
            curr_byte = pt_get_byte(doc->pt, doc->cursor_offset, &is_gap);
            curr_byte = (curr_byte & 0xF0) | v;
            pt_overwrite_bytes(doc->pt, doc->cursor_offset, &curr_byte, 1);
            doc->cursor_nibble = 0;
            doc->cursor_offset++;
            doc->is_modified = true;
            history_record(doc->history, HXD_ACTION_WRITE, "Write Hex", doc->cursor_offset - 1, 1, doc->pt);
            return true;
        }
    } else {
        // OVERWRITE mode
        if (doc->cursor_offset < sz) {
            curr_byte = pt_get_byte(doc->pt, doc->cursor_offset, &is_gap);
        } else {
            curr_byte = 0;
        }

        if (doc->cursor_nibble == 0) {
            curr_byte = (curr_byte & 0x0F) | (v << 4);
            pt_overwrite_bytes(doc->pt, doc->cursor_offset, &curr_byte, 1);
            doc->cursor_nibble = 1;
        } else {
            curr_byte = (curr_byte & 0xF0) | v;
            pt_overwrite_bytes(doc->pt, doc->cursor_offset, &curr_byte, 1);
            doc->cursor_nibble = 0;
            doc->cursor_offset++;
        }
        doc->is_modified = true;
        history_record(doc->history, HXD_ACTION_WRITE, "Write Hex", doc->cursor_offset - (doc->cursor_nibble == 0 ? 1 : 0), 1, doc->pt);
        return true;
    }
}

bool doc_type_ascii_char(document_t *doc, char c) {
    if (!doc) return false;
    uint8_t b = (uint8_t)c;

    if (doc->edit_mode == HXD_MODE_INSERT) {
        pt_insert_bytes(doc->pt, doc->cursor_offset, &b, 1);
        doc->cursor_offset++;
        doc->is_modified = true;
        history_record(doc->history, HXD_ACTION_INSERT, "Insert Char", doc->cursor_offset - 1, 1, doc->pt);
        return true;
    } else {
        pt_overwrite_bytes(doc->pt, doc->cursor_offset, &b, 1);
        doc->cursor_offset++;
        doc->is_modified = true;
        history_record(doc->history, HXD_ACTION_WRITE, "Write Char", doc->cursor_offset - 1, 1, doc->pt);
        return true;
    }
}

bool doc_delete(document_t *doc) {
    if (!doc) return false;
    if (doc->selection.length > 0) {
        pt_delete_bytes(doc->pt, doc->selection.start, doc->selection.length);
        doc->cursor_offset = doc->selection.start;
        history_record(doc->history, HXD_ACTION_DELETE, "Delete Selection", doc->selection.start, doc->selection.length, doc->pt);
        doc->selection.length = 0;
        doc->is_modified = true;
        return true;
    } else if (doc->cursor_offset < pt_get_size(doc->pt)) {
        pt_delete_bytes(doc->pt, doc->cursor_offset, 1);
        doc->is_modified = true;
        history_record(doc->history, HXD_ACTION_DELETE, "Delete Byte", doc->cursor_offset, 1, doc->pt);
        return true;
    }
    return false;
}

bool doc_insert(document_t *doc, hxd_offset_t off, const uint8_t *data, hxd_size_t len) {
    if (!doc || !data || len == 0) return false;
    bool ok = pt_insert_bytes(doc->pt, off, data, len);
    if (ok) {
        doc->is_modified = true;
        history_record(doc->history, HXD_ACTION_INSERT, "Insert Data", off, len, doc->pt);
    }
    return ok;
}

bool doc_fill(document_t *doc, hxd_offset_t off, hxd_size_t len, uint8_t fill_byte) {
    if (!doc || len == 0) return false;
    bool ok = pt_fill_bytes(doc->pt, off, len, fill_byte);
    if (ok) {
        doc->is_modified = true;
        history_record(doc->history, HXD_ACTION_FILL, "Fill Pattern", off, len, doc->pt);
    }
    return ok;
}

bool doc_make_gap(document_t *doc, hxd_offset_t off, hxd_size_t len) {
    if (!doc || len == 0) return false;
    bool ok = pt_make_gap(doc->pt, off, len);
    if (ok) {
        doc->is_modified = true;
        history_record(doc->history, HXD_ACTION_GAP, "Make Gap", off, len, doc->pt);
    }
    return ok;
}

bool doc_set_size(document_t *doc, hxd_size_t new_size, uint8_t pad_byte) {
    if (!doc) return false;
    bool ok = pt_set_size(doc->pt, new_size, pad_byte);
    if (ok) {
        doc->is_modified = true;
        history_record(doc->history, HXD_ACTION_RESIZE, "Resize Document", 0, new_size, doc->pt);
    }
    return ok;
}

bool doc_paste(document_t *doc, const uint8_t *data, hxd_size_t len) {
    if (!doc || !data || len == 0) return false;

    if (doc->selection.length > 0) {
        pt_delete_bytes(doc->pt, doc->selection.start, doc->selection.length);
        doc->cursor_offset = doc->selection.start;
        doc->selection.length = 0;
    }

    bool ok = false;
    if (doc->edit_mode == HXD_MODE_INSERT) {
        ok = pt_insert_bytes(doc->pt, doc->cursor_offset, data, len);
    } else {
        ok = pt_overwrite_bytes(doc->pt, doc->cursor_offset, data, len);
    }

    if (ok) {
        doc->cursor_offset += len;
        doc->is_modified = true;
        history_record(doc->history, HXD_ACTION_PASTE, "Paste Data", doc->cursor_offset - len, len, doc->pt);
    }
    return ok;
}

bool doc_insert_file(document_t *doc, const char *path) {
    if (!doc || !path) return false;
    piece_table_t *file_pt = pt_open_file(path);
    if (!file_pt) return false;

    hxd_size_t fsz = pt_get_size(file_pt);
    if (fsz == 0) {
        pt_free(file_pt);
        return true;
    }

    uint8_t buf[65536];
    hxd_offset_t off = 0;
    hxd_offset_t insert_pos = doc->cursor_offset;

    while (off < fsz) {
        hxd_size_t chunk = sizeof(buf);
        if (off + chunk > fsz) chunk = fsz - off;
        pt_read_bytes(file_pt, off, buf, chunk, NULL);
        pt_insert_bytes(doc->pt, insert_pos, buf, chunk);
        insert_pos += chunk;
        off += chunk;
    }

    pt_free(file_pt);
    doc->is_modified = true;
    history_record(doc->history, HXD_ACTION_INSERT, "Insert File", doc->cursor_offset, fsz, doc->pt);
    doc->cursor_offset = insert_pos;
    return true;
}

bool doc_can_undo(const document_t *doc) {
    return doc && history_can_undo(doc->history);
}

bool doc_can_redo(const document_t *doc) {
    return doc && history_can_redo(doc->history);
}

bool doc_undo(document_t *doc) {
    if (!doc || !doc_can_undo(doc)) return false;
    piece_table_t *prev = history_undo(doc->history);
    if (prev) {
        pt_free(doc->pt);
        doc->pt = prev;
        doc->is_modified = (doc->history->current_index != doc->saved_history_index);
        if (doc->cursor_offset > pt_get_size(doc->pt)) {
            doc->cursor_offset = pt_get_size(doc->pt);
        }
        return true;
    }
    return false;
}

bool doc_redo(document_t *doc) {
    if (!doc || !doc_can_redo(doc)) return false;
    piece_table_t *next = history_redo(doc->history);
    if (next) {
        pt_free(doc->pt);
        doc->pt = next;
        doc->is_modified = (doc->history->current_index != doc->saved_history_index);
        if (doc->cursor_offset > pt_get_size(doc->pt)) {
            doc->cursor_offset = pt_get_size(doc->pt);
        }
        return true;
    }
    return false;
}

bool doc_jump_to_history(document_t *doc, int history_index) {
    if (!doc) return false;
    piece_table_t *st = history_jump_to(doc->history, history_index);
    if (st) {
        pt_free(doc->pt);
        doc->pt = st;
        doc->is_modified = (doc->history->current_index != doc->saved_history_index);
        if (doc->cursor_offset > pt_get_size(doc->pt)) {
            doc->cursor_offset = pt_get_size(doc->pt);
        }
        return true;
    }
    return false;
}

bool doc_toggle_bookmark(document_t *doc, hxd_offset_t offset, const char *name) {
    if (!doc) return false;
    for (size_t i = 0; i < doc->bookmark_count; i++) {
        if (doc->bookmarks[i].offset == offset) {
            doc_remove_bookmark(doc, i);
            return false; // Removed
        }
    }

    if (doc->bookmark_count >= doc->bookmark_capacity) {
        doc->bookmark_capacity = doc->bookmark_capacity ? doc->bookmark_capacity * 2 : 8;
        doc->bookmarks = realloc(doc->bookmarks, doc->bookmark_capacity * sizeof(bookmark_t));
    }

    char def_name[64];
    if (!name || strlen(name) == 0) {
        snprintf(def_name, sizeof(def_name), "Bookmark at 0x%08llX", (unsigned long long)offset);
        name = def_name;
    }

    doc->bookmarks[doc->bookmark_count].offset = offset;
    doc->bookmarks[doc->bookmark_count].name = strdup(name);
    doc->bookmark_count++;
    return true; // Added
}

void doc_remove_bookmark(document_t *doc, size_t index) {
    if (!doc || index >= doc->bookmark_count) return;
    if (doc->bookmarks[index].name) free(doc->bookmarks[index].name);
    for (size_t i = index; i + 1 < doc->bookmark_count; i++) {
        doc->bookmarks[i] = doc->bookmarks[i + 1];
    }
    doc->bookmark_count--;
}

bool doc_bitwise_op(document_t *doc, hxd_offset_t offset, hxd_size_t len, hxd_bitop_t op, const uint8_t *operand, size_t operand_len) {
    if (!doc || !doc->pt || len == 0) return false;
    bool ok = pt_bitwise_op(doc->pt, offset, len, op, operand, operand_len);
    if (ok) {
        doc->is_modified = true;
        const char *op_name = "Bitwise Op";
        if (op == HXD_BITOP_NOT) op_name = "Bitwise NOT";
        else if (op == HXD_BITOP_AND) op_name = "Bitwise AND";
        else if (op == HXD_BITOP_OR) op_name = "Bitwise OR";
        else if (op == HXD_BITOP_XOR) op_name = "Bitwise XOR";
        history_record(doc->history, HXD_ACTION_REPLACE, op_name, offset, len, doc->pt);
    }
    return ok;
}

void doc_get_modified_bytes(const document_t *doc, hxd_offset_t offset, size_t len, bool *out_modified) {
    if (!out_modified || len == 0) return;
    if (!doc || !doc->is_modified) {
        memset(out_modified, 0, len * sizeof(bool));
        return;
    }

    if (!doc->saved_pt) {
        memset(out_modified, 1, len * sizeof(bool));
        return;
    }

    hxd_size_t saved_sz = pt_get_size(doc->saved_pt);
    uint8_t curr_buf[128], saved_buf[128];
    bool curr_gap[128], saved_gap[128];

    uint8_t *curr_p = curr_buf;
    uint8_t *saved_p = saved_buf;
    bool *curr_gap_p = curr_gap;
    bool *saved_gap_p = saved_gap;

    if (len > sizeof(curr_buf)) {
        curr_p = malloc(len);
        saved_p = malloc(len);
        curr_gap_p = malloc(len * sizeof(bool));
        saved_gap_p = malloc(len * sizeof(bool));
        if (!curr_p || !saved_p || !curr_gap_p || !saved_gap_p) {
            if (curr_p && curr_p != curr_buf) free(curr_p);
            if (saved_p && saved_p != saved_buf) free(saved_p);
            if (curr_gap_p && curr_gap_p != curr_gap) free(curr_gap_p);
            if (saved_gap_p && saved_gap_p != saved_gap) free(saved_gap_p);
            memset(out_modified, 1, len * sizeof(bool));
            return;
        }
    }

    hxd_size_t curr_read = pt_read_bytes(doc->pt, offset, curr_p, len, curr_gap_p);
    hxd_size_t saved_read = 0;
    if (offset < saved_sz) {
        hxd_size_t to_read = (offset + len > saved_sz) ? (saved_sz - offset) : len;
        saved_read = pt_read_bytes(doc->saved_pt, offset, saved_p, to_read, saved_gap_p);
    }

    for (size_t i = 0; i < len; i++) {
        if (i >= curr_read) {
            out_modified[i] = false;
        } else if (offset + i >= saved_sz || i >= saved_read) {
            out_modified[i] = true;
        } else if (curr_p[i] != saved_p[i] || curr_gap_p[i] != saved_gap_p[i]) {
            out_modified[i] = true;
        } else {
            out_modified[i] = false;
        }
    }

    if (curr_p != curr_buf) free(curr_p);
    if (saved_p != saved_buf) free(saved_p);
    if (curr_gap_p != curr_gap) free(curr_gap_p);
    if (saved_gap_p != saved_gap) free(saved_gap_p);
}
