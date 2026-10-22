#include "bottombar.h"
#include "app.h"
#include "../core/checksums.h"
#include "../core/statistics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Checksums Callbacks
 * ========================================================================= */

static void calculate_checksums(BottomBar *bb, App *app) {
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    hxd_range_t range = {0, 0};
    int scope_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(bb->cs_scope_combo));
    if (scope_idx == 1 && doc->selection.length > 0) {
        range = doc->selection;
    }

    checksum_list_t *cs = checksums_calculate(doc->pt, range, doc->endian);
    if (!cs) return;

    gtk_list_store_clear(bb->cs_store);
    for (size_t i = 0; i < cs->count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(bb->cs_store, &iter);
        gtk_list_store_set(bb->cs_store, &iter,
                           0, cs->items[i].name,
                           1, bb->cs_display_hex ? cs->items[i].result_hex : cs->items[i].result_dec,
                           2, cs->items[i].description,
                           -1);
    }

    checksums_free(cs);
}

static void on_cs_refresh_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    BottomBar *bb = (BottomBar *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(bb->notebook), "app");
    calculate_checksums(bb, app);
}

static void on_cs_copy_all(GtkButton *btn, gpointer user_data) {
    (void)btn;
    BottomBar *bb = (BottomBar *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(bb->notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    hxd_range_t range = {0, 0};
    checksum_list_t *cs = checksums_calculate(doc->pt, range, doc->endian);
    if (!cs) return;

    GString *str = g_string_new(NULL);
    for (size_t i = 0; i < cs->count; i++) {
        g_string_append_printf(str, "%-24s: %s\n", cs->items[i].name, cs->items[i].result_hex);
    }
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), str->str, -1);
    g_string_free(str, TRUE);
    checksums_free(cs);
}

/* =========================================================================
 * Bookmarks Callbacks
 * ========================================================================= */

static void on_bm_add_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    BottomBar *bb = (BottomBar *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(bb->notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    doc_toggle_bookmark(doc, doc->cursor_offset, NULL);
    bottombar_update(bb, app);
}

static void on_bm_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data) {
    (void)tree;
    (void)col;
    BottomBar *bb = (BottomBar *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(bb->notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    GtkTreeIter iter;
    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(bb->bm_store), &iter, path)) {
        guint64 off = 0;
        gtk_tree_model_get(GTK_TREE_MODEL(bb->bm_store), &iter, 2, &off, -1);
        doc_set_cursor(doc, off, false);
        hex_view_scroll_to_offset(app->active_view, off);
        app_update_ui(app);
    }
}

/* =========================================================================
 * File Comparison Callbacks
 * ========================================================================= */

static void format_chunk_preview(piece_table_t *pt, hxd_offset_t off, hxd_size_t len, char *buf, size_t buf_sz) {
    if (!pt) {
        snprintf(buf, buf_sz, "-");
        return;
    }
    hxd_size_t total_sz = pt_get_size(pt);
    if (off >= total_sz) {
        snprintf(buf, buf_sz, "<EOF>");
        return;
    }

    uint8_t data[8];
    size_t to_read = (len < sizeof(data)) ? (size_t)len : sizeof(data);
    if (off + to_read > total_sz) {
        to_read = (size_t)(total_sz - off);
    }
    hxd_size_t read_bytes = pt_read_bytes(pt, off, data, to_read, NULL);

    if (read_bytes == 0) {
        snprintf(buf, buf_sz, "<EOF>");
        return;
    }

    if (len == 1 && read_bytes == 1) {
        uint8_t b = data[0];
        snprintf(buf, buf_sz, "%02X ('%c')", b, (b >= 32 && b <= 126) ? b : '.');
        return;
    }

    char hex_part[32] = {0};
    char ascii_part[16] = {0};
    size_t hex_pos = 0;
    size_t asc_pos = 0;

    for (size_t i = 0; i < (size_t)read_bytes && i < 4; i++) {
        hex_pos += snprintf(hex_part + hex_pos, sizeof(hex_part) - hex_pos, "%02X ", data[i]);
        uint8_t b = data[i];
        ascii_part[asc_pos++] = (b >= 32 && b <= 126) ? (char)b : '.';
    }
    ascii_part[asc_pos] = '\0';

    if (len > 4 || (size_t)read_bytes < len) {
        snprintf(buf, buf_sz, "%s.. [%s..]", hex_part, ascii_part);
    } else {
        snprintf(buf, buf_sz, "%s [%s]", hex_part, ascii_part);
    }
}

void bottombar_load_diff_chunks(BottomBar *bb,
                                document_t *doc1,
                                document_t *doc2,
                                const app_diff_chunk_t *chunks,
                                size_t chunk_count) {
    if (!bb || !bb->fc_diff_store) return;

    gtk_list_store_clear(bb->fc_diff_store);

    const char *name1 = (doc1 && doc1->display_name) ? doc1->display_name : "File 1";
    const char *name2 = (doc2 && doc2->display_name) ? doc2->display_name : "File 2";

    if (bb->fc_col_f1) gtk_tree_view_column_set_title(bb->fc_col_f1, name1);
    if (bb->fc_col_f2) gtk_tree_view_column_set_title(bb->fc_col_f2, name2);

    if (doc2 && doc2->filepath && bb->fc_entry_path) {
        gtk_entry_set_text(GTK_ENTRY(bb->fc_entry_path), doc2->filepath);
    }

    if (bb->fc_lbl_summary) {
        char sum_buf[256];
        if (chunk_count == 0) {
            snprintf(sum_buf, sizeof(sum_buf), "Files are 100%% identical (0 differences found).");
        } else {
            snprintf(sum_buf, sizeof(sum_buf),
                     "Found %zu difference%s between %s and %s",
                     chunk_count, chunk_count == 1 ? "" : "s", name1, name2);
        }
        gtk_label_set_text(GTK_LABEL(bb->fc_lbl_summary), sum_buf);
    }

    if (chunk_count == 0 || !chunks) {
        return;
    }

    bb->fc_diff_syncing = true;
    for (size_t i = 0; i < chunk_count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(bb->fc_diff_store, &iter);

        char num_str[32];
        snprintf(num_str, sizeof(num_str), "%zu", i + 1);

        char off_str[32];
        snprintf(off_str, sizeof(off_str), "0x%08llX", (unsigned long long)chunks[i].offset);

        char len_str[48];
        if (chunks[i].length >= 16) {
            snprintf(len_str, sizeof(len_str), "%llu bytes (0x%llX)",
                     (unsigned long long)chunks[i].length,
                     (unsigned long long)chunks[i].length);
        } else {
            snprintf(len_str, sizeof(len_str), "%llu byte%s",
                     (unsigned long long)chunks[i].length,
                     chunks[i].length == 1 ? "" : "s");
        }

        char p1_str[64] = {0};
        char p2_str[64] = {0};
        format_chunk_preview(doc1 ? doc1->pt : NULL, chunks[i].offset, chunks[i].length, p1_str, sizeof(p1_str));
        format_chunk_preview(doc2 ? doc2->pt : NULL, chunks[i].offset, chunks[i].length, p2_str, sizeof(p2_str));

        gtk_list_store_set(bb->fc_diff_store, &iter,
                           0, num_str,
                           1, off_str,
                           2, len_str,
                           3, p1_str,
                           4, p2_str,
                           5, (guint64)chunks[i].offset,
                           6, (guint64)chunks[i].length,
                           7, (guint)i,
                           -1);
    }
    bb->fc_diff_syncing = false;
}

void bottombar_select_diff_row(BottomBar *bb, size_t chunk_idx) {
    if (!bb || !bb->fc_diff_tree || !bb->fc_diff_store) return;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(bb->fc_diff_tree));
    if (!sel) return;

    GtkTreeModel *model = NULL;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        guint cur_idx = 0;
        gtk_tree_model_get(model, &iter, 7, &cur_idx, -1);
        if (cur_idx == (guint)chunk_idx) {
            return;
        }
    }

    bb->fc_diff_syncing = true;
    GtkTreePath *path = gtk_tree_path_new_from_indices((gint)chunk_idx, -1);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(bb->fc_diff_tree), path, NULL, FALSE);
    gtk_tree_path_free(path);
    bb->fc_diff_syncing = false;
}

static void on_fc_diff_selected(BottomBar *bb) {
    if (!bb || bb->fc_diff_syncing) return;
    App *app = (App *)g_object_get_data(G_OBJECT(bb->notebook), "app");
    if (!app) return;

    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(bb->fc_diff_tree));
    GtkTreeModel *model = NULL;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) {
        return;
    }

    guint64 off = 0;
    guint64 len = 0;
    guint chunk_idx = 0;
    gtk_tree_model_get(model, &iter,
                       5, &off,
                       6, &len,
                       7, &chunk_idx,
                       -1);

    int cur_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->doc_notebook));
    GtkWidget *page = (cur_page >= 0) ? gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->doc_notebook), cur_page) : NULL;
    app_diff_view_state_t *st = page ? g_object_get_data(G_OBJECT(page), "diff_state") : NULL;

    if (!st) {
        int total = gtk_notebook_get_n_pages(GTK_NOTEBOOK(app->doc_notebook));
        for (int i = 0; i < total; i++) {
            GtkWidget *p = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->doc_notebook), i);
            if (p) {
                app_diff_view_state_t *cand = g_object_get_data(G_OBJECT(p), "diff_state");
                if (cand) {
                    gtk_notebook_set_current_page(GTK_NOTEBOOK(app->doc_notebook), i);
                    st = cand;
                    break;
                }
            }
        }
    }

    if (st) {
        bb->fc_diff_syncing = true;
        app_diff_view_goto_chunk(st, (size_t)chunk_idx);
        bb->fc_diff_syncing = false;
    } else {
        document_t *doc = app_get_active_document(app);
        if (doc) {
            doc_set_cursor(doc, off, false);
            if (len > 0) {
                doc_set_selection(doc, off, len);
            }
            if (app->active_view) {
                hex_view_scroll_to_offset(app->active_view, off);
                hex_view_refresh(app->active_view);
            }
            app_update_ui(app);
        }
    }
}

static void on_fc_diff_cursor_changed(GtkTreeView *tree, gpointer user_data) {
    (void)tree;
    on_fc_diff_selected((BottomBar *)user_data);
}

static void on_fc_diff_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data) {
    (void)tree;
    (void)path;
    (void)col;
    on_fc_diff_selected((BottomBar *)user_data);
}

static gboolean on_fc_diff_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    if (event->button == 1) {
        on_fc_diff_selected((BottomBar *)user_data);
    }
    return FALSE;
}

static void on_fc_browse_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    BottomBar *bb = (BottomBar *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(bb->notebook), "app");

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select File to Compare",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );

    if (app && app->last_open_dir && g_file_test(app->last_open_dir, G_FILE_TEST_IS_DIR)) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), app->last_open_dir);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            if (app) app_remember_file_dir(app, filename);
            gtk_entry_set_text(GTK_ENTRY(bb->fc_entry_path), filename);
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_fc_compare_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    BottomBar *bb = (BottomBar *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(bb->notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    const char *cmp_path = gtk_entry_get_text(GTK_ENTRY(bb->fc_entry_path));
    if (!cmp_path || strlen(cmp_path) == 0) return;

    piece_table_t *cmp_pt = pt_open_file(cmp_path);
    if (!cmp_pt) return;

    hxd_size_t sz1 = pt_get_size(doc->pt);
    hxd_size_t sz2 = pt_get_size(cmp_pt);
    hxd_size_t max_sz = (sz1 > sz2) ? sz1 : sz2;

    size_t cap = 64;
    app_diff_chunk_t *chunks = malloc(cap * sizeof(app_diff_chunk_t));
    size_t chunk_count = 0;

    bool in_diff = false;
    hxd_offset_t d_start = 0;
    hxd_size_t d_len = 0;

    const size_t CHUNK = 4096;
    uint8_t b1[CHUNK], b2[CHUNK];
    hxd_offset_t cur = 0;

    while (cur < max_sz && chunk_count < 10000) {
        size_t n = (max_sz - cur < CHUNK) ? (size_t)(max_sz - cur) : CHUNK;
        hxd_size_t r1 = (cur < sz1) ? pt_read_bytes(doc->pt, cur, b1, n, NULL) : 0;
        hxd_size_t r2 = (cur < sz2) ? pt_read_bytes(cmp_pt, cur, b2, n, NULL) : 0;

        for (size_t i = 0; i < n; i++) {
            bool has1 = (cur + i < sz1 && i < (size_t)r1);
            bool has2 = (cur + i < sz2 && i < (size_t)r2);
            bool diff = (!has1 || !has2 || b1[i] != b2[i]);

            if (diff) {
                if (!in_diff) {
                    in_diff = true;
                    d_start = cur + i;
                    d_len = 1;
                } else {
                    d_len++;
                }
            } else {
                if (in_diff) {
                    if (chunk_count >= cap) {
                        cap *= 2;
                        chunks = realloc(chunks, cap * sizeof(app_diff_chunk_t));
                    }
                    chunks[chunk_count].offset = d_start;
                    chunks[chunk_count].length = d_len;
                    chunk_count++;
                    in_diff = false;
                    if (chunk_count >= 10000) break;
                }
            }
        }
        cur += n;
    }
    if (in_diff && chunk_count < 10000) {
        if (chunk_count >= cap) {
            cap *= 2;
            chunks = realloc(chunks, cap * sizeof(app_diff_chunk_t));
        }
        chunks[chunk_count].offset = d_start;
        chunks[chunk_count].length = d_len;
        chunk_count++;
    }

    document_t fake_doc2 = {0};
    fake_doc2.pt = cmp_pt;
    fake_doc2.display_name = (char *)cmp_path;
    fake_doc2.filepath = (char *)cmp_path;

    bottombar_load_diff_chunks(bb, doc, &fake_doc2, chunks, chunk_count);

    free(chunks);
    pt_free(cmp_pt);
}

/* =========================================================================
 * BottomBar Construction
 * ========================================================================= */

BottomBar *bottombar_create(App *app) {
    BottomBar *bb = calloc(1, sizeof(BottomBar));
    bb->notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(bb->notebook), GTK_POS_TOP);
    g_object_set_data(G_OBJECT(bb->notebook), "app", app);
    bb->cs_display_hex = true;

    // 1. Checksums Panel
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        GtkWidget *tb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        bb->cs_scope_combo = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(bb->cs_scope_combo), "Whole Document");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(bb->cs_scope_combo), "Selection Only");
        gtk_combo_box_set_active(GTK_COMBO_BOX(bb->cs_scope_combo), 0);
        gtk_box_pack_start(GTK_BOX(tb), bb->cs_scope_combo, FALSE, FALSE, 0);

        GtkWidget *btn_ref = gtk_button_new_with_label("Calculate");
        g_signal_connect(btn_ref, "clicked", G_CALLBACK(on_cs_refresh_clicked), bb);
        gtk_box_pack_start(GTK_BOX(tb), btn_ref, FALSE, FALSE, 0);

        GtkWidget *btn_cp = gtk_button_new_with_label("Copy All");
        g_signal_connect(btn_cp, "clicked", G_CALLBACK(on_cs_copy_all), bb);
        gtk_box_pack_start(GTK_BOX(tb), btn_cp, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(box), tb, FALSE, FALSE, 2);

        // Checksums Table
        bb->cs_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        bb->cs_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(bb->cs_store));

        GtkCellRenderer *r_name = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_name = gtk_tree_view_column_new_with_attributes("Algorithm", r_name, "text", 0, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->cs_tree), c_name);

        GtkCellRenderer *r_val = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_val = gtk_tree_view_column_new_with_attributes("Value", r_val, "text", 1, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->cs_tree), c_val);

        GtkCellRenderer *r_desc = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_desc = gtk_tree_view_column_new_with_attributes("Description", r_desc, "text", 2, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->cs_tree), c_desc);

        GtkWidget *sc = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(sc), bb->cs_tree);
        gtk_box_pack_start(GTK_BOX(box), sc, TRUE, TRUE, 0);

        gtk_notebook_append_page(GTK_NOTEBOOK(bb->notebook), box, gtk_label_new("Checksums"));
    }

    // 2. Bookmarks Panel
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        GtkWidget *tb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        GtkWidget *btn_add = gtk_button_new_with_label("Toggle Bookmark at Cursor");
        g_signal_connect(btn_add, "clicked", G_CALLBACK(on_bm_add_clicked), bb);
        gtk_box_pack_start(GTK_BOX(tb), btn_add, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), tb, FALSE, FALSE, 2);

        bb->bm_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64);
        bb->bm_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(bb->bm_store));

        GtkCellRenderer *r_off = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_off = gtk_tree_view_column_new_with_attributes("Offset", r_off, "text", 0, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->bm_tree), c_off);

        GtkCellRenderer *r_name = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_name = gtk_tree_view_column_new_with_attributes("Name", r_name, "text", 1, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->bm_tree), c_name);

        g_signal_connect(bb->bm_tree, "row-activated", G_CALLBACK(on_bm_row_activated), bb);

        GtkWidget *sc = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(sc), bb->bm_tree);
        gtk_box_pack_start(GTK_BOX(box), sc, TRUE, TRUE, 0);

        gtk_notebook_append_page(GTK_NOTEBOOK(bb->notebook), box, gtk_label_new("Bookmarks"));
    }

    // 3. Statistics Panel
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_container_set_border_width(GTK_CONTAINER(box), 6);

        // Left summary
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        bb->stat_lbl_total = gtk_label_new("Total Bytes: 0");
        bb->stat_lbl_entropy = gtk_label_new("Entropy: 0.0000 bits/byte");
        bb->stat_lbl_mean = gtk_label_new("Mean: 0.00");
        bb->stat_lbl_nulls = gtk_label_new("Null Bytes (0x00): 0");
        bb->stat_lbl_print = gtk_label_new("Printable ASCII: 0");
        bb->stat_lbl_control = gtk_label_new("Control Bytes: 0");
        bb->stat_lbl_high = gtk_label_new("High Bytes (0x80-FF): 0");

        gtk_label_set_xalign(GTK_LABEL(bb->stat_lbl_total), 0);
        gtk_label_set_xalign(GTK_LABEL(bb->stat_lbl_entropy), 0);
        gtk_label_set_xalign(GTK_LABEL(bb->stat_lbl_mean), 0);
        gtk_label_set_xalign(GTK_LABEL(bb->stat_lbl_nulls), 0);
        gtk_label_set_xalign(GTK_LABEL(bb->stat_lbl_print), 0);
        gtk_label_set_xalign(GTK_LABEL(bb->stat_lbl_control), 0);
        gtk_label_set_xalign(GTK_LABEL(bb->stat_lbl_high), 0);

        gtk_box_pack_start(GTK_BOX(vbox), bb->stat_lbl_total, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), bb->stat_lbl_entropy, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), bb->stat_lbl_mean, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), bb->stat_lbl_nulls, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), bb->stat_lbl_print, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), bb->stat_lbl_control, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), bb->stat_lbl_high, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, FALSE, 8);

        // Right byte histogram table
        bb->stat_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64);
        bb->stat_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(bb->stat_store));

        GtkCellRenderer *r_b = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_b = gtk_tree_view_column_new_with_attributes("Byte", r_b, "text", 0, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->stat_tree), c_b);

        GtkCellRenderer *r_cnt = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_cnt = gtk_tree_view_column_new_with_attributes("Count", r_cnt, "text", 1, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->stat_tree), c_cnt);

        GtkWidget *sc = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(sc), bb->stat_tree);
        gtk_box_pack_start(GTK_BOX(box), sc, TRUE, TRUE, 0);

        gtk_notebook_append_page(GTK_NOTEBOOK(bb->notebook), box, gtk_label_new("Statistics"));
    }

    // 4. File Comparison Panel
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_container_set_border_width(GTK_CONTAINER(box), 6);
        bb->fc_page = box;

        GtkWidget *hb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        bb->fc_entry_path = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(bb->fc_entry_path), "Path to compared file...");
        gtk_box_pack_start(GTK_BOX(hb), bb->fc_entry_path, TRUE, TRUE, 0);

        GtkWidget *btn_br = gtk_button_new_with_label("Browse...");
        g_signal_connect(btn_br, "clicked", G_CALLBACK(on_fc_browse_clicked), bb);
        gtk_box_pack_start(GTK_BOX(hb), btn_br, FALSE, FALSE, 0);

        GtkWidget *btn_cmp = gtk_button_new_with_label("Compare");
        g_signal_connect(btn_cmp, "clicked", G_CALLBACK(on_fc_compare_clicked), bb);
        gtk_box_pack_start(GTK_BOX(hb), btn_cmp, FALSE, FALSE, 0);

        GtkWidget *btn_full_diff = gtk_button_new_with_label("Full Diff Window...");
        g_signal_connect_swapped(btn_full_diff, "clicked", G_CALLBACK(app_show_diff_dialog), app);
        gtk_box_pack_start(GTK_BOX(hb), btn_full_diff, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(box), hb, FALSE, FALSE, 0);

        bb->fc_lbl_summary = gtk_label_new("No comparison performed yet.");
        gtk_widget_set_halign(bb->fc_lbl_summary, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(box), bb->fc_lbl_summary, FALSE, FALSE, 2);

        bb->fc_diff_store = gtk_list_store_new(8,
                                               G_TYPE_STRING,   // 0: Diff #
                                               G_TYPE_STRING,   // 1: Offset / Address
                                               G_TYPE_STRING,   // 2: Length
                                               G_TYPE_STRING,   // 3: File 1 snippet
                                               G_TYPE_STRING,   // 4: File 2 snippet
                                               G_TYPE_UINT64,   // 5: Raw offset
                                               G_TYPE_UINT64,   // 6: Raw length
                                               G_TYPE_UINT);    // 7: Chunk index
        bb->fc_diff_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(bb->fc_diff_store));
        gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(bb->fc_diff_tree), TRUE);
        gtk_tree_view_set_enable_search(GTK_TREE_VIEW(bb->fc_diff_tree), TRUE);
        gtk_tree_view_set_search_column(GTK_TREE_VIEW(bb->fc_diff_tree), 1);

        // Column 0: Diff #
        GtkCellRenderer *r_num = gtk_cell_renderer_text_new();
        g_object_set(r_num, "xalign", 0.5f, NULL);
        bb->fc_col_diff = gtk_tree_view_column_new_with_attributes("Diff #", r_num, "text", 0, NULL);
        gtk_tree_view_column_set_resizable(bb->fc_col_diff, TRUE);
        gtk_tree_view_column_set_min_width(bb->fc_col_diff, 60);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->fc_diff_tree), bb->fc_col_diff);

        // Column 1: Address
        GtkCellRenderer *r_off = gtk_cell_renderer_text_new();
        g_object_set(r_off, "family", "Monospace", NULL);
        bb->fc_col_off = gtk_tree_view_column_new_with_attributes("Address", r_off, "text", 1, NULL);
        gtk_tree_view_column_set_resizable(bb->fc_col_off, TRUE);
        gtk_tree_view_column_set_min_width(bb->fc_col_off, 120);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->fc_diff_tree), bb->fc_col_off);

        // Column 2: Length
        GtkCellRenderer *r_len = gtk_cell_renderer_text_new();
        bb->fc_col_len = gtk_tree_view_column_new_with_attributes("Length", r_len, "text", 2, NULL);
        gtk_tree_view_column_set_resizable(bb->fc_col_len, TRUE);
        gtk_tree_view_column_set_min_width(bb->fc_col_len, 120);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->fc_diff_tree), bb->fc_col_len);

        // Column 3: File 1
        GtkCellRenderer *r_orig = gtk_cell_renderer_text_new();
        g_object_set(r_orig, "family", "Monospace", NULL);
        bb->fc_col_f1 = gtk_tree_view_column_new_with_attributes("Active File", r_orig, "text", 3, NULL);
        gtk_tree_view_column_set_resizable(bb->fc_col_f1, TRUE);
        gtk_tree_view_column_set_min_width(bb->fc_col_f1, 160);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->fc_diff_tree), bb->fc_col_f1);

        // Column 4: File 2
        GtkCellRenderer *r_cmp = gtk_cell_renderer_text_new();
        g_object_set(r_cmp, "family", "Monospace", NULL);
        bb->fc_col_f2 = gtk_tree_view_column_new_with_attributes("Compared File", r_cmp, "text", 4, NULL);
        gtk_tree_view_column_set_resizable(bb->fc_col_f2, TRUE);
        gtk_tree_view_column_set_min_width(bb->fc_col_f2, 160);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->fc_diff_tree), bb->fc_col_f2);

        g_signal_connect(bb->fc_diff_tree, "cursor-changed", G_CALLBACK(on_fc_diff_cursor_changed), bb);
        g_signal_connect(bb->fc_diff_tree, "row-activated", G_CALLBACK(on_fc_diff_activated), bb);
        g_signal_connect(bb->fc_diff_tree, "button-release-event", G_CALLBACK(on_fc_diff_button_release), bb);

        GtkWidget *sc = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sc), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(sc), bb->fc_diff_tree);
        gtk_box_pack_start(GTK_BOX(box), sc, TRUE, TRUE, 0);

        gtk_notebook_append_page(GTK_NOTEBOOK(bb->notebook), box, gtk_label_new("File Comparison"));
    }

    return bb;
}

void bottombar_update(BottomBar *bb, App *app) {
    if (!bb || !app) return;
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    // 1. Update Bookmarks
    gtk_list_store_clear(bb->bm_store);
    for (size_t i = 0; i < doc->bookmark_count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(bb->bm_store, &iter);

        char off_str[32];
        snprintf(off_str, sizeof(off_str), "0x%08llX", (unsigned long long)doc->bookmarks[i].offset);
        gtk_list_store_set(bb->bm_store, &iter,
                           0, off_str,
                           1, doc->bookmarks[i].name,
                           2, (guint64)doc->bookmarks[i].offset,
                           -1);
    }

    // 2. Update Statistics
    hxd_range_t full = {0, 0};
    statistics_result_t st;
    statistics_calculate(doc->pt, full, &st);

    char buf[128];
    snprintf(buf, sizeof(buf), "Total Bytes: %llu", (unsigned long long)st.total_bytes);
    gtk_label_set_text(GTK_LABEL(bb->stat_lbl_total), buf);

    snprintf(buf, sizeof(buf), "Entropy: %.4f bits/byte", st.entropy);
    gtk_label_set_text(GTK_LABEL(bb->stat_lbl_entropy), buf);

    snprintf(buf, sizeof(buf), "Mean: %.2f", st.mean);
    gtk_label_set_text(GTK_LABEL(bb->stat_lbl_mean), buf);

    snprintf(buf, sizeof(buf), "Null Bytes (0x00): %llu (%.1f%%)", (unsigned long long)st.null_bytes, st.total_bytes ? (st.null_bytes * 100.0 / st.total_bytes) : 0);
    gtk_label_set_text(GTK_LABEL(bb->stat_lbl_nulls), buf);

    snprintf(buf, sizeof(buf), "Printable ASCII: %llu (%.1f%%)", (unsigned long long)st.printable_bytes, st.total_bytes ? (st.printable_bytes * 100.0 / st.total_bytes) : 0);
    gtk_label_set_text(GTK_LABEL(bb->stat_lbl_print), buf);

    snprintf(buf, sizeof(buf), "Control Bytes: %llu (%.1f%%)", (unsigned long long)st.control_bytes, st.total_bytes ? (st.control_bytes * 100.0 / st.total_bytes) : 0);
    gtk_label_set_text(GTK_LABEL(bb->stat_lbl_control), buf);

    snprintf(buf, sizeof(buf), "High Bytes (0x80-FF): %llu (%.1f%%)", (unsigned long long)st.high_bytes, st.total_bytes ? (st.high_bytes * 100.0 / st.total_bytes) : 0);
    gtk_label_set_text(GTK_LABEL(bb->stat_lbl_high), buf);

    gtk_list_store_clear(bb->stat_store);
    for (int i = 0; i < 256; i++) {
        if (st.counts[i] > 0) {
            GtkTreeIter iter;
            gtk_list_store_append(bb->stat_store, &iter);
            char b_str[32], cnt_str[32];
            snprintf(b_str, sizeof(b_str), "0x%02X ('%c')", i, (i >= 32 && i <= 126) ? (char)i : '.');
            snprintf(cnt_str, sizeof(cnt_str), "%llu", (unsigned long long)st.counts[i]);
            gtk_list_store_set(bb->stat_store, &iter, 0, b_str, 1, cnt_str, 2, (guint64)st.counts[i], -1);
        }
    }
}
