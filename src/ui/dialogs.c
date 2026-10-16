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
