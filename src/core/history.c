#include "history.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 64

history_t *history_create(void) {
    history_t *h = calloc(1, sizeof(history_t));
    h->capacity = INITIAL_CAPACITY;
    h->entries = calloc(h->capacity, sizeof(history_entry_t));
    h->current_index = -1;
    h->count = 0;
    h->group_same = true;
    return h;
}

static void free_entry(history_entry_t *e) {
    if (!e) return;
    if (e->description) free(e->description);
    if (e->snapshot) pt_free(e->snapshot);
    memset(e, 0, sizeof(*e));
}

void history_free(history_t *h) {
    if (!h) return;
    for (int i = 0; i < h->count; i++) {
        free_entry(&h->entries[i]);
    }
    free(h->entries);
    free(h);
}

bool history_record(history_t *h, hxd_action_type_t type, const char *desc, hxd_offset_t off, hxd_size_t len, const piece_table_t *pt) {
    if (!h || !pt) return false;

    // Truncate any redo entries ahead of current_index
    for (int i = h->current_index + 1; i < h->count; i++) {
        free_entry(&h->entries[i]);
    }
    h->count = h->current_index + 1;

    // Optional coalescing for sequential typing (WRITE 1 byte adjacent)
    if (h->group_same && h->count > 0 && h->current_index >= 0) {
        history_entry_t *last = &h->entries[h->current_index];
        if (last->action_type == HXD_ACTION_WRITE && type == HXD_ACTION_WRITE) {
            if (off == last->offset + last->length) {
                // Extend the last action
                last->length += len;
                last->timestamp = time(NULL);
                if (last->snapshot) pt_free(last->snapshot);
                last->snapshot = pt_clone(pt);
                return true;
            }
        }
    }

    // Grow if needed
    if (h->count >= h->capacity) {
        h->capacity *= 2;
        h->entries = realloc(h->entries, h->capacity * sizeof(history_entry_t));
    }

    history_entry_t *entry = &h->entries[h->count];
    entry->action_type = type;
    entry->description = desc ? strdup(desc) : strdup(history_action_type_to_string(type));
    entry->offset = off;
    entry->length = len;
    entry->timestamp = time(NULL);
    entry->snapshot = pt_clone(pt);

    h->current_index = h->count;
    h->count++;
    return true;
}

bool history_can_undo(const history_t *h) {
    return h && h->current_index > 0;
}

bool history_can_redo(const history_t *h) {
    return h && h->current_index + 1 < h->count;
}

piece_table_t *history_undo(history_t *h) {
    if (!history_can_undo(h)) return NULL;
    h->current_index--;
    return pt_clone(h->entries[h->current_index].snapshot);
}

piece_table_t *history_redo(history_t *h) {
    if (!history_can_redo(h)) return NULL;
    h->current_index++;
    return pt_clone(h->entries[h->current_index].snapshot);
}

piece_table_t *history_jump_to(history_t *h, int target_index) {
    if (!h || target_index < 0 || target_index >= h->count) return NULL;
    h->current_index = target_index;
    return pt_clone(h->entries[h->current_index].snapshot);
}

int history_get_count(const history_t *h) {
    return h ? h->count : 0;
}

int history_get_current_index(const history_t *h) {
    return h ? h->current_index : -1;
}

const history_entry_t *history_get_entry(const history_t *h, int index) {
    if (!h || index < 0 || index >= h->count) return NULL;
    return &h->entries[index];
}

const char *history_action_type_to_string(hxd_action_type_t type) {
    switch (type) {
        case HXD_ACTION_NEW:     return "New";
        case HXD_ACTION_OPEN:    return "Open";
        case HXD_ACTION_WRITE:   return "Write";
        case HXD_ACTION_INSERT:  return "Insert";
        case HXD_ACTION_DELETE:  return "Delete";
        case HXD_ACTION_FILL:    return "Fill";
        case HXD_ACTION_REPLACE: return "Replace";
        case HXD_ACTION_RESIZE:  return "Resize";
        case HXD_ACTION_GAP:     return "Gap";
        case HXD_ACTION_PASTE:   return "Paste";
        default:                 return "Operation";
    }
}

const char *history_action_type_to_icon(hxd_action_type_t type) {
    switch (type) {
        case HXD_ACTION_NEW:     return "filenew.svg";
        case HXD_ACTION_OPEN:    return "fileopen.svg";
        case HXD_ACTION_WRITE:   return "editcell.svg";
        case HXD_ACTION_INSERT:  return "editinsert.svg";
        case HXD_ACTION_DELETE:  return "editclear.svg";
        case HXD_ACTION_FILL:    return "editfill.svg";
        case HXD_ACTION_REPLACE: return "editreplace.svg";
        case HXD_ACTION_RESIZE:  return "editsetsize.svg";
        case HXD_ACTION_GAP:     return "cmd.selection.gap.svg";
        case HXD_ACTION_PASTE:   return "editpaste.svg";
        default:                 return "editcell.svg";
    }
}
