#ifndef HXD_TYPES_H
#define HXD_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint64_t hxd_offset_t;
typedef uint64_t hxd_size_t;

#define HXD_OFFSET_INVALID ((hxd_offset_t)-1)

typedef enum {
    HXD_ENDIAN_LITTLE = 0,
    HXD_ENDIAN_BIG    = 1
} hxd_endianness_t;

typedef enum {
    HXD_MODE_OVERWRITE = 0,
    HXD_MODE_INSERT    = 1
} hxd_edit_mode_t;

typedef enum {
    HXD_THEME_DARK = 0,
    HXD_THEME_LIGHT,
    HXD_THEME_BW,
    HXD_THEME_SYSTEM
} hxd_theme_t;

typedef enum {
    HXD_HEX_UPPERCASE = 0,
    HXD_HEX_LOWERCASE = 1
} hxd_hex_case_t;

typedef enum {
    HXD_ACTION_NEW,
    HXD_ACTION_OPEN,
    HXD_ACTION_WRITE,
    HXD_ACTION_INSERT,
    HXD_ACTION_DELETE,
    HXD_ACTION_FILL,
    HXD_ACTION_REPLACE,
    HXD_ACTION_RESIZE,
    HXD_ACTION_GAP,
    HXD_ACTION_PASTE
} hxd_action_type_t;

typedef struct {
    hxd_offset_t start;
    hxd_size_t   length;
} hxd_range_t;

static inline bool hxd_range_is_valid(hxd_range_t r) {
    return r.length > 0;
}

static inline bool hxd_range_contains(hxd_range_t r, hxd_offset_t off) {
    return off >= r.start && off < (r.start + r.length);
}

typedef struct {
    hxd_offset_t offset;
    hxd_size_t   length;
} app_diff_chunk_t;

#endif // HXD_TYPES_H
