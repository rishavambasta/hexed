#include "dialogs.h"
#include "app.h"
#include "theme.h"
#include "toolbar.h"
#include "../version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

    if (app->last_open_dir && g_file_test(app->last_open_dir, G_FILE_TEST_IS_DIR)) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), app->last_open_dir);
    } else if (doc->filepath) {
        char *dir = g_path_get_dirname(doc->filepath);
        if (dir && g_file_test(dir, G_FILE_TEST_IS_DIR)) {
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dir);
        }
        if (dir) g_free(dir);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            app_remember_file_dir(app, filename);
            doc_insert_file(doc, filename);
            g_free(filename);
            app_update_ui(app);
        }
    }

    gtk_widget_destroy(dialog);
}

/* =========================================================================
 * Select Range / Block Dialog
 * ========================================================================= */

typedef struct {
    App          *app;
    document_t   *doc;
    GtkWidget    *dlg;
    GtkWidget    *entry_start;
    GtkWidget    *radio_end;
    GtkWidget    *radio_len;
    GtkWidget    *entry_end;
    GtkWidget    *entry_len;
    GtkWidget    *chk_extend;
    GtkWidget    *lbl_summary;
    hxd_size_t    doc_size;
    hxd_range_t   existing_sel;
    bool          updating;
} select_range_dialog_state_t;

static uint64_t parse_offset_string(const char *text, uint64_t default_val) {
    if (!text) return default_val;
    while (*text && isspace((unsigned char)*text)) text++;
    if (!*text) return default_val;

    size_t len = strlen(text);
    if (len > 1 && (text[len - 1] == 'h' || text[len - 1] == 'H')) {
        char copy[64];
        if (len < sizeof(copy)) {
            memcpy(copy, text, len - 1);
            copy[len - 1] = '\0';
            return strtoull(copy, NULL, 16);
        }
    }
    return strtoull(text, NULL, 0);
}

static void select_range_update_sync(select_range_dialog_state_t *st) {
    if (st->updating) return;
    st->updating = true;

    uint64_t start = parse_offset_string(gtk_entry_get_text(GTK_ENTRY(st->entry_start)), 0);
    bool by_end = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st->radio_end));

    gtk_widget_set_sensitive(st->entry_end, by_end);
    gtk_widget_set_sensitive(st->entry_len, !by_end);

    uint64_t end = 0;
    uint64_t len = 0;

    if (by_end) {
        end = parse_offset_string(gtk_entry_get_text(GTK_ENTRY(st->entry_end)), start);
        if (end >= start) len = end - start + 1;
        else len = 1;

        char buf[64];
        snprintf(buf, sizeof(buf), "0x%llX (%llu)", (unsigned long long)len, (unsigned long long)len);
        gtk_entry_set_text(GTK_ENTRY(st->entry_len), buf);
    } else {
        len = parse_offset_string(gtk_entry_get_text(GTK_ENTRY(st->entry_len)), 1);
        if (len == 0) len = 1;
        end = start + len - 1;

        char buf[64];
        snprintf(buf, sizeof(buf), "0x%llX (%llu)", (unsigned long long)end, (unsigned long long)end);
        gtk_entry_set_text(GTK_ENTRY(st->entry_end), buf);
    }

    char sum_buf[256];
    snprintf(sum_buf, sizeof(sum_buf),
             "Target Range: <b>0x%08llX - 0x%08llX</b> (%llu bytes)",
             (unsigned long long)start, (unsigned long long)end, (unsigned long long)len);
    gtk_label_set_markup(GTK_LABEL(st->lbl_summary), sum_buf);

    st->updating = false;
}

static void on_sr_entry_changed(GtkEditable *editable, gpointer user_data) {
    (void)editable;
    select_range_dialog_state_t *st = (select_range_dialog_state_t *)user_data;
    select_range_update_sync(st);
}

static void on_sr_radio_toggled(GtkToggleButton *btn, gpointer user_data) {
    (void)btn;
    select_range_dialog_state_t *st = (select_range_dialog_state_t *)user_data;
    select_range_update_sync(st);
}

static void on_sr_preset_clicked(GtkButton *btn, gpointer user_data) {
    select_range_dialog_state_t *st = (select_range_dialog_state_t *)user_data;
    const char *label = gtk_button_get_label(btn);

    st->updating = true;
    if (strcmp(label, "0 to EOF") == 0) {
        gtk_entry_set_text(GTK_ENTRY(st->entry_start), "0x0");
        char buf[64];
        snprintf(buf, sizeof(buf), "0x%llX", (unsigned long long)(st->doc_size > 0 ? st->doc_size - 1 : 0));
        gtk_entry_set_text(GTK_ENTRY(st->entry_end), buf);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(st->radio_end), TRUE);
    } else if (strcmp(label, "Cursor to EOF") == 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "0x%llX", (unsigned long long)st->doc->cursor_offset);
        gtk_entry_set_text(GTK_ENTRY(st->entry_start), buf);
        snprintf(buf, sizeof(buf), "0x%llX", (unsigned long long)(st->doc_size > 0 ? st->doc_size - 1 : 0));
        gtk_entry_set_text(GTK_ENTRY(st->entry_end), buf);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(st->radio_end), TRUE);
    } else if (strcmp(label, "0 to Cursor") == 0) {
        gtk_entry_set_text(GTK_ENTRY(st->entry_start), "0x0");
        char buf[64];
        snprintf(buf, sizeof(buf), "0x%llX", (unsigned long long)st->doc->cursor_offset);
        gtk_entry_set_text(GTK_ENTRY(st->entry_end), buf);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(st->radio_end), TRUE);
    } else if (strcmp(label, "+256 B") == 0) {
        gtk_entry_set_text(GTK_ENTRY(st->entry_len), "256");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(st->radio_len), TRUE);
    } else if (strcmp(label, "+1 KB") == 0) {
        gtk_entry_set_text(GTK_ENTRY(st->entry_len), "1024");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(st->radio_len), TRUE);
    } else if (strcmp(label, "+4 KB") == 0) {
        gtk_entry_set_text(GTK_ENTRY(st->entry_len), "4096");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(st->radio_len), TRUE);
    } else if (strcmp(label, "+64 KB") == 0) {
        gtk_entry_set_text(GTK_ENTRY(st->entry_len), "65536");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(st->radio_len), TRUE);
    }
    st->updating = false;
    select_range_update_sync(st);
}

void dialog_show_select_range(App *app) {
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    select_range_dialog_state_t st;
    memset(&st, 0, sizeof(st));
    st.app = app;
    st.doc = doc;
    st.doc_size = pt_get_size(doc->pt);
    st.existing_sel = doc->selection;

    st.dlg = gtk_dialog_new_with_buttons(
        "Select Range / Block",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Select", GTK_RESPONSE_OK,
        NULL
    );
    gtk_dialog_set_default_response(GTK_DIALOG(st.dlg), GTK_RESPONSE_OK);
    gtk_window_set_default_size(GTK_WINDOW(st.dlg), 460, 360);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(st.dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    // Grid for Start, Mode (End Offset / Length)
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

    // Start Offset
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Start Offset:"), 0, 0, 1, 1);
    st.entry_start = gtk_entry_new();
    char start_str[32];
    hxd_offset_t init_start = (doc->selection.length > 0) ? doc->selection.start : doc->cursor_offset;
    snprintf(start_str, sizeof(start_str), "0x%llX", (unsigned long long)init_start);
    gtk_entry_set_text(GTK_ENTRY(st.entry_start), start_str);
    g_signal_connect(st.entry_start, "changed", G_CALLBACK(on_sr_entry_changed), &st);
    gtk_grid_attach(GTK_GRID(grid), st.entry_start, 1, 0, 1, 1);

    // End Offset radio + entry
    st.radio_end = gtk_radio_button_new_with_label(NULL, "End Offset:");
    gtk_grid_attach(GTK_GRID(grid), st.radio_end, 0, 1, 1, 1);
    st.entry_end = gtk_entry_new();
    char end_str[32];
    hxd_offset_t init_end = (doc->selection.length > 0) ? (doc->selection.start + doc->selection.length - 1) : init_start;
    snprintf(end_str, sizeof(end_str), "0x%llX", (unsigned long long)init_end);
    gtk_entry_set_text(GTK_ENTRY(st.entry_end), end_str);
    g_signal_connect(st.entry_end, "changed", G_CALLBACK(on_sr_entry_changed), &st);
    g_signal_connect(st.radio_end, "toggled", G_CALLBACK(on_sr_radio_toggled), &st);
    gtk_grid_attach(GTK_GRID(grid), st.entry_end, 1, 1, 1, 1);

    // Length radio + entry
    st.radio_len = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(st.radio_end), "Length (Bytes):");
    gtk_grid_attach(GTK_GRID(grid), st.radio_len, 0, 2, 1, 1);
    st.entry_len = gtk_entry_new();
    char len_str[64];
    hxd_size_t init_len = (doc->selection.length > 0) ? doc->selection.length : 1;
    snprintf(len_str, sizeof(len_str), "0x%llX (%llu)", (unsigned long long)init_len, (unsigned long long)init_len);
    gtk_entry_set_text(GTK_ENTRY(st.entry_len), len_str);
    g_signal_connect(st.entry_len, "changed", G_CALLBACK(on_sr_entry_changed), &st);
    g_signal_connect(st.radio_len, "toggled", G_CALLBACK(on_sr_radio_toggled), &st);
    gtk_grid_attach(GTK_GRID(grid), st.entry_len, 1, 2, 1, 1);

    gtk_box_pack_start(GTK_BOX(vbox), grid, FALSE, FALSE, 0);

    // Extend selection checkbox
    st.chk_extend = gtk_check_button_new_with_label("Extend existing selection");
    gtk_box_pack_start(GTK_BOX(vbox), st.chk_extend, FALSE, FALSE, 0);

    // Quick presets
    GtkWidget *lbl_presets = gtk_label_new("<b>Quick Presets:</b>");
    gtk_label_set_use_markup(GTK_LABEL(lbl_presets), TRUE);
    gtk_label_set_xalign(GTK_LABEL(lbl_presets), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_presets, FALSE, FALSE, 2);

    GtkWidget *hb_presets1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    #define ADD_PRESET(pbox, plabel) do { \
        GtkWidget *btn = gtk_button_new_with_label(plabel); \
        g_signal_connect(btn, "clicked", G_CALLBACK(on_sr_preset_clicked), &st); \
        gtk_box_pack_start(GTK_BOX(pbox), btn, TRUE, TRUE, 0); \
    } while(0)

    ADD_PRESET(hb_presets1, "0 to EOF");
    ADD_PRESET(hb_presets1, "0 to Cursor");
    ADD_PRESET(hb_presets1, "Cursor to EOF");
    gtk_box_pack_start(GTK_BOX(vbox), hb_presets1, FALSE, FALSE, 0);

    GtkWidget *hb_presets2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    ADD_PRESET(hb_presets2, "+256 B");
    ADD_PRESET(hb_presets2, "+1 KB");
    ADD_PRESET(hb_presets2, "+4 KB");
    ADD_PRESET(hb_presets2, "+64 KB");
    gtk_box_pack_start(GTK_BOX(vbox), hb_presets2, FALSE, FALSE, 0);
    #undef ADD_PRESET

    // Summary label
    st.lbl_summary = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(st.lbl_summary), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), st.lbl_summary, FALSE, FALSE, 4);

    select_range_update_sync(&st);

    gtk_widget_show_all(st.dlg);

    if (gtk_dialog_run(GTK_DIALOG(st.dlg)) == GTK_RESPONSE_OK) {
        uint64_t start = parse_offset_string(gtk_entry_get_text(GTK_ENTRY(st.entry_start)), 0);
        bool by_end = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st.radio_end));
        uint64_t len = 0;

        if (by_end) {
            uint64_t end = parse_offset_string(gtk_entry_get_text(GTK_ENTRY(st.entry_end)), start);
            if (end >= start) len = end - start + 1;
            else len = 1;
        } else {
            len = parse_offset_string(gtk_entry_get_text(GTK_ENTRY(st.entry_len)), 1);
            if (len == 0) len = 1;
        }

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st.chk_extend)) && st.existing_sel.length > 0) {
            uint64_t u_start = (start < st.existing_sel.start) ? start : st.existing_sel.start;
            uint64_t cur_end = start + len;
            uint64_t ex_end = st.existing_sel.start + st.existing_sel.length;
            uint64_t u_end = (cur_end > ex_end) ? cur_end : ex_end;
            start = u_start;
            len = u_end - u_start;
        }

        if (start > st.doc_size) start = st.doc_size;
        if (start + len > st.doc_size) len = st.doc_size - start;

        doc_set_cursor(doc, start, false);
        doc_set_selection(doc, start, len);
        if (app->active_view) {
            hex_view_scroll_to_offset(app->active_view, start);
        }
        app_update_ui(app);
    }

    gtk_widget_destroy(st.dlg);
}

/* =========================================================================
 * Goto / Jump to Address Dialog (Ctrl+G)
 * ========================================================================= */

typedef struct {
    App          *app;
    document_t   *doc;
    hxd_size_t    doc_size;
    hxd_offset_t  cursor_offset;
    GtkWidget    *dlg;
    GtkWidget    *entry_offset;
    GtkWidget    *radio_abs;
    GtkWidget    *radio_rel;
    GtkWidget    *radio_end;
    GtkWidget    *radio_hex;
    GtkWidget    *radio_dec;
    GtkWidget    *lbl_status;
} goto_dialog_state_t;

static void goto_dialog_update_preview(goto_dialog_state_t *st) {
    if (!st || !st->entry_offset || !st->lbl_status) return;
    const char *text = gtk_entry_get_text(GTK_ENTRY(st->entry_offset));
    bool is_dec = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st->radio_dec));

    uint64_t raw_val = 0;
    if (text && strlen(text) > 0) {
        if (is_dec) {
            raw_val = strtoull(text, NULL, 10);
        } else {
            raw_val = parse_offset_string(text, 0);
        }
    }

    int mode = 0; // 0 = abs, 1 = rel, 2 = end
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st->radio_rel))) mode = 1;
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st->radio_end))) mode = 2;

    int64_t target = 0;
    if (mode == 0) {
        target = (int64_t)raw_val;
    } else if (mode == 1) {
        target = (int64_t)st->cursor_offset + (int64_t)raw_val;
    } else {
        target = (int64_t)st->doc_size - (int64_t)raw_val;
    }

    bool out_of_bounds = false;
    if (target < 0) {
        target = 0;
        out_of_bounds = true;
    }
    if ((hxd_size_t)target > st->doc_size) {
        target = st->doc_size;
        out_of_bounds = true;
    }

    char stat_str[256];
    if (out_of_bounds) {
        snprintf(stat_str, sizeof(stat_str),
                 "<span color='#f59e0b'>Target (clamped): <b>0x%llX (%lld)</b></span> | File size: 0x%llX (%llu B)",
                 (unsigned long long)target, (long long)target,
                 (unsigned long long)st->doc_size, (unsigned long long)st->doc_size);
    } else {
        snprintf(stat_str, sizeof(stat_str),
                 "Target Offset: <b>0x%llX (%lld)</b> | File bounds: 0x0 .. 0x%llX",
                 (unsigned long long)target, (long long)target, (unsigned long long)st->doc_size);
    }
    gtk_label_set_markup(GTK_LABEL(st->lbl_status), stat_str);
}

static void on_goto_entry_changed(GtkEditable *editable, gpointer user_data) {
    (void)editable;
    goto_dialog_update_preview((goto_dialog_state_t *)user_data);
}

static int s_last_goto_mode = 0; // 0 = Absolute, 1 = Relative, 2 = End of file
static bool s_last_goto_dec = false;

static void on_goto_radio_toggled(GtkToggleButton *btn, gpointer user_data) {
    (void)btn;
    goto_dialog_state_t *st = (goto_dialog_state_t *)user_data;
    if (st) {
        if (st->radio_rel && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st->radio_rel))) {
            s_last_goto_mode = 1;
        } else if (st->radio_end && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st->radio_end))) {
            s_last_goto_mode = 2;
        } else if (st->radio_abs && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st->radio_abs))) {
            s_last_goto_mode = 0;
        }
        if (st->radio_dec) {
            s_last_goto_dec = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st->radio_dec));
        }
    }
    goto_dialog_update_preview(st);
}

void dialog_show_goto(App *app) {
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    goto_dialog_state_t st;
    memset(&st, 0, sizeof(st));
    st.app = app;
    st.doc = doc;
    st.doc_size = pt_get_size(doc->pt);
    st.cursor_offset = doc->cursor_offset;

    st.dlg = gtk_dialog_new_with_buttons(
        "Go to Address / Offset (Ctrl+G)",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Go To", GTK_RESPONSE_OK,
        NULL
    );
    gtk_dialog_set_default_response(GTK_DIALOG(st.dlg), GTK_RESPONSE_OK);
    gtk_window_set_default_size(GTK_WINDOW(st.dlg), 440, 260);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(st.dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    // Entry box
    GtkWidget *hb_entry = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(hb_entry), gtk_label_new("Address / Offset:"), FALSE, FALSE, 0);
    st.entry_offset = gtk_entry_new();
    char cur_str[32];
    if (s_last_goto_mode == 1) {
        snprintf(cur_str, sizeof(cur_str), "0");
    } else {
        snprintf(cur_str, sizeof(cur_str),
                 app->hex_case == HXD_HEX_LOWERCASE ? "0x%llx" : "0x%llX",
                 (unsigned long long)st.cursor_offset);
    }
    gtk_entry_set_text(GTK_ENTRY(st.entry_offset), cur_str);
    gtk_editable_select_region(GTK_EDITABLE(st.entry_offset), 0, -1);
    gtk_entry_set_activates_default(GTK_ENTRY(st.entry_offset), TRUE);
    g_signal_connect(st.entry_offset, "changed", G_CALLBACK(on_goto_entry_changed), &st);
    gtk_box_pack_start(GTK_BOX(hb_entry), st.entry_offset, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hb_entry, FALSE, FALSE, 0);

    // Number base frame
    GtkWidget *hb_base = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    st.radio_hex = gtk_radio_button_new_with_label(NULL, "Hexadecimal (0x / h)");
    st.radio_dec = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(st.radio_hex), "Decimal");
    if (s_last_goto_dec) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(st.radio_dec), TRUE);
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(st.radio_hex), TRUE);
    }
    g_signal_connect(st.radio_hex, "toggled", G_CALLBACK(on_goto_radio_toggled), &st);
    g_signal_connect(st.radio_dec, "toggled", G_CALLBACK(on_goto_radio_toggled), &st);
    gtk_box_pack_start(GTK_BOX(hb_base), st.radio_hex, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hb_base), st.radio_dec, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hb_base, FALSE, FALSE, 0);

    // Mode frame
    GtkWidget *f_mode = gtk_frame_new("Origin / Jump Mode");
    GtkWidget *vbox_mode = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox_mode), 6);
    gtk_container_add(GTK_CONTAINER(f_mode), vbox_mode);

    st.radio_abs = gtk_radio_button_new_with_label(NULL, "Beginning of file (Absolute)");
    st.radio_rel = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(st.radio_abs), "Current position (Relative +/-)");
    st.radio_end = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(st.radio_abs), "End of file (Backwards -)");

    if (s_last_goto_mode == 1) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(st.radio_rel), TRUE);
    } else if (s_last_goto_mode == 2) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(st.radio_end), TRUE);
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(st.radio_abs), TRUE);
    }

    g_signal_connect(st.radio_abs, "toggled", G_CALLBACK(on_goto_radio_toggled), &st);
    g_signal_connect(st.radio_rel, "toggled", G_CALLBACK(on_goto_radio_toggled), &st);
    g_signal_connect(st.radio_end, "toggled", G_CALLBACK(on_goto_radio_toggled), &st);
    gtk_box_pack_start(GTK_BOX(vbox_mode), st.radio_abs, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox_mode), st.radio_rel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox_mode), st.radio_end, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), f_mode, FALSE, FALSE, 0);

    // Status / target label
    st.lbl_status = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(st.lbl_status), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), st.lbl_status, FALSE, FALSE, 4);

    goto_dialog_update_preview(&st);

    gtk_widget_show_all(st.dlg);

    if (gtk_dialog_run(GTK_DIALOG(st.dlg)) == GTK_RESPONSE_OK) {
        const char *text = gtk_entry_get_text(GTK_ENTRY(st.entry_offset));
        bool is_dec = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st.radio_dec));
        s_last_goto_dec = is_dec;

        uint64_t raw_val = 0;
        if (text && strlen(text) > 0) {
            if (is_dec) raw_val = strtoull(text, NULL, 10);
            else raw_val = parse_offset_string(text, 0);
        }

        int mode = 0;
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st.radio_rel))) mode = 1;
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st.radio_end))) mode = 2;
        s_last_goto_mode = mode;

        int64_t target = 0;
        if (mode == 0) target = (int64_t)raw_val;
        else if (mode == 1) target = (int64_t)st.cursor_offset + (int64_t)raw_val;
        else target = (int64_t)st.doc_size - (int64_t)raw_val;

        if (target < 0) target = 0;
        if ((hxd_size_t)target > st.doc_size) target = st.doc_size;

        doc_set_cursor(doc, (hxd_offset_t)target, false);
        doc_clear_selection(doc);
        if (app->active_view) {
            hex_view_scroll_to_offset(app->active_view, (hxd_offset_t)target);
        }
        app_update_ui(app);
    }

    gtk_widget_destroy(st.dlg);
}

/* =========================================================================
 * Bitwise Operations Dialog (AND, OR, NOT, XOR)
 * ========================================================================= */

typedef struct {
    App        *app;
    document_t *doc;
    GtkWidget  *dlg;
    GtkWidget  *radio_not;
    GtkWidget  *radio_and;
    GtkWidget  *radio_or;
    GtkWidget  *radio_xor;
    GtkWidget  *entry_operand;
    GtkWidget  *lbl_operand_title;
    GtkWidget  *radio_sel;
    GtkWidget  *radio_all;
    GtkWidget  *lbl_preview;
} bitwise_dialog_state_t;

static void on_bw_preset_clicked(GtkButton *btn, gpointer user_data) {
    bitwise_dialog_state_t *st = (bitwise_dialog_state_t *)user_data;
    const char *lbl = gtk_button_get_label(btn);
    gtk_entry_set_text(GTK_ENTRY(st->entry_operand), lbl);
}

static void bitwise_dialog_update_preview(bitwise_dialog_state_t *st) {
    if (!st || !st->entry_operand || !st->lbl_preview) return;

    bool is_not = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st->radio_not));
    gtk_widget_set_sensitive(st->entry_operand, !is_not);

    bool use_sel = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st->radio_sel));
    hxd_offset_t off = use_sel ? st->doc->selection.start : 0;
    hxd_size_t total_sz = pt_get_size(st->doc->pt);
    hxd_size_t len = use_sel ? st->doc->selection.length : total_sz;
    if (off + len > total_sz) len = (total_sz > off) ? total_sz - off : 0;

    hxd_bitop_t op = HXD_BITOP_NOT;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st->radio_and))) op = HXD_BITOP_AND;
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st->radio_or))) op = HXD_BITOP_OR;
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st->radio_xor))) op = HXD_BITOP_XOR;

    uint8_t op_byte = 0;
    const char *op_txt = gtk_entry_get_text(GTK_ENTRY(st->entry_operand));
    if (op_txt && strlen(op_txt) > 0) {
        op_byte = (uint8_t)parse_offset_string(op_txt, 0);
    }

    size_t preview_cnt = (len < 8) ? (size_t)len : 8;
    uint8_t orig_sample[8], res_sample[8];
    memset(orig_sample, 0, sizeof(orig_sample));
    memset(res_sample, 0, sizeof(res_sample));

    if (preview_cnt > 0) {
        pt_read_bytes(st->doc->pt, off, orig_sample, preview_cnt, NULL);
        for (size_t i = 0; i < preview_cnt; i++) {
            switch (op) {
                case HXD_BITOP_NOT: res_sample[i] = (uint8_t)(~orig_sample[i]); break;
                case HXD_BITOP_AND: res_sample[i] = (uint8_t)(orig_sample[i] & op_byte); break;
                case HXD_BITOP_OR:  res_sample[i] = (uint8_t)(orig_sample[i] | op_byte); break;
                case HXD_BITOP_XOR: res_sample[i] = (uint8_t)(orig_sample[i] ^ op_byte); break;
            }
        }
    }

    char prev_str[512];
    char b1_str[64] = "", b2_str[64] = "";
    for (size_t i = 0; i < preview_cnt; i++) {
        char tmp[8];
        snprintf(tmp, sizeof(tmp), "%02X ", orig_sample[i]);
        strcat(b1_str, tmp);
        snprintf(tmp, sizeof(tmp), "%02X ", res_sample[i]);
        strcat(b2_str, tmp);
    }

    snprintf(prev_str, sizeof(prev_str),
             "Scope: <b>%llu bytes</b> at 0x%08llX\n"
             "Before: <tt>%s</tt>\n"
             "After:  <tt><b>%s</b></tt>",
             (unsigned long long)len, (unsigned long long)off,
             preview_cnt > 0 ? b1_str : "(Empty)",
             preview_cnt > 0 ? b2_str : "(Empty)");
    gtk_label_set_markup(GTK_LABEL(st->lbl_preview), prev_str);
}

static void on_bw_changed(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    bitwise_dialog_update_preview((bitwise_dialog_state_t *)user_data);
}

void dialog_show_bitwise_op(App *app) {
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    bitwise_dialog_state_t st;
    memset(&st, 0, sizeof(st));
    st.app = app;
    st.doc = doc;

    st.dlg = gtk_dialog_new_with_buttons(
        "Bitwise Operations (AND / OR / NOT / XOR)",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Apply", GTK_RESPONSE_OK,
        NULL
    );
    gtk_dialog_set_default_response(GTK_DIALOG(st.dlg), GTK_RESPONSE_OK);
    gtk_window_set_default_size(GTK_WINDOW(st.dlg), 460, 360);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(st.dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    // Operation Choice
    GtkWidget *f_op = gtk_frame_new("Operation");
    GtkWidget *hb_op = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(hb_op), 8);
    gtk_container_add(GTK_CONTAINER(f_op), hb_op);

    st.radio_not = gtk_radio_button_new_with_label(NULL, "NOT (~)");
    st.radio_and = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(st.radio_not), "AND (&)");
    st.radio_or  = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(st.radio_not), "OR (|)");
    st.radio_xor = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(st.radio_not), "XOR (^)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(st.radio_xor), TRUE);

    g_signal_connect(st.radio_not, "toggled", G_CALLBACK(on_bw_changed), &st);
    g_signal_connect(st.radio_and, "toggled", G_CALLBACK(on_bw_changed), &st);
    g_signal_connect(st.radio_or,  "toggled", G_CALLBACK(on_bw_changed), &st);
    g_signal_connect(st.radio_xor, "toggled", G_CALLBACK(on_bw_changed), &st);

    gtk_box_pack_start(GTK_BOX(hb_op), st.radio_not, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hb_op), st.radio_and, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hb_op), st.radio_or,  TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hb_op), st.radio_xor, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), f_op, FALSE, FALSE, 0);

    // Operand Input
    GtkWidget *hb_operand = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    st.lbl_operand_title = gtk_label_new("Mask / Key (Hex or Dec):");
    gtk_box_pack_start(GTK_BOX(hb_operand), st.lbl_operand_title, FALSE, FALSE, 0);
    st.entry_operand = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(st.entry_operand), "0xFF");
    g_signal_connect(st.entry_operand, "changed", G_CALLBACK(on_bw_changed), &st);
    gtk_box_pack_start(GTK_BOX(hb_operand), st.entry_operand, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hb_operand, FALSE, FALSE, 0);

    // Operand Presets
    GtkWidget *hb_presets = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    #define ADD_BW_PRESET(txt) do { \
        GtkWidget *b = gtk_button_new_with_label(txt); \
        g_signal_connect(b, "clicked", G_CALLBACK(on_bw_preset_clicked), &st); \
        gtk_box_pack_start(GTK_BOX(hb_presets), b, TRUE, TRUE, 0); \
    } while(0)
    ADD_BW_PRESET("0xFF");
    ADD_BW_PRESET("0x55");
    ADD_BW_PRESET("0xAA");
    ADD_BW_PRESET("0x0F");
    ADD_BW_PRESET("0xF0");
    ADD_BW_PRESET("0x20");
    #undef ADD_BW_PRESET
    gtk_box_pack_start(GTK_BOX(vbox), hb_presets, FALSE, FALSE, 0);

    // Scope Frame
    GtkWidget *f_scope = gtk_frame_new("Scope");
    GtkWidget *hb_scope = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(hb_scope), 8);
    gtk_container_add(GTK_CONTAINER(f_scope), hb_scope);

    bool has_sel = (doc->selection.length > 0);
    char sel_txt[64], all_txt[64];
    snprintf(sel_txt, sizeof(sel_txt), "Selected Range (%llu B)", (unsigned long long)doc->selection.length);
    snprintf(all_txt, sizeof(all_txt), "Entire File (%llu B)", (unsigned long long)pt_get_size(doc->pt));

    st.radio_sel = gtk_radio_button_new_with_label(NULL, sel_txt);
    st.radio_all = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(st.radio_sel), all_txt);
    gtk_widget_set_sensitive(st.radio_sel, has_sel);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(has_sel ? st.radio_sel : st.radio_all), TRUE);

    g_signal_connect(st.radio_sel, "toggled", G_CALLBACK(on_bw_changed), &st);
    g_signal_connect(st.radio_all, "toggled", G_CALLBACK(on_bw_changed), &st);

    gtk_box_pack_start(GTK_BOX(hb_scope), st.radio_sel, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hb_scope), st.radio_all, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), f_scope, FALSE, FALSE, 0);

    // Preview
    GtkWidget *f_prev = gtk_frame_new("Live Preview");
    st.lbl_preview = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(st.lbl_preview), 0.0);
    gtk_container_set_border_width(GTK_CONTAINER(st.lbl_preview), 8);
    gtk_container_add(GTK_CONTAINER(f_prev), st.lbl_preview);
    gtk_box_pack_start(GTK_BOX(vbox), f_prev, FALSE, FALSE, 2);

    bitwise_dialog_update_preview(&st);

    gtk_widget_show_all(st.dlg);

    if (gtk_dialog_run(GTK_DIALOG(st.dlg)) == GTK_RESPONSE_OK) {
        bool use_sel = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st.radio_sel));
        hxd_offset_t off = use_sel ? doc->selection.start : 0;
        hxd_size_t total_sz = pt_get_size(doc->pt);
        hxd_size_t len = use_sel ? doc->selection.length : total_sz;
        if (off + len > total_sz) len = (total_sz > off) ? total_sz - off : 0;

        hxd_bitop_t op = HXD_BITOP_NOT;
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st.radio_and))) op = HXD_BITOP_AND;
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st.radio_or))) op = HXD_BITOP_OR;
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(st.radio_xor))) op = HXD_BITOP_XOR;

        uint8_t op_byte = (uint8_t)parse_offset_string(gtk_entry_get_text(GTK_ENTRY(st.entry_operand)), 0);
        doc_bitwise_op(doc, off, len, op, &op_byte, 1);
        app_update_ui(app);
    }

    gtk_widget_destroy(st.dlg);
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
    if (st->app && st->app->last_open_dir && g_file_test(st->app->last_open_dir, G_FILE_TEST_IS_DIR)) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), st->app->last_open_dir);
    }
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        if (path) {
            if (st->app) app_remember_file_dir(st->app, path);
            gtk_entry_set_text(GTK_ENTRY(st->entry_f1), path);
            g_free(path);
        }
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
    if (st->app && st->app->last_open_dir && g_file_test(st->app->last_open_dir, G_FILE_TEST_IS_DIR)) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), st->app->last_open_dir);
    }
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        if (path) {
            if (st->app) app_remember_file_dir(st->app, path);
            gtk_entry_set_text(GTK_ENTRY(st->entry_f2), path);
            g_free(path);
        }
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

static void on_diff_meld_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    diff_dialog_state_t *st = (diff_dialog_state_t *)user_data;
    if (!st || !st->app) return;

    on_diff_run_clicked(NULL, st);
    if (!st->pt1 || !st->pt2) return;

    document_t *doc1 = st->doc1;
    document_t *doc2 = st->doc2;

    if (!doc1) {
        const char *p1 = gtk_entry_get_text(GTK_ENTRY(st->entry_f1));
        if (p1 && strlen(p1) > 0) {
            app_open_document(st->app, p1);
            doc1 = app_get_active_document(st->app);
        }
    }
    if (!doc2) {
        const char *p2 = gtk_entry_get_text(GTK_ENTRY(st->entry_f2));
        if (p2 && strlen(p2) > 0) {
            app_open_document(st->app, p2);
            doc2 = app_get_active_document(st->app);
        }
    }

    if (doc1 && doc2) {
        app_open_diff_view(st->app, doc1, doc2);
        gtk_dialog_response(GTK_DIALOG(st->dialog), GTK_RESPONSE_CLOSE);
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

    // Run Compare button (Green / Suggested Action -> default launch side-by-side in editor)
    GtkWidget *btn_run = gtk_button_new_with_label("Compare Side-by-Side ⬌");
    gtk_widget_set_tooltip_text(btn_run, "Compare files and launch side-by-side Meld-style view in main editor (Default)");
    GtkStyleContext *sc_btn = gtk_widget_get_style_context(btn_run);
    gtk_style_context_add_class(sc_btn, "suggested-action");
    g_signal_connect(btn_run, "clicked", G_CALLBACK(on_diff_meld_clicked), st);
    gtk_grid_attach(GTK_GRID(grid), btn_run, 4, 0, 1, 1);

    GtkWidget *btn_list = gtk_button_new_with_label("Preview List Only");
    gtk_widget_set_tooltip_text(btn_list, "Run comparison and inspect diff list inside this dialog");
    g_signal_connect(btn_list, "clicked", G_CALLBACK(on_diff_run_clicked), st);
    gtk_grid_attach(GTK_GRID(grid), btn_list, 4, 1, 1, 1);

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
    GtkWidget *btn_meld = gtk_button_new_with_label("Open Side-by-Side in Editor ⬌");
    g_signal_connect(btn_meld, "clicked", G_CALLBACK(on_diff_meld_clicked), st);

    gtk_box_pack_start(GTK_BOX(nav_bar), btn_prev, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(nav_bar), btn_next, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(nav_bar), btn_jump, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(nav_bar), btn_meld, FALSE, FALSE, 0);
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
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_case), "Lowercase (a-f)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_case), "Uppercase (A-F)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_case), app->hex_case);

    // Bytes per row
    GtkWidget *spin_bpr = gtk_spin_button_new_with_range(8, 64, 8);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_bpr), app->bytes_per_row);

    // Font size (pt)
    GtkWidget *spin_font = gtk_spin_button_new_with_range(8, 32, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_font), app->font_size_pt > 0 ? app->font_size_pt : DEFAULT_FONT_SIZE_PT);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Theme:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), combo_theme, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), chk_interlace, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), chk_cross, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Hex Case:"), 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), combo_case, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Bytes Per Row:"), 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), spin_bpr, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Font Size (pt):"), 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), spin_font, 1, 5, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dlg);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK) {
        hxd_theme_t new_theme = (hxd_theme_t)gtk_combo_box_get_active(GTK_COMBO_BOX(combo_theme));
        bool new_interlace = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_interlace));
        bool new_cross = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_cross));
        hxd_hex_case_t new_case = (hxd_hex_case_t)gtk_combo_box_get_active(GTK_COMBO_BOX(combo_case));
        int new_bpr = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_bpr));
        int new_font = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_font));

        app_apply_settings(app, new_theme, new_interlace, new_cross, new_case, new_bpr, new_font);
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
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dlg), HEXED_APP_NAME);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dlg), HEXED_VERSION);
    const char *authors[] = { HEXED_AUTHOR, NULL };
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dlg), authors);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dlg), "Copyright © 2026 " HEXED_AUTHOR);
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dlg),
        HEXED_APP_NAME ": High-performance, cross-platform (Linux & Windows) offline GUI hex editor created by " HEXED_AUTHOR ".\n\n"
        "Features:\n"
        "• Virtual Piece Table Engine (instant gigabyte file opening & zero lag)\n"
        "• Native Intel HEX & Motorola S-record parsing with sparse gap preservation\n"
        "• Full 25-type live Data Inspector with bi-directional editing\n"
        "• Real-time Base Converter (Hex, Dec, Oct, Bin)\n"
        "• Checksums & Cryptographic Hashes (CRC-8/16/32/64, MD5, SHA-1, SHA-256)\n"
        "• Pattern Find & Replace across Hex, Strings, Floats, and Integers\n"
        "• Shannon Entropy & Byte Frequency Histogram Statistics\n"
        "• Side-by-Side Meld-Style File Comparison and Differencing Engine\n"
        "• Multi-format Export (Binary, Data URL Base64, C/C++ Snippets, Intel HEX, SREC)"
    );
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dlg), "https://myhex.online");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

int dialog_prompt_ihex_mode(App *app, const char *path) {
    if (!path) return 0;

    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Open Intel-HEX File",
        (app && app->window) ? GTK_WINDOW(app->window) : NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "Raw Intel-HEX", 2,
        "Converted Binary", 1,
        NULL
    );

    gtk_dialog_set_default_response(GTK_DIALOG(dlg), 1);

    GtkWidget *btn_bin = gtk_dialog_get_widget_for_response(GTK_DIALOG(dlg), 1);
    if (btn_bin) {
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_bin), "suggested-action");
    }

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 16);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_box_pack_start(GTK_BOX(content), hbox, TRUE, TRUE, 0);

    GtkWidget *icon = gtk_image_new_from_icon_name("dialog-question", GTK_ICON_SIZE_DIALOG);
    gtk_widget_set_valign(icon, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

    const char *basename = strrchr(path, '/');
#ifdef _WIN32
    const char *bslash = strrchr(path, '\\');
    if (bslash && (!basename || bslash > basename)) basename = bslash;
#endif
    basename = basename ? basename + 1 : path;

    gchar *escaped_name = g_markup_escape_text(basename, -1);
    char title_markup[512];
    snprintf(title_markup, sizeof(title_markup),
             "<span size='large' weight='bold'>Intel-HEX File Detected</span>\n"
             "<span size='small' color='#888888'>%s</span>", escaped_name);
    g_free(escaped_name);

    GtkWidget *lbl_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_title), title_markup);
    gtk_widget_set_halign(lbl_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_title, FALSE, FALSE, 0);

    GtkWidget *lbl_desc = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_desc),
        "This file appears to be in <b>Intel HEX</b> format.\n\n"
        "How would you like to open it?\n\n"
        "• <b>Converted Binary</b>: Decodes record addresses and loads the binary data.\n"
        "• <b>Raw Intel-HEX</b>: Opens the literal text records and checksums as raw bytes."
    );
    gtk_label_set_line_wrap(GTK_LABEL(lbl_desc), TRUE);
    gtk_widget_set_halign(lbl_desc, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_desc, FALSE, FALSE, 4);

    gtk_widget_show_all(dlg);
    gint res = gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);

    if (res == 1) return 1; // Converted Binary
    if (res == 2) return 2; // Raw Intel-HEX
    return 0; // Cancel
}
