#include "sidebars.h"
#include "app.h"
#include "../core/data_inspector.h"
#include "../core/base_converter.h"
#include "../core/search.h"
#include "../core/export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Data Inspector Callbacks
 * ========================================================================= */

static void on_di_value_edited(GtkCellRendererText *renderer, gchar *path_str, gchar *new_text, gpointer user_data) {
    (void)renderer;
    Sidebars *sb = (Sidebars *)user_data;
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(sb->di_store), &iter, path_str)) return;

    gchar *type_id = NULL;
    gtk_tree_model_get(GTK_TREE_MODEL(sb->di_store), &iter, 3, &type_id, -1);
    if (!type_id) return;

    App *app = (App *)g_object_get_data(G_OBJECT(sb->left_notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (doc) {
        inspector_write_value(doc->pt, doc->cursor_offset, type_id, new_text, doc->endian);
        doc->is_modified = true;
        app_update_ui(app);
    }
    g_free(type_id);
}

static void on_di_endian_toggled(GtkButton *btn, gpointer user_data) {
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->left_notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (doc) {
        doc->endian = (doc->endian == HXD_ENDIAN_LITTLE) ? HXD_ENDIAN_BIG : HXD_ENDIAN_LITTLE;
        gtk_button_set_label(btn, doc->endian == HXD_ENDIAN_LITTLE ? "LE" : "BE");
        app_update_ui(app);
    }
}

static void on_di_editing_toggled(GtkToggleButton *btn, gpointer user_data) {
    Sidebars *sb = (Sidebars *)user_data;
    sb->di_editing_enabled = gtk_toggle_button_get_active(btn);
}

static void on_di_copy_all(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->left_notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    inspector_results_t insp;
    inspector_compute(doc->pt, doc->cursor_offset, doc->endian, false, &insp);

    GString *str = g_string_new(NULL);
    for (size_t i = 0; i < insp.count; i++) {
        if (insp.entries[i].is_valid) {
            g_string_append_printf(str, "%-16s: %s\n", insp.entries[i].name, insp.entries[i].value);
        }
    }
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), str->str, -1);
    g_string_free(str, TRUE);
}

/* =========================================================================
 * Base Converter Callbacks
 * ========================================================================= */

static void on_bc_entry_changed(GtkEditable *editable, gpointer user_data) {
    Sidebars *sb = (Sidebars *)user_data;
    if (sb->bc_updating) return;

    const char *text = gtk_entry_get_text(GTK_ENTRY(editable));
    base_converter_state_t bc;
    base_converter_init(&bc);
    bc.preserve_leading_zeros = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sb->bc_chk_zeros));

    bool ok = false;
    if (GTK_WIDGET(editable) == sb->bc_entry_bin) ok = base_converter_update_from_bin(&bc, text);
    else if (GTK_WIDGET(editable) == sb->bc_entry_oct) ok = base_converter_update_from_oct(&bc, text);
    else if (GTK_WIDGET(editable) == sb->bc_entry_dec) ok = base_converter_update_from_dec(&bc, text);
    else if (GTK_WIDGET(editable) == sb->bc_entry_hex) ok = base_converter_update_from_hex(&bc, text);

    if (ok) {
        sb->bc_updating = true;
        if (GTK_WIDGET(editable) != sb->bc_entry_bin) gtk_entry_set_text(GTK_ENTRY(sb->bc_entry_bin), bc.bin);
        if (GTK_WIDGET(editable) != sb->bc_entry_oct) gtk_entry_set_text(GTK_ENTRY(sb->bc_entry_oct), bc.oct);
        if (GTK_WIDGET(editable) != sb->bc_entry_dec) gtk_entry_set_text(GTK_ENTRY(sb->bc_entry_dec), bc.dec);
        if (GTK_WIDGET(editable) != sb->bc_entry_hex) gtk_entry_set_text(GTK_ENTRY(sb->bc_entry_hex), bc.hex);
        sb->bc_updating = false;
    }
}

static void on_bc_copy_all(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    char buf[512];
    snprintf(buf, sizeof(buf), "Bin: %s\nOct: %s\nDec: %s\nHex: %s\n",
             gtk_entry_get_text(GTK_ENTRY(sb->bc_entry_bin)),
             gtk_entry_get_text(GTK_ENTRY(sb->bc_entry_oct)),
             gtk_entry_get_text(GTK_ENTRY(sb->bc_entry_dec)),
             gtk_entry_get_text(GTK_ENTRY(sb->bc_entry_hex)));
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), buf, -1);
}

static void on_bc_clear(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    sb->bc_updating = true;
    gtk_entry_set_text(GTK_ENTRY(sb->bc_entry_bin), "0");
    gtk_entry_set_text(GTK_ENTRY(sb->bc_entry_oct), "0");
    gtk_entry_set_text(GTK_ENTRY(sb->bc_entry_dec), "0");
    gtk_entry_set_text(GTK_ENTRY(sb->bc_entry_hex), "0");
    sb->bc_updating = false;
}

/* =========================================================================
 * Find & Replace Callbacks
 * ========================================================================= */

static void do_find(Sidebars *sb, App *app, search_direction_t dir) {
    document_t *doc = app_get_active_document(app);
    if (!doc) {
        if (sb->find_status_lbl) gtk_label_set_text(GTK_LABEL(sb->find_status_lbl), "No active document");
        return;
    }

    const char *query = gtk_entry_get_text(GTK_ENTRY(sb->find_entry));
    if (!query || strlen(query) == 0) {
        if (sb->find_status_lbl) gtk_label_set_text(GTK_LABEL(sb->find_status_lbl), "Please enter search text");
        return;
    }

    search_type_t st = (search_type_t)gtk_combo_box_get_active(GTK_COMBO_BOX(sb->find_type_combo));
    uint8_t *pat = NULL;
    uint8_t *mask = NULL;
    size_t pat_len = 0;
    if (!search_parse_pattern_ex(query, st, doc->endian, &pat, &mask, &pat_len)) {
        if (sb->find_status_lbl) {
            if (st == SEARCH_TYPE_HEX) {
                gtk_label_set_markup(GTK_LABEL(sb->find_status_lbl),
                    "<span color='#e06c75'>Invalid hex! (Switch Type to 'ASCII String' to search text)</span>");
            } else {
                gtk_label_set_markup(GTK_LABEL(sb->find_status_lbl),
                    "<span color='#e06c75'>Invalid pattern for selected type</span>");
            }
        }
        return;
    }

    search_params_t sp;
    memset(&sp, 0, sizeof(sp));
    sp.type = st;
    sp.direction = dir;
    sp.endian = doc->endian;
    sp.match_case = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sb->find_chk_case));
    sp.wildcard_mask = mask;
    sp.regex_query = (st == SEARCH_TYPE_REGEX) ? query : NULL;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sb->find_chk_selection))) {
        sp.scope = doc->selection;
    }

    hxd_offset_t start;
    if (doc->selection.length > 0) {
        if (dir == SEARCH_DIR_FORWARD) {
            start = doc->selection.start + 1;
        } else {
            start = (doc->selection.start > 0) ? (doc->selection.start - 1) : 0;
        }
    } else {
        start = doc->cursor_offset;
    }

    hxd_offset_t found = search_find(doc->pt, start, pat, pat_len, &sp);
    bool wrapped = false;

    if (found == HXD_OFFSET_INVALID) {
        if (dir == SEARCH_DIR_FORWARD) {
            found = search_find(doc->pt, 0, pat, pat_len, &sp);
            if (found != HXD_OFFSET_INVALID) wrapped = true;
        } else {
            hxd_offset_t doc_size = pt_get_size(doc->pt);
            found = search_find(doc->pt, doc_size > 0 ? doc_size - 1 : 0, pat, pat_len, &sp);
            if (found != HXD_OFFSET_INVALID) wrapped = true;
        }
    }

    if (found != HXD_OFFSET_INVALID) {
        size_t match_len = (st == SEARCH_TYPE_REGEX && sp.matched_len > 0) ? sp.matched_len : pat_len;
        doc_set_cursor(doc, found, doc->cursor_in_ascii);
        doc_set_selection(doc, found, match_len);
        if (app->active_view) {
            hex_view_scroll_to_offset(app->active_view, found);
        }
        app_update_ui(app);

        if (sb->find_status_lbl) {
            char stat[128];
            if (wrapped) {
                snprintf(stat, sizeof(stat), "<span color='#61afef'>Wrapped: match at 0x%llX (%zu B)</span>",
                         (unsigned long long)found, match_len);
            } else {
                snprintf(stat, sizeof(stat), "<span color='#98c379'>Match at 0x%llX (%zu B)</span>",
                         (unsigned long long)found, match_len);
            }
            gtk_label_set_markup(GTK_LABEL(sb->find_status_lbl), stat);
        }
    } else {
        if (sb->find_status_lbl) {
            gtk_label_set_markup(GTK_LABEL(sb->find_status_lbl),
                "<span color='#e06c75'>Pattern not found</span>");
        }
    }

    free(pat);
    if (mask) free(mask);
}

static void on_find_next(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    do_find(sb, app, SEARCH_DIR_FORWARD);
}

static void on_find_prev(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    do_find(sb, app, SEARCH_DIR_BACKWARD);
}

static void on_find_entry_activate(GtkEntry *entry, gpointer user_data) {
    (void)entry;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    do_find(sb, app, SEARCH_DIR_FORWARD);
}

static gboolean on_find_entry_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");

    if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) {
        if (event->state & GDK_SHIFT_MASK) {
            do_find(sb, app, SEARCH_DIR_BACKWARD);
            return TRUE;
        }
    } else if (event->keyval == GDK_KEY_Escape) {
        if (app && app->active_view && app->active_view->drawing_area) {
            gtk_widget_grab_focus(app->active_view->drawing_area);
            return TRUE;
        }
    }
    return FALSE;
}

static void on_replace_entry_activate(GtkEntry *entry, gpointer user_data) {
    (void)entry;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    (void)app;
    on_find_next(NULL, sb);
}

static void on_find_all(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    const char *query = gtk_entry_get_text(GTK_ENTRY(sb->find_entry));
    if (!query || strlen(query) == 0) return;

    search_type_t st = (search_type_t)gtk_combo_box_get_active(GTK_COMBO_BOX(sb->find_type_combo));
    uint8_t *pat = NULL;
    uint8_t *mask = NULL;
    size_t pat_len = 0;
    if (!search_parse_pattern_ex(query, st, doc->endian, &pat, &mask, &pat_len)) {
        if (sb->find_status_lbl) {
            gtk_label_set_markup(GTK_LABEL(sb->find_status_lbl),
                "<span color='#e06c75'>Invalid pattern</span>");
        }
        return;
    }

    search_params_t sp;
    memset(&sp, 0, sizeof(sp));
    sp.type = st;
    sp.direction = SEARCH_DIR_FORWARD;
    sp.endian = doc->endian;
    sp.match_case = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sb->find_chk_case));
    sp.wildcard_mask = mask;
    sp.regex_query = (st == SEARCH_TYPE_REGEX) ? query : NULL;

    search_results_t *res = search_find_all(doc->pt, pat, pat_len, &sp);

    gtk_list_store_clear(sb->find_results_store);
    for (size_t i = 0; i < res->count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(sb->find_results_store, &iter);
        char off_str[32];
        snprintf(off_str, sizeof(off_str), "0x%08llX", (unsigned long long)res->offsets[i]);
        gtk_list_store_set(sb->find_results_store, &iter, 0, off_str, 1, (guint64)res->offsets[i], -1);
    }

    if (sb->find_status_lbl) {
        char stat[128];
        snprintf(stat, sizeof(stat), "<span color='#98c379'>Found %zu occurrence%s</span>",
                 res->count, res->count == 1 ? "" : "s");
        gtk_label_set_markup(GTK_LABEL(sb->find_status_lbl), stat);
    }

    search_results_free(res);
    free(pat);
    if (mask) free(mask);
}

static void on_find_result_selected(GtkTreeSelection *sel, gpointer user_data) {
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    GtkTreeModel *model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        guint64 off = 0;
        gtk_tree_model_get(model, &iter, 1, &off, -1);

        const char *query = gtk_entry_get_text(GTK_ENTRY(sb->find_entry));
        search_type_t st = (search_type_t)gtk_combo_box_get_active(GTK_COMBO_BOX(sb->find_type_combo));
        uint8_t *pat = NULL;
        size_t pat_len = 0;
        if (search_parse_pattern(query, st, doc->endian, &pat, &pat_len)) {
            doc_set_selection(doc, off, pat_len);
            free(pat);
        } else {
            doc_set_selection(doc, off, 1);
        }

        doc_set_cursor(doc, off, doc->cursor_in_ascii);
        if (app->active_view) {
            hex_view_scroll_to_offset(app->active_view, off);
        }
        app_update_ui(app);
    }
}

static void on_replace_click(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    const char *query = gtk_entry_get_text(GTK_ENTRY(sb->find_entry));
    const char *rep_text = gtk_entry_get_text(GTK_ENTRY(sb->replace_entry));
    if (!query || strlen(query) == 0) return;

    search_type_t st = (search_type_t)gtk_combo_box_get_active(GTK_COMBO_BOX(sb->find_type_combo));
    uint8_t *pat = NULL, *rep = NULL;
    size_t pat_len = 0, rep_len = 0;
    if (!search_parse_pattern(query, st, doc->endian, &pat, &pat_len)) return;
    if (!search_parse_pattern(rep_text, st, doc->endian, &rep, &rep_len)) {
        free(pat);
        return;
    }

    bool is_current_match = false;
    if (doc->selection.length == pat_len && pat_len > 0) {
        uint8_t *cur_bytes = malloc(pat_len);
        pt_read_bytes(doc->pt, doc->selection.start, cur_bytes, pat_len, NULL);
        if (memcmp(cur_bytes, pat, pat_len) == 0) {
            is_current_match = true;
        }
        free(cur_bytes);
    }

    if (is_current_match) {
        search_replace(doc->pt, doc->selection.start, doc->selection.length, rep, rep_len);
        doc->is_modified = true;
        doc_set_cursor(doc, doc->selection.start + rep_len, doc->cursor_in_ascii);
        doc_clear_selection(doc);
        if (sb->find_status_lbl) {
            gtk_label_set_markup(GTK_LABEL(sb->find_status_lbl),
                "<span color='#98c379'>Replaced 1 occurrence</span>");
        }
    }

    free(pat);
    free(rep);

    // Find next
    do_find(sb, app, SEARCH_DIR_FORWARD);
    app_update_ui(app);
}

static void on_replace_all_click(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    const char *query = gtk_entry_get_text(GTK_ENTRY(sb->find_entry));
    const char *rep_text = gtk_entry_get_text(GTK_ENTRY(sb->replace_entry));
    search_type_t st = (search_type_t)gtk_combo_box_get_active(GTK_COMBO_BOX(sb->find_type_combo));

    uint8_t *pat = NULL, *rep = NULL;
    size_t pat_len = 0, rep_len = 0;
    if (!search_parse_pattern(query, st, doc->endian, &pat, &pat_len) ||
        !search_parse_pattern(rep_text, st, doc->endian, &rep, &rep_len)) {
        if (pat) free(pat);
        if (rep) free(rep);
        if (sb->find_status_lbl) {
            gtk_label_set_markup(GTK_LABEL(sb->find_status_lbl),
                "<span color='#e06c75'>Invalid pattern for Replace All</span>");
        }
        return;
    }

    search_params_t sp;
    memset(&sp, 0, sizeof(sp));
    sp.type = st;
    sp.direction = SEARCH_DIR_FORWARD;
    sp.endian = doc->endian;
    sp.match_case = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sb->find_chk_case));

    size_t count = search_replace_all(doc->pt, pat, pat_len, rep, rep_len, &sp);
    free(pat);
    free(rep);

    if (count > 0) {
        doc->is_modified = true;
        app_update_ui(app);
    }

    if (sb->find_status_lbl) {
        char stat[128];
        snprintf(stat, sizeof(stat), "<span color='#98c379'>Replaced %zu occurrence%s</span>",
                 count, count == 1 ? "" : "s");
        gtk_label_set_markup(GTK_LABEL(sb->find_status_lbl), stat);
    }
}


/* =========================================================================
 * History Panel Callbacks
 * ========================================================================= */

static void on_history_selected(GtkTreeSelection *sel, gpointer user_data) {
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    GtkTreeModel *model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        int idx = 0;
        gtk_tree_model_get(model, &iter, 3, &idx, -1);
        doc_jump_to_history(doc, idx);
        app_update_ui(app);
    }
}

/* =========================================================================
 * Selection Panel Callbacks
 * ========================================================================= */

static uint64_t parse_sidebar_offset(const char *text, uint64_t def_val) {
    if (!text) return def_val;
    while (*text == ' ' || *text == '\t') text++;
    if (*text == '\0') return def_val;

    char *endptr = NULL;
    uint64_t val = 0;
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        val = strtoull(text, &endptr, 16);
    } else {
        size_t len = strlen(text);
        if (len > 0 && (text[len - 1] == 'h' || text[len - 1] == 'H')) {
            val = strtoull(text, &endptr, 16);
        } else {
            val = strtoull(text, &endptr, 10);
        }
    }
    return (endptr && endptr > text) ? val : def_val;
}

static void apply_sidebar_selection(Sidebars *sb) {
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    if (!app) return;
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    hxd_size_t doc_size = pt_get_size(doc->pt);
    uint64_t start = parse_sidebar_offset(gtk_entry_get_text(GTK_ENTRY(sb->sel_start_hex)), doc->selection.start);
    uint64_t end = parse_sidebar_offset(gtk_entry_get_text(GTK_ENTRY(sb->sel_end_hex)), start);
    uint64_t len = 0;
    if (end >= start) {
        len = end - start + 1;
    } else {
        len = parse_sidebar_offset(gtk_entry_get_text(GTK_ENTRY(sb->sel_len_hex)), 1);
    }

    if (start > doc_size) start = doc_size;
    if (start + len > doc_size) len = (doc_size > start) ? (doc_size - start) : 0;

    doc_set_cursor(doc, start, false);
    doc_set_selection(doc, start, len);
    if (app->active_view) {
        hex_view_scroll_to_offset(app->active_view, start);
    }
    app_update_ui(app);
}

static void on_sel_apply_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    apply_sidebar_selection((Sidebars *)user_data);
}

static void on_sel_entry_activate(GtkEntry *entry, gpointer user_data) {
    (void)entry;
    apply_sidebar_selection((Sidebars *)user_data);
}

static void on_sel_dialog_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    if (app) {
        app_show_select_range_dialog(app);
    }
}

static void on_sel_all_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    if (!app) return;
    document_t *doc = app_get_active_document(app);
    if (!doc) return;
    doc_select_all(doc);
    app_update_ui(app);
}

static void on_sel_none_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    if (!app) return;
    document_t *doc = app_get_active_document(app);
    if (!doc) return;
    doc_clear_selection(doc);
    app_update_ui(app);
}

static void on_sel_invert_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    if (!app) return;
    document_t *doc = app_get_active_document(app);
    if (!doc) return;
    doc_invert_selection(doc);
    app_update_ui(app);
}

/* =========================================================================
 * Export Panel Callbacks
 * ========================================================================= */

static void on_export_execute(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    hxd_range_t range = doc->selection.length > 0 ? doc->selection : (hxd_range_t){0, 0};

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sb->export_radio_file))) {
        GtkWidget *dialog = gtk_file_chooser_dialog_new(
            "Export to Binary File",
            GTK_WINDOW(app->window),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Save", GTK_RESPONSE_ACCEPT,
            NULL
        );
        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            export_to_binary_file(doc->pt, filename, range);
            g_free(filename);
        }
        gtk_widget_destroy(dialog);
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sb->export_radio_url))) {
        char *url = export_to_data_url(doc->pt, range);
        gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), url, -1);
        free(url);

        GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app->window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Data URL copied to clipboard!");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sb->export_radio_snippet))) {
        export_snippet_lang_t lang = (export_snippet_lang_t)gtk_combo_box_get_active(GTK_COMBO_BOX(sb->export_combo_snippet_lang));
        int bpr = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sb->export_spin_bpr));
        char *snippet = export_to_snippet(doc->pt, range, lang, bpr);
        gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), snippet, -1);
        free(snippet);

        GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app->window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Code snippet copied to clipboard!");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sb->export_radio_ihex))) {
        GtkWidget *dialog = gtk_file_chooser_dialog_new(
            "Export Intel HEX",
            GTK_WINDOW(app->window),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Save", GTK_RESPONSE_ACCEPT,
            NULL
        );
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "export.hex");
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            ihex_export_file(doc->pt, filename, range, IHEX_FMT_I32HEX, 16);
            g_free(filename);
        }
        gtk_widget_destroy(dialog);
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sb->export_radio_srec))) {
        GtkWidget *dialog = gtk_file_chooser_dialog_new(
            "Export Motorola S-Record",
            GTK_WINDOW(app->window),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Save", GTK_RESPONSE_ACCEPT,
            NULL
        );
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "export.srec");
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            srec_export_file(doc->pt, filename, range, SREC_FMT_S19, "HXD", 16);
            g_free(filename);
        }
        gtk_widget_destroy(dialog);
    }
}

/* =========================================================================
 * Closable Tab & Collapse Callbacks
 * ========================================================================= */

static void on_di_tab_close_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->left_notebook), "app");
    sidebars_set_data_inspector_visible(sb, app, false);
}

static void on_bc_tab_close_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->left_notebook), "app");
    sidebars_set_base_converter_visible(sb, app, false);
}

static void on_left_collapse_btn_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->left_notebook), "app");
    sidebars_toggle_left_collapse(sb, app);
}

static void on_right_collapse_btn_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    Sidebars *sb = (Sidebars *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(sb->right_notebook), "app");
    sidebars_toggle_right_collapse(sb, app);
}

static GtkWidget *create_closable_tab_header(const char *title, GCallback on_close, gpointer user_data) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *lbl = gtk_label_new(title);
    gtk_box_pack_start(GTK_BOX(box), lbl, TRUE, TRUE, 0);

    GtkWidget *btn = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
    GtkWidget *img = gtk_image_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
    gtk_button_set_image(GTK_BUTTON(btn), img);
    gtk_widget_set_tooltip_text(btn, "Close Tab");
    g_signal_connect(btn, "clicked", on_close, user_data);
    gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 0);

    gtk_widget_show_all(box);
    return box;
}

/* =========================================================================
 * Sidebars Construction
 * ========================================================================= */

Sidebars *sidebars_create(App *app) {
    Sidebars *sb = calloc(1, sizeof(Sidebars));
    sb->di_visible = true;
    sb->bc_visible = true;
    sb->left_collapsed = true;
    sb->left_saved_position = 260;
    sb->right_collapsed = true;
    sb->right_saved_position = 320;

    // -------------------------------------------------------------------------
    // LEFT NOTEBOOK (Data Inspector, Base Converter, Disk Editor)
    // -------------------------------------------------------------------------
    sb->left_notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(sb->left_notebook), GTK_POS_TOP);
    g_object_set_data(G_OBJECT(sb->left_notebook), "app", app);

    // Collapse button on notebook tab bar
    GtkWidget *btn_col = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(btn_col), GTK_RELIEF_NONE);
    GtkWidget *col_img = gtk_image_new_from_icon_name("pan-start-symbolic", GTK_ICON_SIZE_MENU);
    gtk_button_set_image(GTK_BUTTON(btn_col), col_img);
    gtk_widget_set_tooltip_text(btn_col, "Collapse Sidebar");
    g_signal_connect(btn_col, "clicked", G_CALLBACK(on_left_collapse_btn_clicked), sb);
    gtk_notebook_set_action_widget(GTK_NOTEBOOK(sb->left_notebook), btn_col, GTK_PACK_END);
    gtk_widget_show_all(btn_col);

    // 1. Data Inspector Panel
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        sb->di_page = box;

        // Toolbar
        GtkWidget *tb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        sb->btn_di_endian = gtk_button_new_with_label("LE");
        gtk_widget_set_tooltip_text(sb->btn_di_endian, "Toggle Endianness (Little/Big)");
        g_signal_connect(sb->btn_di_endian, "clicked", G_CALLBACK(on_di_endian_toggled), sb);

        sb->btn_di_editing = gtk_check_button_new_with_label("Edit");
        gtk_widget_set_tooltip_text(sb->btn_di_editing, "Enable Inline Value Editing");
        g_signal_connect(sb->btn_di_editing, "toggled", G_CALLBACK(on_di_editing_toggled), sb);

        GtkWidget *btn_copy = gtk_button_new_with_label("Copy All");
        g_signal_connect(btn_copy, "clicked", G_CALLBACK(on_di_copy_all), sb);

        gtk_box_pack_start(GTK_BOX(tb), sb->btn_di_endian, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(tb), sb->btn_di_editing, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(tb), btn_copy, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), tb, FALSE, FALSE, 2);

        // TreeView
        sb->di_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        sb->di_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sb->di_store));

        GtkCellRenderer *r_type = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *col_type = gtk_tree_view_column_new_with_attributes("Type", r_type, "text", 0, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(sb->di_tree), col_type);

        GtkCellRenderer *r_val = gtk_cell_renderer_text_new();
        g_object_set(r_val, "editable", TRUE, NULL);
        g_signal_connect(r_val, "edited", G_CALLBACK(on_di_value_edited), sb);
        GtkTreeViewColumn *col_val = gtk_tree_view_column_new_with_attributes("Value", r_val, "text", 1, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(sb->di_tree), col_val);

        GtkCellRenderer *r_desc = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *col_desc = gtk_tree_view_column_new_with_attributes("Description", r_desc, "text", 2, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(sb->di_tree), col_desc);

        GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scroll), sb->di_tree);
        gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

        GtkWidget *di_tab = create_closable_tab_header("Data Inspector", G_CALLBACK(on_di_tab_close_clicked), sb);
        gtk_notebook_append_page(GTK_NOTEBOOK(sb->left_notebook), box, di_tab);
    }

    // 2. Base Converter Panel
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_container_set_border_width(GTK_CONTAINER(box), 8);
        sb->bc_page = box;

        GtkWidget *grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

        #define ADD_BC_ROW(label, entry_var, row) do { \
            gtk_grid_attach(GTK_GRID(grid), gtk_label_new(label), 0, row, 1, 1); \
            entry_var = gtk_entry_new(); \
            g_signal_connect(entry_var, "changed", G_CALLBACK(on_bc_entry_changed), sb); \
            gtk_grid_attach(GTK_GRID(grid), entry_var, 1, row, 1, 1); \
        } while(0)

        ADD_BC_ROW("Binary:", sb->bc_entry_bin, 0);
        ADD_BC_ROW("Octal:", sb->bc_entry_oct, 1);
        ADD_BC_ROW("Decimal:", sb->bc_entry_dec, 2);
        ADD_BC_ROW("Hex:", sb->bc_entry_hex, 3);
        #undef ADD_BC_ROW

        gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 0);

        sb->bc_chk_zeros = gtk_check_button_new_with_label("Preserve Leading Zeros");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sb->bc_chk_zeros), TRUE);
        gtk_box_pack_start(GTK_BOX(box), sb->bc_chk_zeros, FALSE, FALSE, 0);

        GtkWidget *h_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        GtkWidget *btn_copy = gtk_button_new_with_label("Copy All");
        g_signal_connect(btn_copy, "clicked", G_CALLBACK(on_bc_copy_all), sb);
        GtkWidget *btn_clear = gtk_button_new_with_label("Clear");
        g_signal_connect(btn_clear, "clicked", G_CALLBACK(on_bc_clear), sb);

        gtk_box_pack_start(GTK_BOX(h_box), btn_copy, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(h_box), btn_clear, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(box), h_box, FALSE, FALSE, 4);

        GtkWidget *bc_tab = create_closable_tab_header("Base Converter", G_CALLBACK(on_bc_tab_close_clicked), sb);
        gtk_notebook_append_page(GTK_NOTEBOOK(sb->left_notebook), box, bc_tab);
    }

    // -------------------------------------------------------------------------
    // RIGHT NOTEBOOK (Find, Selection, History, Export)
    // -------------------------------------------------------------------------
    sb->right_notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(sb->right_notebook), GTK_POS_TOP);
    g_object_set_data(G_OBJECT(sb->right_notebook), "app", app);

    GtkWidget *btn_rcol = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(btn_rcol), GTK_RELIEF_NONE);
    GtkWidget *rcol_img = gtk_image_new_from_icon_name("pan-end-symbolic", GTK_ICON_SIZE_MENU);
    gtk_button_set_image(GTK_BUTTON(btn_rcol), rcol_img);
    gtk_widget_set_tooltip_text(btn_rcol, "Collapse/Expand Right Sidebar");
    g_signal_connect(btn_rcol, "clicked", G_CALLBACK(on_right_collapse_btn_clicked), sb);
    gtk_notebook_set_action_widget(GTK_NOTEBOOK(sb->right_notebook), btn_rcol, GTK_PACK_END);
    gtk_widget_show_all(btn_rcol);

    // 1. Find & Replace
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_container_set_border_width(GTK_CONTAINER(box), 8);

        GtkWidget *grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

        gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Find:"), 0, 0, 1, 1);
        sb->find_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(sb->find_entry), "Search query...");
        g_signal_connect(sb->find_entry, "activate", G_CALLBACK(on_find_entry_activate), sb);
        g_signal_connect(sb->find_entry, "key-press-event", G_CALLBACK(on_find_entry_key_press), sb);
        gtk_grid_attach(GTK_GRID(grid), sb->find_entry, 1, 0, 1, 1);

        gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Replace:"), 0, 1, 1, 1);
        sb->replace_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(sb->replace_entry), "Replacement data...");
        g_signal_connect(sb->replace_entry, "activate", G_CALLBACK(on_replace_entry_activate), sb);
        gtk_grid_attach(GTK_GRID(grid), sb->replace_entry, 1, 1, 1, 1);

        gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Type:"), 0, 2, 1, 1);
        sb->find_type_combo = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "Hex");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "ASCII String");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "Unicode UTF-16");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "Int8");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "UInt8");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "Int16");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "UInt16");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "Int32");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "UInt32");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "Int64");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "UInt64");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "Float32");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "Float64");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "Binary");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "Regular Expression (RegEx)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->find_type_combo), "Wildcard Pattern (e.g. 48 89 ?? ?? )");
        gtk_combo_box_set_active(GTK_COMBO_BOX(sb->find_type_combo), 0);
        gtk_grid_attach(GTK_GRID(grid), sb->find_type_combo, 1, 2, 1, 1);

        gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 0);

        sb->find_chk_case = gtk_check_button_new_with_label("Match Case");
        sb->find_chk_selection = gtk_check_button_new_with_label("In Selection");
        gtk_box_pack_start(GTK_BOX(box), sb->find_chk_case, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sb->find_chk_selection, FALSE, FALSE, 0);

        GtkWidget *hb1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        GtkWidget *btn_fn = gtk_button_new_with_label("Find Next (F3)");
        g_signal_connect(btn_fn, "clicked", G_CALLBACK(on_find_next), sb);
        GtkWidget *btn_fp = gtk_button_new_with_label("Find Prev");
        g_signal_connect(btn_fp, "clicked", G_CALLBACK(on_find_prev), sb);
        gtk_box_pack_start(GTK_BOX(hb1), btn_fn, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hb1), btn_fp, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(box), hb1, FALSE, FALSE, 0);

        GtkWidget *hb2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        GtkWidget *btn_rep = gtk_button_new_with_label("Replace");
        g_signal_connect(btn_rep, "clicked", G_CALLBACK(on_replace_click), sb);
        GtkWidget *btn_rep_all = gtk_button_new_with_label("Replace All");
        g_signal_connect(btn_rep_all, "clicked", G_CALLBACK(on_replace_all_click), sb);
        gtk_box_pack_start(GTK_BOX(hb2), btn_rep, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hb2), btn_rep_all, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(box), hb2, FALSE, FALSE, 0);

        GtkWidget *btn_fa = gtk_button_new_with_label("Find All Occurrences");
        g_signal_connect(btn_fa, "clicked", G_CALLBACK(on_find_all), sb);
        gtk_box_pack_start(GTK_BOX(box), btn_fa, FALSE, FALSE, 0);

        sb->find_status_lbl = gtk_label_new("");
        gtk_label_set_xalign(GTK_LABEL(sb->find_status_lbl), 0.0f);
        gtk_label_set_line_wrap(GTK_LABEL(sb->find_status_lbl), TRUE);
        gtk_box_pack_start(GTK_BOX(box), sb->find_status_lbl, FALSE, FALSE, 2);

        // Results Tree
        sb->find_results_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_UINT64);
        sb->find_results_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sb->find_results_store));
        GtkCellRenderer *r_res = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_res = gtk_tree_view_column_new_with_attributes("Offset Match", r_res, "text", 0, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(sb->find_results_list), c_res);
        GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(sb->find_results_list));
        g_signal_connect(sel, "changed", G_CALLBACK(on_find_result_selected), sb);

        GtkWidget *sc = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(sc), sb->find_results_list);
        gtk_box_pack_start(GTK_BOX(box), sc, TRUE, TRUE, 0);

        gtk_notebook_append_page(GTK_NOTEBOOK(sb->right_notebook), box, gtk_label_new("Find"));
    }

    // 2. Selection Panel
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_container_set_border_width(GTK_CONTAINER(box), 8);

        GtkWidget *grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

        gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Start (Hex):"), 0, 0, 1, 1);
        sb->sel_start_hex = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), sb->sel_start_hex, 1, 0, 1, 1);

        gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Start (Dec):"), 0, 1, 1, 1);
        sb->sel_start_dec = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), sb->sel_start_dec, 1, 1, 1, 1);

        gtk_grid_attach(GTK_GRID(grid), gtk_label_new("End (Hex):"), 0, 2, 1, 1);
        sb->sel_end_hex = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), sb->sel_end_hex, 1, 2, 1, 1);

        gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Length (Hex):"), 0, 3, 1, 1);
        sb->sel_len_hex = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), sb->sel_len_hex, 1, 3, 1, 1);

        gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Length (Dec):"), 0, 4, 1, 1);
        sb->sel_len_dec = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), sb->sel_len_dec, 1, 4, 1, 1);

        gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 0);

        GtkWidget *hb_apply = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        GtkWidget *btn_apply = gtk_button_new_with_label("Apply Selection");
        g_signal_connect(btn_apply, "clicked", G_CALLBACK(on_sel_apply_clicked), sb);
        gtk_box_pack_start(GTK_BOX(hb_apply), btn_apply, TRUE, TRUE, 0);

        GtkWidget *btn_dialog = gtk_button_new_with_label("Select Range... (Ctrl+E)");
        g_signal_connect(btn_dialog, "clicked", G_CALLBACK(on_sel_dialog_clicked), sb);
        gtk_box_pack_start(GTK_BOX(hb_apply), btn_dialog, TRUE, TRUE, 0);

        gtk_box_pack_start(GTK_BOX(box), hb_apply, FALSE, FALSE, 0);

        GtkWidget *btn_all = gtk_button_new_with_label("Select All (Ctrl+A)");
        g_signal_connect(btn_all, "clicked", G_CALLBACK(on_sel_all_clicked), sb);
        GtkWidget *btn_none = gtk_button_new_with_label("Select None");
        g_signal_connect(btn_none, "clicked", G_CALLBACK(on_sel_none_clicked), sb);
        GtkWidget *btn_inv = gtk_button_new_with_label("Invert Selection");
        g_signal_connect(btn_inv, "clicked", G_CALLBACK(on_sel_invert_clicked), sb);

        gtk_box_pack_start(GTK_BOX(box), btn_all, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), btn_none, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), btn_inv, FALSE, FALSE, 0);

        g_signal_connect(sb->sel_start_hex, "activate", G_CALLBACK(on_sel_entry_activate), sb);
        g_signal_connect(sb->sel_start_dec, "activate", G_CALLBACK(on_sel_entry_activate), sb);
        g_signal_connect(sb->sel_end_hex, "activate", G_CALLBACK(on_sel_entry_activate), sb);
        g_signal_connect(sb->sel_len_hex, "activate", G_CALLBACK(on_sel_entry_activate), sb);
        g_signal_connect(sb->sel_len_dec, "activate", G_CALLBACK(on_sel_entry_activate), sb);

        gtk_notebook_append_page(GTK_NOTEBOOK(sb->right_notebook), box, gtk_label_new("Selection"));
    }

    // 3. History Panel
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

        sb->chk_history_group = gtk_check_button_new_with_label("Group Same Operations");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sb->chk_history_group), TRUE);
        gtk_box_pack_start(GTK_BOX(box), sb->chk_history_group, FALSE, FALSE, 4);

        sb->history_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
        sb->history_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sb->history_store));

        GtkCellRenderer *r_act = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_act = gtk_tree_view_column_new_with_attributes("Action", r_act, "text", 0, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(sb->history_tree), c_act);

        GtkCellRenderer *r_off = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_off = gtk_tree_view_column_new_with_attributes("Offset", r_off, "text", 1, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(sb->history_tree), c_off);

        GtkCellRenderer *r_len = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_len = gtk_tree_view_column_new_with_attributes("Length", r_len, "text", 2, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(sb->history_tree), c_len);

        GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(sb->history_tree));
        g_signal_connect(sel, "changed", G_CALLBACK(on_history_selected), sb);

        GtkWidget *sc = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(sc), sb->history_tree);
        gtk_box_pack_start(GTK_BOX(box), sc, TRUE, TRUE, 0);

        gtk_notebook_append_page(GTK_NOTEBOOK(sb->right_notebook), box, gtk_label_new("History"));
    }

    // 4. Export Panel
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_container_set_border_width(GTK_CONTAINER(box), 8);

        sb->export_radio_file = gtk_radio_button_new_with_label(NULL, "Binary File (.bin)");
        sb->export_radio_url = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(sb->export_radio_file), "Data URL (Base64)");
        sb->export_radio_snippet = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(sb->export_radio_file), "Code Snippet");
        sb->export_radio_ihex = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(sb->export_radio_file), "Intel HEX (.hex)");
        sb->export_radio_srec = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(sb->export_radio_file), "Motorola S-Record (.srec)");

        gtk_box_pack_start(GTK_BOX(box), sb->export_radio_file, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sb->export_radio_url, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sb->export_radio_snippet, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sb->export_radio_ihex, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sb->export_radio_srec, FALSE, FALSE, 0);

        sb->export_combo_snippet_lang = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->export_combo_snippet_lang), "C / C++ Array");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->export_combo_snippet_lang), "Hex Dump View");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->export_combo_snippet_lang), "Python bytes()");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->export_combo_snippet_lang), "JavaScript Uint8Array");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->export_combo_snippet_lang), "Go []byte");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->export_combo_snippet_lang), "Rust &[u8]");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sb->export_combo_snippet_lang), "C# byte[]");
        gtk_combo_box_set_active(GTK_COMBO_BOX(sb->export_combo_snippet_lang), 0);
        gtk_box_pack_start(GTK_BOX(box), sb->export_combo_snippet_lang, FALSE, FALSE, 4);

        GtkWidget *bpr_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_box_pack_start(GTK_BOX(bpr_box), gtk_label_new("Bytes/Line:"), FALSE, FALSE, 0);
        sb->export_spin_bpr = gtk_spin_button_new_with_range(1, 64, 1);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(sb->export_spin_bpr), 16);
        gtk_box_pack_start(GTK_BOX(bpr_box), sb->export_spin_bpr, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(box), bpr_box, FALSE, FALSE, 4);

        GtkWidget *btn_exp = gtk_button_new_with_label("Export");
        g_signal_connect(btn_exp, "clicked", G_CALLBACK(on_export_execute), sb);
        gtk_box_pack_start(GTK_BOX(box), btn_exp, FALSE, FALSE, 6);

        gtk_notebook_append_page(GTK_NOTEBOOK(sb->right_notebook), box, gtk_label_new("Export"));
    }

    return sb;
}

void sidebars_update(Sidebars *sb, App *app) {
    if (!sb || !app) return;
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    // 1. Update Data Inspector
    inspector_results_t insp;
    inspector_compute(doc->pt, doc->cursor_offset, doc->endian, false, &insp);

    gtk_list_store_clear(sb->di_store);
    for (size_t i = 0; i < insp.count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(sb->di_store, &iter);
        gtk_list_store_set(sb->di_store, &iter,
                           0, insp.entries[i].name,
                           1, insp.entries[i].is_valid ? insp.entries[i].value : "--",
                           2, insp.entries[i].description,
                           3, insp.entries[i].id,
                           -1);
    }

    // 2. Update Base Converter
    bool is_gap = false;
    uint8_t byte_val = pt_get_byte(doc->pt, doc->cursor_offset, &is_gap);
    base_converter_state_t bc;
    base_converter_init(&bc);
    bc.preserve_leading_zeros = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sb->bc_chk_zeros));
    base_converter_update_from_uint64(&bc, byte_val, 8);

    sb->bc_updating = true;
    gtk_entry_set_text(GTK_ENTRY(sb->bc_entry_bin), bc.bin);
    gtk_entry_set_text(GTK_ENTRY(sb->bc_entry_oct), bc.oct);
    gtk_entry_set_text(GTK_ENTRY(sb->bc_entry_dec), bc.dec);
    gtk_entry_set_text(GTK_ENTRY(sb->bc_entry_hex), bc.hex);
    sb->bc_updating = false;

    // 3. Update Selection
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%08llX", (unsigned long long)doc->selection.start);
    gtk_entry_set_text(GTK_ENTRY(sb->sel_start_hex), buf);
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)doc->selection.start);
    gtk_entry_set_text(GTK_ENTRY(sb->sel_start_dec), buf);

    hxd_offset_t end_off = doc->selection.start + (doc->selection.length > 0 ? doc->selection.length - 1 : 0);
    snprintf(buf, sizeof(buf), "0x%08llX", (unsigned long long)end_off);
    gtk_entry_set_text(GTK_ENTRY(sb->sel_end_hex), buf);

    snprintf(buf, sizeof(buf), "0x%llX", (unsigned long long)doc->selection.length);
    gtk_entry_set_text(GTK_ENTRY(sb->sel_len_hex), buf);
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)doc->selection.length);
    gtk_entry_set_text(GTK_ENTRY(sb->sel_len_dec), buf);

    // 4. Update History
    gtk_list_store_clear(sb->history_store);
    int hcount = history_get_count(doc->history);
    for (int i = 0; i < hcount; i++) {
        const history_entry_t *entry = history_get_entry(doc->history, i);
        if (!entry) continue;

        GtkTreeIter iter;
        gtk_list_store_append(sb->history_store, &iter);

        char off_str[32], len_str[32];
        snprintf(off_str, sizeof(off_str), "0x%08llX", (unsigned long long)entry->offset);
        snprintf(len_str, sizeof(len_str), "%llu B", (unsigned long long)entry->length);

        gtk_list_store_set(sb->history_store, &iter,
                           0, entry->description,
                           1, off_str,
                           2, len_str,
                           3, i,
                           -1);
    }
}

void sidebars_set_data_inspector_visible(Sidebars *sb, App *app, bool visible) {
    if (!sb) return;
    sb->di_visible = visible;
    if (sb->di_page) {
        if (visible) {
            gtk_widget_show(sb->di_page);
            int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(sb->left_notebook), sb->di_page);
            if (page_num >= 0) {
                gtk_notebook_set_current_page(GTK_NOTEBOOK(sb->left_notebook), page_num);
            }
            if (sb->left_collapsed && app && app->paned_h1) {
                sidebars_toggle_left_collapse(sb, app);
            }
        } else {
            gtk_widget_hide(sb->di_page);
            if (!sb->bc_visible && !sb->left_collapsed && app && app->paned_h1) {
                sidebars_toggle_left_collapse(sb, app);
            }
        }
    }
}

void sidebars_set_base_converter_visible(Sidebars *sb, App *app, bool visible) {
    if (!sb) return;
    sb->bc_visible = visible;
    if (sb->bc_page) {
        if (visible) {
            gtk_widget_show(sb->bc_page);
            int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(sb->left_notebook), sb->bc_page);
            if (page_num >= 0) {
                gtk_notebook_set_current_page(GTK_NOTEBOOK(sb->left_notebook), page_num);
            }
            if (sb->left_collapsed && app && app->paned_h1) {
                sidebars_toggle_left_collapse(sb, app);
            }
        } else {
            gtk_widget_hide(sb->bc_page);
            if (!sb->di_visible && !sb->left_collapsed && app && app->paned_h1) {
                sidebars_toggle_left_collapse(sb, app);
            }
        }
    }
}

void sidebars_toggle_left_collapse(Sidebars *sb, App *app) {
    if (!sb || !app || !app->paned_h1) return;

    if (!sb->left_collapsed) {
        // Collapse
        int cur_pos = gtk_paned_get_position(GTK_PANED(app->paned_h1));
        if (cur_pos > 30) sb->left_saved_position = cur_pos;
        sb->left_collapsed = true;
        gtk_paned_set_position(GTK_PANED(app->paned_h1), 0);
    } else {
        // Restore / Uncollapse
        if (!sb->di_visible && !sb->bc_visible) {
            // Restore at least one tab if all were closed
            sidebars_set_data_inspector_visible(sb, app, true);
        }
        sb->left_collapsed = false;
        int target_pos = (sb->left_saved_position > 50) ? sb->left_saved_position : 260;
        gtk_paned_set_position(GTK_PANED(app->paned_h1), target_pos);
    }
}

bool sidebars_is_left_collapsed(Sidebars *sb) {
    return sb ? sb->left_collapsed : false;
}

void sidebars_ensure_right_expanded(Sidebars *sb, App *app) {
    if (!sb || !app || !app->paned_h2) return;

    if (sb->right_collapsed) {
        sb->right_collapsed = false;
        int alloc_w = gtk_widget_get_allocated_width(app->paned_h2);
        if (alloc_w <= 100) {
            alloc_w = 900;
        }
        int target_pos;
        if (sb->right_saved_position > 100 && sb->right_saved_position < alloc_w - 150) {
            target_pos = sb->right_saved_position;
        } else {
            target_pos = (alloc_w > 500) ? (alloc_w - 340) : (alloc_w * 2 / 3);
        }
        if (target_pos < 100) target_pos = 100;
        gtk_paned_set_position(GTK_PANED(app->paned_h2), target_pos);
    }
}

void sidebars_toggle_right_collapse(Sidebars *sb, App *app) {
    if (!sb || !app || !app->paned_h2) return;

    if (!sb->right_collapsed) {
        // Collapse to right
        int cur_pos = gtk_paned_get_position(GTK_PANED(app->paned_h2));
        int alloc_w = gtk_widget_get_allocated_width(app->paned_h2);
        if (cur_pos > 50 && cur_pos < alloc_w - 50) {
            sb->right_saved_position = cur_pos;
        }
        sb->right_collapsed = true;
        gtk_paned_set_position(GTK_PANED(app->paned_h2), 99999);
    } else {
        // Restore / Expand
        sidebars_ensure_right_expanded(sb, app);
    }
}

bool sidebars_is_right_collapsed(Sidebars *sb) {
    return sb ? sb->right_collapsed : false;
}

void sidebars_find_next(Sidebars *sb, App *app) {
    if (!sb || !app) return;
    do_find(sb, app, SEARCH_DIR_FORWARD);
}

void sidebars_find_prev(Sidebars *sb, App *app) {
    if (!sb || !app) return;
    do_find(sb, app, SEARCH_DIR_BACKWARD);
}
