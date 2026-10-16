#include "piece_table.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
#else
    #include <unistd.h>
    #include <sys/mman.h>
#endif

#define INITIAL_APPEND_CAPACITY 65536

static piece_append_buffer_t *append_buf_create(void) {
    piece_append_buffer_t *buf = calloc(1, sizeof(piece_append_buffer_t));
    buf->capacity = INITIAL_APPEND_CAPACITY;
    buf->data = malloc(buf->capacity);
    buf->size = 0;
    return buf;
}

static void append_buf_free(piece_append_buffer_t *buf) {
    if (!buf) return;
    free(buf->data);
    free(buf);
}

static hxd_offset_t append_buf_append(piece_append_buffer_t *buf, const uint8_t *data, size_t len) {
    if (buf->size + len > buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        while (new_cap < buf->size + len) {
            new_cap *= 2;
        }
        buf->data = realloc(buf->data, new_cap);
        buf->capacity = new_cap;
    }
    hxd_offset_t offset = buf->size;
    memcpy(buf->data + offset, data, len);
    buf->size += len;
    return offset;
}

static piece_t *piece_create(piece_source_t source, hxd_offset_t src_off, hxd_size_t len, uint8_t fill_byte) {
    piece_t *p = calloc(1, sizeof(piece_t));
    p->source = source;
    p->source_offset = src_off;
    p->length = len;
    p->fill_byte = fill_byte;
    return p;
}

piece_table_t *pt_create(void) {
    piece_table_t *pt = calloc(1, sizeof(piece_table_t));
    pt->original_fd = -1;
    pt->add_buf = append_buf_create();
    pt->buf_refcount = malloc(sizeof(int));
    *pt->buf_refcount = 1;
    return pt;
}

piece_table_t *pt_open_file(const char *path) {
    piece_table_t *pt = pt_create();
    if (!path) return pt;

    pt->original_path = strdup(path);

#ifdef _WIN32
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER size;
        if (GetFileSizeEx(hFile, &size) && size.QuadPart > 0) {
            pt->original_size = (hxd_size_t)size.QuadPart;
            HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
            if (hMap) {
                pt->original_mmap = (uint8_t *)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
                pt->is_mapped = (pt->original_mmap != NULL);
                CloseHandle(hMap);
            }
        }
        CloseHandle(hFile);
    }
#else
    int fd = open(path, O_RDONLY);
    if (fd >= 0) {
        struct stat st;
        if (fstat(fd, &st) == 0 && st.st_size > 0) {
            pt->original_size = (hxd_size_t)st.st_size;
            pt->original_fd = fd;
            void *addr = mmap(NULL, pt->original_size, PROT_READ, MAP_SHARED, fd, 0);
            if (addr != MAP_FAILED) {
                pt->original_mmap = (uint8_t *)addr;
                pt->is_mapped = true;
            }
        } else {
            close(fd);
            pt->original_fd = -1;
        }
    }
#endif

    // Fallback if mmap failed or not supported: read file into memory
    if (pt->original_size > 0 && !pt->is_mapped) {
        FILE *f = fopen(path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (sz > 0) {
                pt->original_size = (hxd_size_t)sz;
                pt->original_mmap = malloc(sz);
                if (fread(pt->original_mmap, 1, sz, f) == (size_t)sz) {
                    pt->is_mapped = false; // malloc'd
                }
            }
            fclose(f);
        }
    }

    if (pt->original_size > 0 && pt->original_mmap != NULL) {
        piece_t *p = piece_create(PIECE_ORIGINAL, 0, pt->original_size, 0);
        pt->head = pt->tail = p;
        pt->total_size = pt->original_size;
        pt->piece_count = 1;
    }

    return pt;
}

piece_table_t *pt_open_memory(const uint8_t *data, hxd_size_t size) {
    piece_table_t *pt = pt_create();
    if (data && size > 0) {
        hxd_offset_t off = append_buf_append(pt->add_buf, data, size);
        piece_t *p = piece_create(PIECE_BUFFER, off, size, 0);
        pt->head = pt->tail = p;
        pt->total_size = size;
        pt->piece_count = 1;
    }
    return pt;
}

piece_table_t *pt_clone(const piece_table_t *src) {
    if (!src) return NULL;
    piece_table_t *clone = calloc(1, sizeof(piece_table_t));
    if (src->original_path) clone->original_path = strdup(src->original_path);
    clone->original_mmap = src->original_mmap;
    clone->original_size = src->original_size;
    clone->original_fd = src->original_fd;
    clone->is_mapped = src->is_mapped;

    clone->add_buf = src->add_buf;
    clone->buf_refcount = src->buf_refcount;
    if (clone->buf_refcount) {
        (*clone->buf_refcount)++;
    }

    // Deep copy piece list
    piece_t *curr = src->head;
    piece_t *prev = NULL;
    while (curr) {
        piece_t *p = piece_create(curr->source, curr->source_offset, curr->length, curr->fill_byte);
        p->prev = prev;
        if (prev) {
            prev->next = p;
        } else {
            clone->head = p;
        }
        prev = p;
        curr = curr->next;
    }
    clone->tail = prev;
    clone->total_size = src->total_size;
    clone->piece_count = src->piece_count;

    return clone;
}

void pt_free(piece_table_t *pt) {
    if (!pt) return;

    piece_t *curr = pt->head;
    while (curr) {
        piece_t *next = curr->next;
        free(curr);
        curr = next;
    }

    if (pt->buf_refcount) {
        (*pt->buf_refcount)--;
        if (*pt->buf_refcount <= 0) {
            append_buf_free(pt->add_buf);
            free(pt->buf_refcount);
            // Free original mmap if we are the last owner
            if (pt->original_mmap) {
#ifdef _WIN32
                if (pt->is_mapped) UnmapViewOfFile(pt->original_mmap);
                else free(pt->original_mmap);
#else
                if (pt->is_mapped) munmap(pt->original_mmap, pt->original_size);
                else free(pt->original_mmap);
#endif
            }
            if (pt->original_fd >= 0) {
#ifndef _WIN32
                close(pt->original_fd);
#endif
            }
        }
    }

    if (pt->original_path) free(pt->original_path);
    free(pt);
}

hxd_size_t pt_get_size(const piece_table_t *pt) {
    return pt ? pt->total_size : 0;
}

/* Helper to find the piece containing the target offset and the offset within that piece */
static piece_t *find_piece_at_offset(const piece_table_t *pt, hxd_offset_t offset, hxd_offset_t *out_piece_off) {
    if (!pt || offset >= pt->total_size) return NULL;
    hxd_offset_t curr_off = 0;
    piece_t *curr = pt->head;
    while (curr) {
        if (offset < curr_off + curr->length) {
            if (out_piece_off) *out_piece_off = offset - curr_off;
            return curr;
        }
        curr_off += curr->length;
        curr = curr->next;
    }
    return NULL;
}

bool pt_is_gap(const piece_table_t *pt, hxd_offset_t offset) {
    hxd_offset_t p_off = 0;
    piece_t *p = find_piece_at_offset(pt, offset, &p_off);
    return (p && p->source == PIECE_GAP);
}

uint8_t pt_get_byte(const piece_table_t *pt, hxd_offset_t offset, bool *out_is_gap) {
    uint8_t b = 0;
    bool is_gap = false;
    pt_read_bytes(pt, offset, &b, 1, &is_gap);
    if (out_is_gap) *out_is_gap = is_gap;
    return b;
}

hxd_size_t pt_read_bytes(const piece_table_t *pt, hxd_offset_t offset, uint8_t *dest, hxd_size_t len, bool *out_is_gap) {
    if (!pt || !dest || len == 0 || offset >= pt->total_size) return 0;

    hxd_size_t bytes_to_read = len;
    if (offset + bytes_to_read > pt->total_size) {
        bytes_to_read = pt->total_size - offset;
    }

    hxd_offset_t curr_off = 0;
    hxd_size_t bytes_copied = 0;
    piece_t *curr = pt->head;

    while (curr && bytes_copied < bytes_to_read) {
        hxd_offset_t piece_start = curr_off;
        hxd_offset_t piece_end = curr_off + curr->length;

        if (offset + bytes_copied >= piece_start && offset + bytes_copied < piece_end) {
            hxd_offset_t in_piece_offset = (offset + bytes_copied) - piece_start;
            hxd_size_t in_piece_len = curr->length - in_piece_offset;
            hxd_size_t needed = bytes_to_read - bytes_copied;
            hxd_size_t chunk = (in_piece_len < needed) ? in_piece_len : needed;

            bool is_gap_piece = (curr->source == PIECE_GAP);

            if (out_is_gap) {
                for (hxd_size_t i = 0; i < chunk; ++i) {
                    out_is_gap[bytes_copied + i] = is_gap_piece;
                }
            }

            switch (curr->source) {
                case PIECE_ORIGINAL:
                    if (pt->original_mmap) {
                        memcpy(dest + bytes_copied, pt->original_mmap + curr->source_offset + in_piece_offset, chunk);
                    } else {
                        memset(dest + bytes_copied, 0, chunk);
                    }
                    break;
                case PIECE_BUFFER:
                    if (pt->add_buf && pt->add_buf->data) {
                        memcpy(dest + bytes_copied, pt->add_buf->data + curr->source_offset + in_piece_offset, chunk);
                    } else {
                        memset(dest + bytes_copied, 0, chunk);
                    }
                    break;
                case PIECE_FILL:
                    memset(dest + bytes_copied, curr->fill_byte, chunk);
                    break;
                case PIECE_GAP:
                    memset(dest + bytes_copied, 0, chunk);
                    break;
            }

            bytes_copied += chunk;
        }

        curr_off += curr->length;
        curr = curr->next;
    }

    return bytes_copied;
}

/* Split piece `p` at `split_offset` (0 < split_offset < p->length). Returns pointer to the right half. */
static piece_t *split_piece(piece_table_t *pt, piece_t *p, hxd_size_t split_offset) {
    if (!p || split_offset == 0 || split_offset >= p->length) return NULL;

    piece_t *right = piece_create(p->source, p->source_offset + split_offset, p->length - split_offset, p->fill_byte);
    p->length = split_offset;

    right->next = p->next;
    right->prev = p;
    if (p->next) {
        p->next->prev = right;
    } else {
        pt->tail = right;
    }
    p->next = right;
    pt->piece_count++;
    return right;
}

/* Insert a piece node before or after given reference, or as only piece if list is empty */
static void insert_piece_after(piece_table_t *pt, piece_t *ref, piece_t *new_p) {
    if (!ref) {
        // Insert at head
        new_p->next = pt->head;
        new_p->prev = NULL;
        if (pt->head) {
            pt->head->prev = new_p;
        } else {
            pt->tail = new_p;
        }
        pt->head = new_p;
    } else {
        new_p->next = ref->next;
        new_p->prev = ref;
        if (ref->next) {
            ref->next->prev = new_p;
        } else {
            pt->tail = new_p;
        }
        ref->next = new_p;
    }
    pt->piece_count++;
}

static void insert_piece_at_offset(piece_table_t *pt, hxd_offset_t offset, piece_t *new_p) {
    if (pt->head == NULL) {
        pt->head = pt->tail = new_p;
        pt->piece_count = 1;
        return;
    }

    if (offset == 0) {
        insert_piece_after(pt, NULL, new_p);
    } else if (offset >= pt->total_size) {
        insert_piece_after(pt, pt->tail, new_p);
    } else {
        hxd_offset_t in_piece = 0;
        piece_t *target = find_piece_at_offset(pt, offset, &in_piece);
        if (target) {
            if (in_piece > 0) {
                split_piece(pt, target, in_piece);
                insert_piece_after(pt, target, new_p);
            } else {
                insert_piece_after(pt, target->prev, new_p);
            }
        } else {
            insert_piece_after(pt, pt->tail, new_p);
        }
    }
}

bool pt_insert_bytes(piece_table_t *pt, hxd_offset_t offset, const uint8_t *data, hxd_size_t len) {
    if (!pt || !data || len == 0) return false;
    if (offset > pt->total_size) offset = pt->total_size;

    hxd_offset_t buf_off = append_buf_append(pt->add_buf, data, len);
    piece_t *new_p = piece_create(PIECE_BUFFER, buf_off, len, 0);

    insert_piece_at_offset(pt, offset, new_p);
    pt->total_size += len;
    return true;
}

bool pt_insert_gap(piece_table_t *pt, hxd_offset_t offset, hxd_size_t len) {
    if (!pt || len == 0) return false;
    if (offset > pt->total_size) offset = pt->total_size;

    piece_t *new_p = piece_create(PIECE_GAP, 0, len, 0);

    insert_piece_at_offset(pt, offset, new_p);
    pt->total_size += len;
    return true;
}

bool pt_delete_bytes(piece_table_t *pt, hxd_offset_t offset, hxd_size_t len) {
    if (!pt || len == 0 || offset >= pt->total_size) return false;

    if (offset + len > pt->total_size) {
        len = pt->total_size - offset;
    }

    hxd_offset_t del_end = offset + len;
    hxd_offset_t curr_off = 0;
    piece_t *curr = pt->head;

    while (curr && curr_off < del_end) {
        piece_t *next = curr->next;
        hxd_offset_t piece_start = curr_off;
        hxd_offset_t piece_end = curr_off + curr->length;

        if (del_end <= piece_start) {
            break;
        }

        if (offset <= piece_start && del_end >= piece_end) {
            // Remove entire piece
            if (curr->prev) curr->prev->next = curr->next;
            else pt->head = curr->next;

            if (curr->next) curr->next->prev = curr->prev;
            else pt->tail = curr->prev;

            free(curr);
            pt->piece_count--;
        } else if (offset > piece_start && del_end < piece_end) {
            // Delete middle of piece: split into left and right
            hxd_size_t left_len = offset - piece_start;
            hxd_size_t right_offset = del_end - piece_start;
            hxd_size_t right_len = curr->length - right_offset;

            piece_t *right = piece_create(curr->source, curr->source_offset + right_offset, right_len, curr->fill_byte);
            right->next = curr->next;
            right->prev = curr;
            if (curr->next) curr->next->prev = right;
            else pt->tail = right;
            curr->next = right;
            curr->length = left_len;
            pt->piece_count++;
            break;
        } else if (offset > piece_start && offset < piece_end) {
            // Trim right side of piece
            curr->length = offset - piece_start;
        } else if (del_end > piece_start && del_end < piece_end) {
            // Trim left side of piece
            hxd_size_t cut = del_end - piece_start;
            curr->source_offset += cut;
            curr->length -= cut;
        }

        curr_off = piece_end;
        curr = next;
    }

    pt->total_size -= len;
    return true;
}

bool pt_overwrite_bytes(piece_table_t *pt, hxd_offset_t offset, const uint8_t *data, hxd_size_t len) {
    if (!pt || !data || len == 0) return false;

    // If offset is beyond current size, pad with zeros up to offset
    if (offset > pt->total_size) {
        hxd_size_t pad = offset - pt->total_size;
        pt_fill_bytes(pt, pt->total_size, pad, 0);
    }

    // Delete existing range if within bounds
    if (offset < pt->total_size) {
        hxd_size_t to_delete = len;
        if (offset + to_delete > pt->total_size) {
            to_delete = pt->total_size - offset;
        }
        pt_delete_bytes(pt, offset, to_delete);
    }

    // Insert new bytes
    return pt_insert_bytes(pt, offset, data, len);
}

bool pt_fill_bytes(piece_table_t *pt, hxd_offset_t offset, hxd_size_t len, uint8_t fill_byte) {
    if (!pt || len == 0) return false;

    if (offset < pt->total_size) {
        hxd_size_t to_delete = len;
        if (offset + to_delete > pt->total_size) {
            to_delete = pt->total_size - offset;
        }
        pt_delete_bytes(pt, offset, to_delete);
    } else if (offset > pt->total_size) {
        hxd_size_t pad = offset - pt->total_size;
        pt_fill_bytes(pt, pt->total_size, pad, 0);
    }

    piece_t *new_p = piece_create(PIECE_FILL, 0, len, fill_byte);

    insert_piece_at_offset(pt, offset, new_p);
    pt->total_size += len;
    return true;
}

bool pt_make_gap(piece_table_t *pt, hxd_offset_t offset, hxd_size_t len) {
    if (!pt || len == 0) return false;

    if (offset < pt->total_size) {
        hxd_size_t to_delete = len;
        if (offset + to_delete > pt->total_size) {
            to_delete = pt->total_size - offset;
        }
        pt_delete_bytes(pt, offset, to_delete);
    } else if (offset > pt->total_size) {
        hxd_size_t pad = offset - pt->total_size;
        pt_fill_bytes(pt, pt->total_size, pad, 0);
    }

    return pt_insert_gap(pt, offset, len);
}

bool pt_set_size(piece_table_t *pt, hxd_size_t new_size, uint8_t pad_byte) {
    if (!pt) return false;
    if (new_size == pt->total_size) return true;

    if (new_size < pt->total_size) {
        return pt_delete_bytes(pt, new_size, pt->total_size - new_size);
    } else {
        return pt_fill_bytes(pt, pt->total_size, new_size - pt->total_size, pad_byte);
    }
}

bool pt_save_file(const piece_table_t *pt, const char *path) {
    if (!pt || !path) return false;

    FILE *f = fopen(path, "wb");
    if (!f) return false;

    uint8_t buffer[65536];
    hxd_offset_t offset = 0;
    while (offset < pt->total_size) {
        hxd_size_t chunk = sizeof(buffer);
        if (offset + chunk > pt->total_size) {
            chunk = pt->total_size - offset;
        }
        pt_read_bytes(pt, offset, buffer, chunk, NULL);
        if (fwrite(buffer, 1, chunk, f) != chunk) {
            fclose(f);
            return false;
        }
        offset += chunk;
    }

    fclose(f);
    return true;
}

void pt_iterate_spans(const piece_table_t *pt, pt_span_callback cb, void *userdata) {
    if (!pt || !cb) return;

    hxd_offset_t file_offset = 0;
    piece_t *curr = pt->head;

    while (curr) {
        bool is_gap = (curr->source == PIECE_GAP);
        const uint8_t *data_ptr = NULL;
        uint8_t stack_buf[1024];

        if (curr->source == PIECE_ORIGINAL && pt->original_mmap) {
            data_ptr = pt->original_mmap + curr->source_offset;
            cb(file_offset, data_ptr, curr->length, is_gap, userdata);
        } else if (curr->source == PIECE_BUFFER && pt->add_buf) {
            data_ptr = pt->add_buf->data + curr->source_offset;
            cb(file_offset, data_ptr, curr->length, is_gap, userdata);
        } else {
            // Fill or Gap chunks
            hxd_size_t rem = curr->length;
            hxd_offset_t chunk_off = file_offset;
            while (rem > 0) {
                hxd_size_t chunk = (rem < sizeof(stack_buf)) ? rem : sizeof(stack_buf);
                if (curr->source == PIECE_FILL) {
                    memset(stack_buf, curr->fill_byte, chunk);
                } else {
                    memset(stack_buf, 0, chunk);
                }
                cb(chunk_off, stack_buf, chunk, is_gap, userdata);
                chunk_off += chunk;
                rem -= chunk;
            }
        }

        file_offset += curr->length;
        curr = curr->next;
    }
}
