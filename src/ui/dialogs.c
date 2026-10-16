#include "dialogs.h"
#include "app.h"
#include "theme.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dialog_show_fill(App *app) {
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Fill Pattern",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_OK", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    char def_start[32], def_len[32];
    hxd_offset_t start = doc->selection.length > 0 ? doc->selection.start : doc->cursor_offset;
    hxd_size_t len = doc->selection.length > 0 ? doc->selection.length : 16;
    snprintf(def_start, sizeof(def_start), "0x%llX", (unsigned long long)start);
    snprintf(def_len, sizeof(def_len), "0x%llX", (unsigned long long)len);

    GtkWidget *entry_start = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_start), def_start);

    GtkWidget *entry_len = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_len), def_len);

    GtkWidget *entry_val = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_val), "00");

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Start Offset:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_start, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Length:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_len, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Fill Byte (Hex):"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_val, 1, 2, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dlg);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK) {
        hxd_offset_t off = strtoull(gtk_entry_get_text(GTK_ENTRY(entry_start)), NULL, 0);
        hxd_size_t l = strtoull(gtk_entry_get_text(GTK_ENTRY(entry_len)), NULL, 0);
        uint8_t byte_val = (uint8_t)strtoul(gtk_entry_get_text(GTK_ENTRY(entry_val)), NULL, 16);
        doc_fill(doc, off, l, byte_val);
        app_update_ui(app);
    }

    gtk_widget_destroy(dlg);
}

void dialog_show_insert(App *app) {
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Insert Block",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_OK", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    char def_start[32];
    snprintf(def_start, sizeof(def_start), "0x%llX", (unsigned long long)doc->cursor_offset);

    GtkWidget *entry_start = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_start), def_start);

    GtkWidget *entry_count = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_count), "16");

    GtkWidget *entry_val = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_val), "00");

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Insert At:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_start, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Count (Bytes):"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_count, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Fill Byte (Hex):"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_val, 1, 2, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dlg);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK) {
        hxd_offset_t off = strtoull(gtk_entry_get_text(GTK_ENTRY(entry_start)), NULL, 0);
        hxd_size_t cnt = strtoull(gtk_entry_get_text(GTK_ENTRY(entry_count)), NULL, 0);
        uint8_t byte_val = (uint8_t)strtoul(gtk_entry_get_text(GTK_ENTRY(entry_val)), NULL, 16);

        if (cnt > 0) {
            uint8_t *tmp = malloc(cnt);
            memset(tmp, byte_val, cnt);
            doc_insert(doc, off, tmp, cnt);
            free(tmp);
            app_update_ui(app);
        }
    }

    gtk_widget_destroy(dlg);
}

void dialog_show_gap(App *app) {
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Make Gap (Sparse Memory)",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_OK", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    char def_start[32], def_len[32];
    hxd_offset_t start = doc->selection.length > 0 ? doc->selection.start : doc->cursor_offset;
    hxd_size_t len = doc->selection.length > 0 ? doc->selection.length : 16;
    snprintf(def_start, sizeof(def_start), "0x%llX", (unsigned long long)start);
    snprintf(def_len, sizeof(def_len), "0x%llX", (unsigned long long)len);

    GtkWidget *entry_start = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_start), def_start);

    GtkWidget *entry_len = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_len), def_len);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Start Offset:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_start, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Gap Length:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_len, 1, 1, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dlg);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK) {
        hxd_offset_t off = strtoull(gtk_entry_get_text(GTK_ENTRY(entry_start)), NULL, 0);
        hxd_size_t l = strtoull(gtk_entry_get_text(GTK_ENTRY(entry_len)), NULL, 0);
        doc_make_gap(doc, off, l);
        app_update_ui(app);
    }

    gtk_widget_destroy(dlg);
}

void dialog_show_size(App *app) {
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Set Document Size",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_OK", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    char cur_sz_str[32];
    snprintf(cur_sz_str, sizeof(cur_sz_str), "0x%llX (%llu)", (unsigned long long)pt_get_size(doc->pt), (unsigned long long)pt_get_size(doc->pt));

    GtkWidget *lbl_cur = gtk_label_new(cur_sz_str);
    GtkWidget *entry_new = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_new), cur_sz_str);

    GtkWidget *entry_pad = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_pad), "00");

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Current Size:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_cur, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("New Size (Hex/Dec):"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_new, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Pad Byte (Hex):"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_pad, 1, 2, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dlg);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK) {
        hxd_size_t new_sz = strtoull(gtk_entry_get_text(GTK_ENTRY(entry_new)), NULL, 0);
        uint8_t pad = (uint8_t)strtoul(gtk_entry_get_text(GTK_ENTRY(entry_pad)), NULL, 16);
        doc_set_size(doc, new_sz, pad);
        app_update_ui(app);
    }

    gtk_widget_destroy(dlg);
}

void dialog_show_insert_file(App *app) {
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select File to Insert",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Insert", GTK_RESPONSE_ACCEPT,
        NULL
    );

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        doc_insert_file(doc, filename);
        g_free(filename);
        app_update_ui(app);
    }

    gtk_widget_destroy(dialog);
}

/* =========================================================================
 * Two-File Diff / Comparison Tool
 * ========================================================================= */

typedef struct {
    App           *app;
    GtkWidget     *dialog;
    GtkWidget     *combo_f1;
    GtkWidget     *entry_f1;
    GtkWidget     *combo_f2;
    GtkWidget     *entry_f2;
    GtkWidget     *lbl_status;
    GtkListStore  *diff_store;
    GtkWidget     *diff_tree;
    GtkTextBuffer *preview_buf1;
    GtkTextBuffer *preview_buf2;
    GtkWidget     *lbl_p1_title;
    GtkWidget     *lbl_p2_title;

    piece_table_t *pt1;
    piece_table_t *pt2;
    bool           owns_pt1;
    bool           owns_pt2;

    document_t    *doc1;
    document_t    *doc2;
} diff_dialog_state_t;

static void format_hex_preview(piece_table_t *pt, hxd_offset_t center_off, hxd_size_t diff_len, GtkTextBuffer *tbuf) {
    gtk_text_buffer_set_text(tbuf, "", 0);
    if (!pt) {
        gtk_text_buffer_set_text(tbuf, "<No file loaded>", -1);
        return;
    }

    hxd_size_t total_sz = pt_get_size(pt);
    if (total_sz == 0) {
        gtk_text_buffer_set_text(tbuf, "<Empty file>", -1);
        return;
    }

    hxd_offset_t row_start = (center_off >= 48) ? ((center_off - 48) / 16) * 16 : 0;
    hxd_offset_t row_end = row_start + 128; // 8 rows = 128 bytes
    if (row_end > total_sz) row_end = total_sz;

    GString *s = g_string_new(NULL);
    for (hxd_offset_t r = row_start; r < row_end; r += 16) {
        g_string_append_printf(s, "%08llX: ", (unsigned long long)r);
        uint8_t bytes[16];
        hxd_size_t read_cnt = pt_read_bytes(pt, r, bytes, 16, NULL);

        for (int i = 0; i < 16; i++) {
            if (i == 8) g_string_append(s, " ");
            if (i < (int)read_cnt) {
                hxd_offset_t b_off = r + i;
                bool is_diff = (b_off >= center_off && b_off < center_off + diff_len);
                if (is_diff) g_string_append_printf(s, "[%02X]", bytes[i]);
                else g_string_append_printf(s, "%02X ", bytes[i]);
            } else {
                g_string_append(s, "   ");
            }
        }

        g_string_append(s, " |");
        for (int i = 0; i < 16; i++) {
            if (i < (int)read_cnt) {
                uint8_t c = bytes[i];
                g_string_append_c(s, (c >= 32 && c <= 126) ? (char)c : '.');
            } else {
                g_string_append_c(s, ' ');
            }
        }
        g_string_append(s, "|\n");
    }

    gtk_text_buffer_set_text(tbuf, s->str, -1);
    g_string_free(s, TRUE);
}

static void append_diff_entry(diff_dialog_state_t *st, size_t index, hxd_offset_t off, hxd_size_t len, int type) {
    char off_str[32], len_str[32], type_str[32];
    snprintf(off_str, sizeof(off_str), "0x%08llX", (unsigned long long)off);
    snprintf(len_str, sizeof(len_str), "%llu byte%s", (unsigned long long)len, len == 1 ? "" : "s");

    if (type == 0) strcpy(type_str, "Mismatch");
    else if (type == 1) strcpy(type_str, "File 1 Extra (Past EOF)");
    else strcpy(type_str, "File 2 Extra (Past EOF)");

    uint8_t b1[8] = {0}, b2[8] = {0};
    hxd_size_t r1 = st->pt1 ? pt_read_bytes(st->pt1, off, b1, len < 8 ? len : 8, NULL) : 0;
    hxd_size_t r2 = st->pt2 ? pt_read_bytes(st->pt2, off, b2, len < 8 ? len : 8, NULL) : 0;

    char s1[64] = "", s2[64] = "";
    if (r1 > 0) {
        char *p = s1;
        for (hxd_size_t i = 0; i < r1; i++) p += sprintf(p, "%02X ", b1[i]);
        if (len > 8) p += sprintf(p, "... ");
        p += sprintf(p, "(\"");
        for (hxd_size_t i = 0; i < r1; i++) {
            uint8_t c = b1[i];
            *p++ = (c >= 32 && c <= 126) ? (char)c : '.';
        }
        sprintf(p, "\")");
    } else {
        strcpy(s1, "<EOF>");
    }

    if (r2 > 0) {
        char *p = s2;
        for (hxd_size_t i = 0; i < r2; i++) p += sprintf(p, "%02X ", b2[i]);
        if (len > 8) p += sprintf(p, "... ");
        p += sprintf(p, "(\"");
        for (hxd_size_t i = 0; i < r2; i++) {
            uint8_t c = b2[i];
            *p++ = (c >= 32 && c <= 126) ? (char)c : '.';
        }
        sprintf(p, "\")");
    } else {
        strcpy(s2, "<EOF>");
    }

    GtkTreeIter iter;
    gtk_list_store_append(st->diff_store, &iter);
    gtk_list_store_set(st->diff_store, &iter,
                       0, (gint)index,
                       1, off_str,
                       2, len_str,
                       3, type_str,
                       4, s1,
                       5, s2,
                       6, (guint64)off,
                       7, (guint64)len,
                       -1);
}

static void on_diff_selection_changed(GtkTreeSelection *sel, gpointer user_data) {
    diff_dialog_state_t *st = (diff_dialog_state_t *)user_data;
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        guint64 off = 0, len = 0;
        gtk_tree_model_get(model, &iter, 6, &off, 7, &len, -1);

        char p1_hdr[128], p2_hdr[128];
        snprintf(p1_hdr, sizeof(p1_hdr), "File 1 Preview (Diff @ 0x%08llX, %llu bytes):", (unsigned long long)off, (unsigned long long)len);
        snprintf(p2_hdr, sizeof(p2_hdr), "File 2 Preview (Diff @ 0x%08llX, %llu bytes):", (unsigned long long)off, (unsigned long long)len);
        gtk_label_set_text(GTK_LABEL(st->lbl_p1_title), p1_hdr);
        gtk_label_set_text(GTK_LABEL(st->lbl_p2_title), p2_hdr);

        format_hex_preview(st->pt1, off, len, st->preview_buf1);
        format_hex_preview(st->pt2, off, len, st->preview_buf2);
    }
}

static void on_diff_prev_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    diff_dialog_state_t *st = (diff_dialog_state_t *)user_data;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(st->diff_tree));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
        if (gtk_tree_path_prev(path)) {
            gtk_tree_selection_select_path(sel, path);
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(st->diff_tree), path, NULL, FALSE, 0, 0);
        }
        gtk_tree_path_free(path);
    }
}

static void on_diff_next_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    diff_dialog_state_t *st = (diff_dialog_state_t *)user_data;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(st->diff_tree));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        GtkTreeIter next_iter = iter;
        if (gtk_tree_model_iter_next(model, &next_iter)) {
            gtk_tree_selection_select_iter(sel, &next_iter);
            GtkTreePath *path = gtk_tree_model_get_path(model, &next_iter);
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(st->diff_tree), path, NULL, FALSE, 0, 0);
            gtk_tree_path_free(path);
        }
    } else {
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(st->diff_store), &iter)) {
            gtk_tree_selection_select_iter(sel, &iter);
        }
    }
}

static void on_diff_jump_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    diff_dialog_state_t *st = (diff_dialog_state_t *)user_data;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(st->diff_tree));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        guint64 off = 0, len = 0;
        gtk_tree_model_get(model, &iter, 6, &off, 7, &len, -1);

        document_t *doc = st->doc1 ? st->doc1 : app_get_active_document(st->app);
        if (doc && st->app && st->app->active_view) {
            doc_set_cursor(doc, off, false);
            doc_set_selection(doc, off, len);
            hex_view_scroll_to_offset(st->app->active_view, off);
            app_update_ui(st->app);
        }
    }
}

static void on_diff_browse_f1(GtkButton *btn, gpointer user_data) {
    (void)btn;
    diff_dialog_state_t *st = (diff_dialog_state_t *)user_data;
    GtkWidget *chooser = gtk_file_chooser_dialog_new(
        "Select File 1 (Reference)",
        GTK_WINDOW(st->dialog),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        gtk_entry_set_text(GTK_ENTRY(st->entry_f1), path);
        g_free(path);
    }
    gtk_widget_destroy(chooser);
}

static void on_diff_browse_f2(GtkButton *btn, gpointer user_data) {
    (void)btn;
    diff_dialog_state_t *st = (diff_dialog_state_t *)user_data;
    GtkWidget *chooser = gtk_file_chooser_dialog_new(
        "Select File 2 (Target)",
        GTK_WINDOW(st->dialog),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        gtk_entry_set_text(GTK_ENTRY(st->entry_f2), path);
        g_free(path);
    }
    gtk_widget_destroy(chooser);
}

static void on_diff_combo_f1_changed(GtkComboBox *combo, gpointer user_data) {
    diff_dialog_state_t *st = (diff_dialog_state_t *)user_data;
    int idx = gtk_combo_box_get_active(combo);
    int num_docs = g_list_length(st->app->hex_views);

    if (idx >= 0 && idx < num_docs) {
        HexView *v = (HexView *)g_list_nth_data(st->app->hex_views, idx);
        document_t *d = hex_view_get_document(v);
        const char *name = d->display_name ? d->display_name : (d->filepath ? d->filepath : "Untitled");
        gtk_entry_set_text(GTK_ENTRY(st->entry_f1), name);
    }
}

static void on_diff_combo_f2_changed(GtkComboBox *combo, gpointer user_data) {
    diff_dialog_state_t *st = (diff_dialog_state_t *)user_data;
    int idx = gtk_combo_box_get_active(combo);
    int num_docs = g_list_length(st->app->hex_views);

    if (idx >= 0 && idx < num_docs) {
        HexView *v = (HexView *)g_list_nth_data(st->app->hex_views, idx);
        document_t *d = hex_view_get_document(v);
        const char *name = d->display_name ? d->display_name : (d->filepath ? d->filepath : "Untitled");
        gtk_entry_set_text(GTK_ENTRY(st->entry_f2), name);
    }
}

static void on_diff_run_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    diff_dialog_state_t *st = (diff_dialog_state_t *)user_data;

    if (st->owns_pt1 && st->pt1) { pt_free(st->pt1); st->pt1 = NULL; }
    if (st->owns_pt2 && st->pt2) { pt_free(st->pt2); st->pt2 = NULL; }
    st->owns_pt1 = false;
    st->owns_pt2 = false;
    st->doc1 = NULL;
    st->doc2 = NULL;

    int idx1 = gtk_combo_box_get_active(GTK_COMBO_BOX(st->combo_f1));
    int idx2 = gtk_combo_box_get_active(GTK_COMBO_BOX(st->combo_f2));
    int num_docs = g_list_length(st->app->hex_views);

    // Resolve pt1
    if (idx1 >= 0 && idx1 < num_docs) {
        HexView *v = (HexView *)g_list_nth_data(st->app->hex_views, idx1);
        st->doc1 = hex_view_get_document(v);
        st->pt1 = st->doc1->pt;
        st->owns_pt1 = false;
    } else {
        const char *p1 = gtk_entry_get_text(GTK_ENTRY(st->entry_f1));
        if (!p1 || strlen(p1) == 0) {
            gtk_label_set_text(GTK_LABEL(st->lbl_status), "Error: File 1 path is empty.");
            return;
        }
        st->pt1 = pt_open_file(p1);
        st->owns_pt1 = true;
        if (!st->pt1) {
            gtk_label_set_text(GTK_LABEL(st->lbl_status), "Error: Failed to open File 1.");
            return;
        }
    }

    // Resolve pt2
    if (idx2 >= 0 && idx2 < num_docs) {
        HexView *v = (HexView *)g_list_nth_data(st->app->hex_views, idx2);
        st->doc2 = hex_view_get_document(v);
        st->pt2 = st->doc2->pt;
        st->owns_pt2 = false;
    } else {
        const char *p2 = gtk_entry_get_text(GTK_ENTRY(st->entry_f2));
        if (!p2 || strlen(p2) == 0) {
            gtk_label_set_text(GTK_LABEL(st->lbl_status), "Error: File 2 path is empty.");
            return;
        }
        st->pt2 = pt_open_file(p2);
        st->owns_pt2 = true;
        if (!st->pt2) {
            gtk_label_set_text(GTK_LABEL(st->lbl_status), "Error: Failed to open File 2.");
            return;
        }
    }

    gtk_list_store_clear(st->diff_store);

    hxd_size_t sz1 = pt_get_size(st->pt1);
    hxd_size_t sz2 = pt_get_size(st->pt2);
    hxd_size_t max_sz = (sz1 > sz2) ? sz1 : sz2;

    uint8_t b1[4096], b2[4096];
    hxd_offset_t off = 0;

    hxd_offset_t diff_start = 0;
    hxd_size_t   diff_len = 0;
    int          diff_type = 0;
    bool         in_diff = false;

    size_t diff_count = 0;
    hxd_size_t total_mismatched_bytes = 0;

    while (off < max_sz && diff_count < 10000) {
        hxd_size_t chunk = sizeof(b1);
        if (off + chunk > max_sz) chunk = max_sz - off;

        hxd_size_t r1 = (off < sz1) ? pt_read_bytes(st->pt1, off, b1, chunk, NULL) : 0;
        hxd_size_t r2 = (off < sz2) ? pt_read_bytes(st->pt2, off, b2, chunk, NULL) : 0;

        for (hxd_size_t i = 0; i < chunk; i++) {
            hxd_offset_t cur_off = off + i;
            bool ex1 = cur_off < sz1;
            bool ex2 = cur_off < sz2;
            uint8_t v1 = (i < r1) ? b1[i] : 0;
            uint8_t v2 = (i < r2) ? b2[i] : 0;

            bool differs = (v1 != v2 || ex1 != ex2);
            if (differs) {
                total_mismatched_bytes++;
                int cur_type = (!ex2) ? 1 : ((!ex1) ? 2 : 0);
                if (!in_diff) {
                    in_diff = true;
                    diff_start = cur_off;
                    diff_len = 1;
                    diff_type = cur_type;
                } else {
                    diff_len++;
                }
            } else {
                if (in_diff) {
                    append_diff_entry(st, diff_count + 1, diff_start, diff_len, diff_type);
                    diff_count++;
                    in_diff = false;
                    if (diff_count >= 10000) break;
                }
            }
        }
        off += chunk;
    }
    if (in_diff && diff_count < 10000) {
        append_diff_entry(st, diff_count + 1, diff_start, diff_len, diff_type);
        diff_count++;
    }

    double match_pct = 100.0;
    if (max_sz > 0) {
        match_pct = (1.0 - ((double)total_mismatched_bytes / (double)max_sz)) * 100.0;
        if (match_pct < 0.0) match_pct = 0.0;
    }

    char status_buf[256];
    if (diff_count == 0) {
        snprintf(status_buf, sizeof(status_buf),
                 "✔ <b>Files are completely identical!</b> (Size: %llu bytes, 100.0%% match)",
                 (unsigned long long)sz1);
    } else {
        snprintf(status_buf, sizeof(status_buf),
                 "Comparison finished: <b>%zu difference block%s</b> (%llu bytes differ) • Sizes: %llu vs %llu • <b>%.2f%% match</b>",
                 diff_count, diff_count == 1 ? "" : "s", (unsigned long long)total_mismatched_bytes,
                 (unsigned long long)sz1, (unsigned long long)sz2, match_pct);
    }
    gtk_label_set_markup(GTK_LABEL(st->lbl_status), status_buf);

    // Auto select first difference
    GtkTreeIter first_iter;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(st->diff_store), &first_iter)) {
        GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(st->diff_tree));
        gtk_tree_selection_select_iter(sel, &first_iter);
    }
}

void dialog_show_diff(App *app) {
    if (!app) return;

    diff_dialog_state_t *st = calloc(1, sizeof(diff_dialog_state_t));
    st->app = app;

    st->dialog = gtk_dialog_new_with_buttons(
        "File Comparison / Diff Tool",
        GTK_WINDOW(app->window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );
    gtk_window_set_default_size(GTK_WINDOW(st->dialog), 960, 680);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(st->dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 10);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(content), main_vbox);

    // 1. Header Grid for File 1 & File 2
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

    // File 1 controls
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("File 1 (Reference):"), 0, 0, 1, 1);
    st->combo_f1 = gtk_combo_box_text_new();
    st->entry_f1 = gtk_entry_new();
    GtkWidget *btn_br1 = gtk_button_new_with_label("Browse...");
    g_signal_connect(btn_br1, "clicked", G_CALLBACK(on_diff_browse_f1), st);
    g_signal_connect(st->combo_f1, "changed", G_CALLBACK(on_diff_combo_f1_changed), st);

    gtk_grid_attach(GTK_GRID(grid), st->combo_f1, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), st->entry_f1, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_br1, 3, 0, 1, 1);

    // File 2 controls
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("File 2 (Target):"), 0, 1, 1, 1);
    st->combo_f2 = gtk_combo_box_text_new();
    st->entry_f2 = gtk_entry_new();
    GtkWidget *btn_br2 = gtk_button_new_with_label("Browse...");
    g_signal_connect(btn_br2, "clicked", G_CALLBACK(on_diff_browse_f2), st);
    g_signal_connect(st->combo_f2, "changed", G_CALLBACK(on_diff_combo_f2_changed), st);

    gtk_grid_attach(GTK_GRID(grid), st->combo_f2, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), st->entry_f2, 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_br2, 3, 1, 1, 1);

    // Run Compare button
    GtkWidget *btn_run = gtk_button_new_with_label("Compare Files");
    GtkStyleContext *sc_btn = gtk_widget_get_style_context(btn_run);
    gtk_style_context_add_class(sc_btn, "suggested-action");
    g_signal_connect(btn_run, "clicked", G_CALLBACK(on_diff_run_clicked), st);
    gtk_grid_attach(GTK_GRID(grid), btn_run, 4, 0, 1, 2);

    gtk_box_pack_start(GTK_BOX(main_vbox), grid, FALSE, FALSE, 0);

    // Populate combo boxes
    int doc_count = g_list_length(app->hex_views);
    for (int i = 0; i < doc_count; i++) {
        HexView *v = (HexView *)g_list_nth_data(app->hex_views, i);
        document_t *d = hex_view_get_document(v);
        const char *name = d->display_name ? d->display_name : (d->filepath ? d->filepath : "Untitled");
        char item_txt[256];
        snprintf(item_txt, sizeof(item_txt), "Tab %d: %s", i + 1, name);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(st->combo_f1), item_txt);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(st->combo_f2), item_txt);
    }
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(st->combo_f1), "[Choose file from disk...]");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(st->combo_f2), "[Choose file from disk...]");

    if (doc_count > 0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(st->combo_f1), 0);
        if (doc_count > 1) gtk_combo_box_set_active(GTK_COMBO_BOX(st->combo_f2), 1);
        else gtk_combo_box_set_active(GTK_COMBO_BOX(st->combo_f2), doc_count);
    }

    // Status Banner
    st->lbl_status = gtk_label_new("Ready to compare. Select two files and click 'Compare Files'.");
    gtk_label_set_xalign(GTK_LABEL(st->lbl_status), 0.0);
    gtk_box_pack_start(GTK_BOX(main_vbox), st->lbl_status, FALSE, FALSE, 2);

    // Main Paned: Upper = Differences TreeView, Lower = Synchronized Hex Previews
    GtkWidget *vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_set_position(GTK_PANED(vpaned), 240);
    gtk_box_pack_start(GTK_BOX(main_vbox), vpaned, TRUE, TRUE, 0);

    // 2. Differences TreeView
    st->diff_store = gtk_list_store_new(8, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64);
    st->diff_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(st->diff_store));

    GtkCellRenderer *r_text = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col_num = gtk_tree_view_column_new_with_attributes("#", r_text, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(st->diff_tree), col_num);

    GtkTreeViewColumn *col_off = gtk_tree_view_column_new_with_attributes("Offset", r_text, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(st->diff_tree), col_off);

    GtkTreeViewColumn *col_len = gtk_tree_view_column_new_with_attributes("Length", r_text, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(st->diff_tree), col_len);

    GtkTreeViewColumn *col_type = gtk_tree_view_column_new_with_attributes("Type", r_text, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(st->diff_tree), col_type);

    GtkTreeViewColumn *col_f1 = gtk_tree_view_column_new_with_attributes("File 1 (Reference)", r_text, "text", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(st->diff_tree), col_f1);

    GtkTreeViewColumn *col_f2 = gtk_tree_view_column_new_with_attributes("File 2 (Target)", r_text, "text", 5, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(st->diff_tree), col_f2);

    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(st->diff_tree));
    g_signal_connect(sel, "changed", G_CALLBACK(on_diff_selection_changed), st);

    GtkWidget *sc_tree = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sc_tree), st->diff_tree);
    gtk_paned_pack1(GTK_PANED(vpaned), sc_tree, TRUE, FALSE);

    // 3. Lower Area: Navigation Toolbar & Side-by-Side Hex Previews
    GtkWidget *lower_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    GtkWidget *nav_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *btn_prev = gtk_button_new_with_label("◀ Previous Diff");
    g_signal_connect(btn_prev, "clicked", G_CALLBACK(on_diff_prev_clicked), st);
    GtkWidget *btn_next = gtk_button_new_with_label("Next Diff ▶");
    g_signal_connect(btn_next, "clicked", G_CALLBACK(on_diff_next_clicked), st);
    GtkWidget *btn_jump = gtk_button_new_with_label("Jump to Diff in Editor");
    g_signal_connect(btn_jump, "clicked", G_CALLBACK(on_diff_jump_clicked), st);

    gtk_box_pack_start(GTK_BOX(nav_bar), btn_prev, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(nav_bar), btn_next, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(nav_bar), btn_jump, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(lower_box), nav_bar, FALSE, FALSE, 2);

    // Previews Paned (Left = File 1, Right = File 2)
    GtkWidget *hpaned_preview = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_position(GTK_PANED(hpaned_preview), 470);

    // File 1 preview
    GtkWidget *vbox_p1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    st->lbl_p1_title = gtk_label_new("File 1 Preview:");
    gtk_label_set_xalign(GTK_LABEL(st->lbl_p1_title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox_p1), st->lbl_p1_title, FALSE, FALSE, 0);

    GtkWidget *tv_p1 = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv_p1), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv_p1), FALSE);
    st->preview_buf1 = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv_p1));
    GtkWidget *sc_p1 = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sc_p1), tv_p1);
    gtk_box_pack_start(GTK_BOX(vbox_p1), sc_p1, TRUE, TRUE, 0);
    gtk_paned_pack1(GTK_PANED(hpaned_preview), vbox_p1, TRUE, FALSE);

    // File 2 preview
    GtkWidget *vbox_p2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    st->lbl_p2_title = gtk_label_new("File 2 Preview:");
    gtk_label_set_xalign(GTK_LABEL(st->lbl_p2_title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox_p2), st->lbl_p2_title, FALSE, FALSE, 0);

    GtkWidget *tv_p2 = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv_p2), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv_p2), FALSE);
    st->preview_buf2 = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv_p2));
    GtkWidget *sc_p2 = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sc_p2), tv_p2);
    gtk_box_pack_start(GTK_BOX(vbox_p2), sc_p2, TRUE, TRUE, 0);
    gtk_paned_pack2(GTK_PANED(hpaned_preview), vbox_p2, TRUE, FALSE);

    gtk_box_pack_start(GTK_BOX(lower_box), hpaned_preview, TRUE, TRUE, 0);
    gtk_paned_pack2(GTK_PANED(vpaned), lower_box, TRUE, FALSE);

    gtk_widget_show_all(st->dialog);

    // Auto-run comparison if two documents are already open
    if (doc_count >= 2) {
        on_diff_run_clicked(NULL, st);
    }

    gtk_dialog_run(GTK_DIALOG(st->dialog));

    if (st->owns_pt1 && st->pt1) pt_free(st->pt1);
    if (st->owns_pt2 && st->pt2) pt_free(st->pt2);
    gtk_widget_destroy(st->dialog);
    free(st);
}

void dialog_show_settings(App *app) {
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Settings",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_OK", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 16);

    // Theme selector
    GtkWidget *combo_theme = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_theme), "Dark");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_theme), "Light");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_theme), "Black & White");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_theme), app->theme);

    // Column interlacing
    GtkWidget *chk_interlace = gtk_check_button_new_with_label("Enable Column Interlacing");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_interlace), app->column_interlacing);

    // Crosshair highlighting
    GtkWidget *chk_cross = gtk_check_button_new_with_label("Enable Current Row/Col Highlighting");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_cross), app->cross_highlighting);

    // Hex case
    GtkWidget *combo_case = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_case), "Uppercase (A-F)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_case), "Lowercase (a-f)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_case), app->hex_case);

    // Bytes per row
    GtkWidget *spin_bpr = gtk_spin_button_new_with_range(8, 64, 8);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_bpr), app->bytes_per_row);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Theme:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), combo_theme, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), chk_interlace, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), chk_cross, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Hex Case:"), 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), combo_case, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Bytes Per Row:"), 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), spin_bpr, 1, 4, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dlg);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK) {
        app->theme = (hxd_theme_t)gtk_combo_box_get_active(GTK_COMBO_BOX(combo_theme));
        app->column_interlacing = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_interlace));
        app->cross_highlighting = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_cross));
        app->hex_case = (hxd_hex_case_t)gtk_combo_box_get_active(GTK_COMBO_BOX(combo_case));
        app->bytes_per_row = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_bpr));

        theme_set(app->theme);
        if (app->active_view) {
            hex_view_set_column_interlacing(app->active_view, app->column_interlacing);
            hex_view_set_cross_highlighting(app->active_view, app->cross_highlighting);
            hex_view_set_hex_case(app->active_view, app->hex_case);
            hex_view_set_bytes_per_row(app->active_view, app->bytes_per_row);
        }
    }

    gtk_widget_destroy(dlg);
}

void dialog_show_controls(App *app) {
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Keyboard Shortcuts & Controls",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_window_set_default_size(GTK_WINDOW(dlg), 450, 400);

    GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

    GtkCellRenderer *r1 = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *c1 = gtk_tree_view_column_new_with_attributes("Shortcut", r1, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), c1);

    GtkCellRenderer *r2 = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *c2 = gtk_tree_view_column_new_with_attributes("Description", r2, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), c2);

    #define ADD_SC(key, desc) do { \
        GtkTreeIter iter; \
        gtk_list_store_append(store, &iter); \
        gtk_list_store_set(store, &iter, 0, key, 1, desc, -1); \
    } while(0)

    ADD_SC("Ctrl+N", "New Hex Document");
    ADD_SC("Ctrl+O", "Open Existing File");
    ADD_SC("Ctrl+S", "Save Current Document");
    ADD_SC("Ctrl+Shift+S", "Save As...");
    ADD_SC("Ctrl+W", "Close Active Document Tab");
    ADD_SC("Ctrl+Z", "Undo Last Action");
    ADD_SC("Ctrl+Y / Ctrl+Shift+Z", "Redo Action");
    ADD_SC("Ctrl+C", "Copy Selected Bytes / Snippet");
    ADD_SC("Ctrl+V", "Paste Data at Cursor");
    ADD_SC("Ctrl+F", "Open Find Panel");
    ADD_SC("Ctrl+H", "Open Replace Panel");
    ADD_SC("F3", "Find Next Occurrence");
    ADD_SC("Shift+F3", "Find Previous Occurrence");
    ADD_SC("Ctrl+A", "Select All Bytes");
    ADD_SC("Insert", "Toggle Overwrite (OVR) / Insert (INS) Mode");
    ADD_SC("Delete / Backspace", "Delete Byte or Selected Region");
    ADD_SC("Tab", "Switch Focus between Hex and ASCII Panes");
    ADD_SC("Arrow Keys", "Navigate Cursor (Nibble or Byte)");
    ADD_SC("Shift + Arrows", "Select Range of Bytes");
    ADD_SC("Page Up / Page Down", "Scroll by One Screen Page");
    ADD_SC("Home / End", "Jump to Start / End of Line");
    ADD_SC("Ctrl+Home / Ctrl+End", "Jump to Start / End of Document");
    #undef ADD_SC

    GtkWidget *sc = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sc), tree);
    gtk_box_pack_start(GTK_BOX(content), sc, TRUE, TRUE, 0);

    gtk_widget_show_all(dlg);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

void dialog_show_about(App *app) {
    GtkWidget *dlg = gtk_about_dialog_new();
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(app->window));
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dlg), "Hex Editor Neo (Offline Clone)");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dlg), "1.0.0");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dlg),
        "High-performance, cross-platform (Linux & Windows) offline GUI clone of the myhex.online web hex editor.\n\n"
        "Features:\n"
        "• Virtual Piece Table Engine (instant gigabyte file opening & zero lag)\n"
        "• Native Intel HEX & Motorola S-record parsing with sparse gap preservation\n"
        "• Full 25-type live Data Inspector with bi-directional editing\n"
        "• Real-time Base Converter (Hex, Dec, Oct, Bin)\n"
        "• Checksums & Cryptographic Hashes (CRC-8/16/32/64, MD5, SHA-1, SHA-256)\n"
        "• Pattern Find & Replace across Hex, Strings, Floats, and Integers\n"
        "• Shannon Entropy & Byte Frequency Histogram Statistics\n"
        "• File Comparison and Differencing Engine\n"
        "• Multi-format Export (Binary, Data URL Base64, C/C++ Snippets, Intel HEX, SREC)"
    );
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dlg), "https://myhex.online");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}
