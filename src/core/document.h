#ifndef HXD_DOCUMENT_H
#define HXD_DOCUMENT_H

#include "types.h"
#include "piece_table.h"
#include "history.h"
#include "intel_hex.h"
#include "srec.h"

typedef struct {
    hxd_offset_t offset;
    char        *name;
} bookmark_t;

typedef struct {
    char             *filepath;
    char             *display_name;
    piece_table_t    *pt;
    history_t        *history;

    hxd_offset_t      cursor_offset;
    int               cursor_nibble; // 0 = high nibble, 1 = low nibble
    bool              cursor_in_ascii;
    hxd_range_t       selection;
    hxd_edit_mode_t   edit_mode; // OVERWRITE or INSERT

    bool              is_modified;
    int               saved_history_index;
    piece_table_t    *saved_pt;      // Baseline snapshot at open/save time for tracking edits
    hxd_endianness_t  endian;
    int               bytes_per_row; // default 16

    bookmark_t       *bookmarks;
    size_t            bookmark_count;
    size_t            bookmark_capacity;
    bool              is_raw_hex_format;
} document_t;

document_t *doc_create_new(void);
document_t *doc_open_file(const char *path);
document_t *doc_open_file_ex(const char *path, bool convert_ihex);
document_t *doc_duplicate(const document_t *src);
bool        doc_save(document_t *doc);
bool        doc_save_as(document_t *doc, const char *new_path);
void        doc_free(document_t *doc);

/* Cursor & Selection */
void        doc_set_cursor(document_t *doc, hxd_offset_t offset, bool in_ascii);
void        doc_set_selection(document_t *doc, hxd_offset_t start, hxd_size_t length);
void        doc_clear_selection(document_t *doc);
void        doc_select_all(document_t *doc);
void        doc_invert_selection(document_t *doc);

/* Interactive Typing */
bool        doc_type_hex_digit(document_t *doc, char hex_char);
bool        doc_type_ascii_char(document_t *doc, char c);

/* Edit commands */
bool        doc_delete(document_t *doc);
bool        doc_insert(document_t *doc, hxd_offset_t off, const uint8_t *data, hxd_size_t len);
bool        doc_fill(document_t *doc, hxd_offset_t off, hxd_size_t len, uint8_t fill_byte);
bool        doc_make_gap(document_t *doc, hxd_offset_t off, hxd_size_t len);
bool        doc_set_size(document_t *doc, hxd_size_t new_size, uint8_t pad_byte);
bool        doc_paste(document_t *doc, const uint8_t *data, hxd_size_t len);
bool        doc_insert_file(document_t *doc, const char *path);
bool        doc_bitwise_op(document_t *doc, hxd_offset_t offset, hxd_size_t len, hxd_bitop_t op, const uint8_t *operand, size_t operand_len);

/* Undo / Redo */
bool        doc_can_undo(const document_t *doc);
bool        doc_can_redo(const document_t *doc);
bool        doc_undo(document_t *doc);
bool        doc_redo(document_t *doc);
bool        doc_jump_to_history(document_t *doc, int history_index);

/* Bookmarks */
bool        doc_toggle_bookmark(document_t *doc, hxd_offset_t offset, const char *name);
void        doc_remove_bookmark(document_t *doc, size_t index);

/* Edit tracking */
void        doc_get_modified_bytes(const document_t *doc, hxd_offset_t offset, size_t len, bool *out_modified);

#endif // HXD_DOCUMENT_H
