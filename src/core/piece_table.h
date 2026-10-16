#ifndef HXD_PIECE_TABLE_H
#define HXD_PIECE_TABLE_H

#include "types.h"
#include <stdio.h>

typedef enum {
    PIECE_ORIGINAL, // Data from original mapped file
    PIECE_BUFFER,   // Data from append-only memory buffer
    PIECE_FILL,     // Repeated fill byte (virtual, zero RAM)
    PIECE_GAP       // Sparse unallocated region (Intel HEX / S-record gaps)
} piece_source_t;

typedef struct piece {
    piece_source_t source;
    hxd_offset_t   source_offset; // Offset in source buffer/file
    hxd_size_t     length;        // Length of this piece
    uint8_t        fill_byte;     // Used when source == PIECE_FILL
    struct piece  *prev;
    struct piece  *next;
} piece_t;

typedef struct {
    uint8_t *data;
    size_t   size;
    size_t   capacity;
} piece_append_buffer_t;

typedef struct {
    char                  *original_path;
    uint8_t               *original_mmap;
    hxd_size_t             original_size;
    int                    original_fd;
    bool                   is_mapped;

    piece_append_buffer_t *add_buf; // Shared across clones (refcounted)
    int                   *buf_refcount;

    piece_t               *head;
    piece_t               *tail;
    hxd_size_t             total_size;
    size_t                 piece_count;
} piece_table_t;

/* Lifecycle */
piece_table_t *pt_create(void);
piece_table_t *pt_open_file(const char *path);
piece_table_t *pt_open_memory(const uint8_t *data, hxd_size_t size);
piece_table_t *pt_clone(const piece_table_t *src);
void           pt_free(piece_table_t *pt);

/* Queries */
hxd_size_t     pt_get_size(const piece_table_t *pt);
uint8_t        pt_get_byte(const piece_table_t *pt, hxd_offset_t offset, bool *out_is_gap);
bool           pt_is_gap(const piece_table_t *pt, hxd_offset_t offset);
hxd_size_t     pt_read_bytes(const piece_table_t *pt, hxd_offset_t offset, uint8_t *dest, hxd_size_t len, bool *out_is_gap);

/* Mutations */
bool           pt_insert_bytes(piece_table_t *pt, hxd_offset_t offset, const uint8_t *data, hxd_size_t len);
bool           pt_insert_gap(piece_table_t *pt, hxd_offset_t offset, hxd_size_t len);
bool           pt_overwrite_bytes(piece_table_t *pt, hxd_offset_t offset, const uint8_t *data, hxd_size_t len);
bool           pt_delete_bytes(piece_table_t *pt, hxd_offset_t offset, hxd_size_t len);
bool           pt_fill_bytes(piece_table_t *pt, hxd_offset_t offset, hxd_size_t len, uint8_t fill_byte);
bool           pt_make_gap(piece_table_t *pt, hxd_offset_t offset, hxd_size_t len);
bool           pt_set_size(piece_table_t *pt, hxd_size_t new_size, uint8_t pad_byte);

typedef enum {
    HXD_BITOP_NOT,
    HXD_BITOP_AND,
    HXD_BITOP_OR,
    HXD_BITOP_XOR
} hxd_bitop_t;

bool           pt_bitwise_op(piece_table_t *pt, hxd_offset_t offset, hxd_size_t len, hxd_bitop_t op, const uint8_t *operand, size_t operand_len);

/* Persistence */
bool           pt_save_file(const piece_table_t *pt, const char *path);

/* Piece inspection for sparse export */
typedef void (*pt_span_callback)(hxd_offset_t file_offset, const uint8_t *data, hxd_size_t length, bool is_gap, void *userdata);
void           pt_iterate_spans(const piece_table_t *pt, pt_span_callback cb, void *userdata);

#endif // HXD_PIECE_TABLE_H
