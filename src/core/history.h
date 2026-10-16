#ifndef HXD_HISTORY_H
#define HXD_HISTORY_H

#include "types.h"
#include "piece_table.h"
#include <time.h>

typedef struct {
    hxd_action_type_t action_type;
    char             *description;
    hxd_offset_t      offset;
    hxd_size_t        length;
    time_t            timestamp;
    piece_table_t    *snapshot; // Cloned piece table representing state after this action
} history_entry_t;

typedef struct {
    history_entry_t *entries;
    int              count;
    int              capacity;
    int              current_index; // -1 if before first action, 0..count-1 for active state
    int              saved_index;   // Checkpoint index where file was saved to disk (-1 if unsaved)
    bool             group_same;
} history_t;

history_t       *history_create(void);
void             history_free(history_t *h);

/* Record a new action state. Clones pt to store the new snapshot. */
bool             history_record(history_t *h, hxd_action_type_t type, const char *desc, hxd_offset_t off, hxd_size_t len, const piece_table_t *pt);

/* Navigation */
bool             history_can_undo(const history_t *h);
bool             history_can_redo(const history_t *h);
piece_table_t   *history_undo(history_t *h);
piece_table_t   *history_redo(history_t *h);
piece_table_t   *history_jump_to(history_t *h, int target_index);

/* Inspection */
int              history_get_count(const history_t *h);
int              history_get_current_index(const history_t *h);
const history_entry_t *history_get_entry(const history_t *h, int index);
const char      *history_action_type_to_string(hxd_action_type_t type);
const char      *history_action_type_to_icon(hxd_action_type_t type);

#endif // HXD_HISTORY_H
