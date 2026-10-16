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

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(GTK_ENTRY(bb->fc_entry_path), filename);
        g_free(filename);
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

    gtk_list_store_clear(bb->fc_diff_store);

    hxd_size_t sz1 = pt_get_size(doc->pt);
    hxd_size_t sz2 = pt_get_size(cmp_pt);
    hxd_size_t max_sz = (sz1 > sz2) ? sz1 : sz2;

    uint8_t b1[4096], b2[4096];
    hxd_offset_t off = 0;
    size_t diff_count = 0;

    while (off < max_sz && diff_count < 1000) {
        hxd_size_t chunk = sizeof(b1);
        if (off + chunk > max_sz) chunk = max_sz - off;

        hxd_size_t r1 = (off < sz1) ? pt_read_bytes(doc->pt, off, b1, chunk, NULL) : 0;
        hxd_size_t r2 = (off < sz2) ? pt_read_bytes(cmp_pt, off, b2, chunk, NULL) : 0;

        for (hxd_size_t i = 0; i < chunk; i++) {
            uint8_t byte1 = (i < r1) ? b1[i] : 0;
            uint8_t byte2 = (i < r2) ? b2[i] : 0;
            bool exist1 = (off + i < sz1);
            bool exist2 = (off + i < sz2);

            if (byte1 != byte2 || exist1 != exist2) {
                GtkTreeIter iter;
                gtk_list_store_append(bb->fc_diff_store, &iter);

                char off_str[32], b1_str[16], b2_str[16];
                snprintf(off_str, sizeof(off_str), "0x%08llX", (unsigned long long)(off + i));
                if (exist1) snprintf(b1_str, sizeof(b1_str), "%02X ('%c')", byte1, (byte1 >= 32 && byte1 <= 126) ? byte1 : '.');
                else strcpy(b1_str, "<EOF>");

                if (exist2) snprintf(b2_str, sizeof(b2_str), "%02X ('%c')", byte2, (byte2 >= 32 && byte2 <= 126) ? byte2 : '.');
                else strcpy(b2_str, "<EOF>");

                gtk_list_store_set(bb->fc_diff_store, &iter,
                                   0, off_str,
                                   1, b1_str,
                                   2, b2_str,
                                   3, (guint64)(off + i),
                                   -1);
                diff_count++;
                if (diff_count >= 1000) break;
            }
        }
        off += chunk;
    }

    pt_free(cmp_pt);
}

static void on_fc_diff_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data) {
    (void)tree;
    (void)col;
    BottomBar *bb = (BottomBar *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(bb->notebook), "app");
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    GtkTreeIter iter;
    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(bb->fc_diff_store), &iter, path)) {
        guint64 off = 0;
        gtk_tree_model_get(GTK_TREE_MODEL(bb->fc_diff_store), &iter, 3, &off, -1);
        doc_set_cursor(doc, off, false);
        hex_view_scroll_to_offset(app->active_view, off);
        app_update_ui(app);
    }
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

        gtk_box_pack_start(GTK_BOX(box), hb, FALSE, FALSE, 0);

        bb->fc_diff_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64);
        bb->fc_diff_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(bb->fc_diff_store));

        GtkCellRenderer *r_off = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_off = gtk_tree_view_column_new_with_attributes("Offset", r_off, "text", 0, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->fc_diff_tree), c_off);

        GtkCellRenderer *r_orig = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_orig = gtk_tree_view_column_new_with_attributes("Active File", r_orig, "text", 1, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->fc_diff_tree), c_orig);

        GtkCellRenderer *r_cmp = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c_cmp = gtk_tree_view_column_new_with_attributes("Compared File", r_cmp, "text", 2, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(bb->fc_diff_tree), c_cmp);

        g_signal_connect(bb->fc_diff_tree, "row-activated", G_CALLBACK(on_fc_diff_activated), bb);

        GtkWidget *sc = gtk_scrolled_window_new(NULL, NULL);
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
