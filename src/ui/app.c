#include "app.h"
#include "toolbar.h"
#include "sidebars.h"
#include "bottombar.h"
#include "dialogs.h"
#include "theme.h"
#include "../core/search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void on_tab_switch(GtkNotebook *nb, GtkWidget *page, guint page_num, gpointer user_data) {
    (void)nb;
    (void)page;
    App *app = (App *)user_data;
    GList *elem = g_list_nth(app->hex_views, page_num);
    if (elem) {
        app->active_view = (HexView *)elem->data;
        app_update_ui(app);
    }
}

static void on_hex_view_changed(HexView *view, void *userdata) {
    (void)view;
    App *app = (App *)userdata;
    app_update_ui(app);
}

static void on_tab_close_clicked(GtkButton *btn, gpointer user_data) {
    HexView *view = (HexView *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(btn), "app");
    int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(app->doc_notebook), hex_view_get_widget(view));
    if (page_num >= 0) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(app->doc_notebook), page_num);
        app->hex_views = g_list_remove(app->hex_views, view);
        doc_free(view->doc);
        hex_view_free(view);

        if (g_list_length(app->hex_views) > 0) {
            int cur_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->doc_notebook));
            app->active_view = (HexView *)g_list_nth_data(app->hex_views, cur_page >= 0 ? cur_page : 0);
        } else {
            app->active_view = NULL;
            app_new_document(app);
        }
        app_update_ui(app);
    }
}

static void add_document_tab(App *app, document_t *doc) {
    HexView *view = hex_view_create(doc);
    hex_view_set_column_interlacing(view, app->column_interlacing);
    hex_view_set_cross_highlighting(view, app->cross_highlighting);
    hex_view_set_hex_case(view, app->hex_case);
    hex_view_set_bytes_per_row(view, app->bytes_per_row);
    hex_view_set_change_callback(view, on_hex_view_changed, app);

    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *tab_label = gtk_label_new(doc->display_name);
    GtkWidget *close_btn = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(close_btn), "app", app);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_tab_close_clicked), view);

    gtk_box_pack_start(GTK_BOX(tab_box), tab_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tab_box), close_btn, FALSE, FALSE, 0);
    gtk_widget_show_all(tab_box);

    int page = gtk_notebook_append_page(GTK_NOTEBOOK(app->doc_notebook), hex_view_get_widget(view), tab_box);
    gtk_widget_show_all(hex_view_get_widget(view));

    app->hex_views = g_list_append(app->hex_views, view);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(app->doc_notebook), page);
    app->active_view = view;
    app_update_ui(app);
}

App *app_create(void) {
    App *app = calloc(1, sizeof(App));
    app->theme = HXD_THEME_DARK;
    app->column_interlacing = true;
    app->cross_highlighting = true;
    app->hex_case = HXD_HEX_UPPERCASE;
    app->bytes_per_row = 16;

    theme_init();

    // Main Window
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Hex Editor Neo");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1260, 820);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    app->main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app->window), app->main_vbox);

    // Toolbar
    app->toolbar = toolbar_create(app);
    gtk_box_pack_start(GTK_BOX(app->main_vbox), app->toolbar->toolbar, FALSE, FALSE, 0);

    // Document Notebook
    app->doc_notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(app->doc_notebook), TRUE);
    g_signal_connect(app->doc_notebook, "switch-page", G_CALLBACK(on_tab_switch), app);

    // Sidebars & Bottombar
    app->sidebars = sidebars_create(app);
    app->bottombar = bottombar_create(app);

    // Layout hierarchy using GtkPaned
    app->paned_h1 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL); // Left sidebar | Rest
    app->paned_h2 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL); // Center Docs | Right sidebar
    app->paned_v  = gtk_paned_new(GTK_ORIENTATION_VERTICAL);   // Top area | Bottombar

    gtk_paned_pack1(GTK_PANED(app->paned_h2), app->doc_notebook, TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(app->paned_h2), app->sidebars->right_notebook, FALSE, TRUE);
    gtk_paned_set_position(GTK_PANED(app->paned_h2), 700);

    gtk_paned_pack1(GTK_PANED(app->paned_h1), app->sidebars->left_notebook, FALSE, TRUE);
    gtk_paned_pack2(GTK_PANED(app->paned_h1), app->paned_h2, TRUE, FALSE);
    gtk_paned_set_position(GTK_PANED(app->paned_h1), 260);

    gtk_paned_pack1(GTK_PANED(app->paned_v), app->paned_h1, TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(app->paned_v), app->bottombar->notebook, FALSE, TRUE);
    gtk_paned_set_position(GTK_PANED(app->paned_v), 540);

    gtk_box_pack_start(GTK_BOX(app->main_vbox), app->paned_v, TRUE, TRUE, 0);

    // Statusbar
    app->statusbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->statusbar), "statusbar");
    gtk_container_set_border_width(GTK_CONTAINER(app->statusbar), 4);

    app->lbl_status_edit = gtk_label_new("No changes");
    app->lbl_status_offset = gtk_label_new("Offset: 0x00000000 (0)");
    app->lbl_status_sel = gtk_label_new("Sel: 0x0 (0 bytes)");
    app->lbl_status_size = gtk_label_new("Size: 0 bytes");
    app->lbl_status_mode = gtk_label_new("OVR");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_status_mode), "mode-badge-ovr");

    gtk_box_pack_start(GTK_BOX(app->statusbar), app->lbl_status_edit, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(app->statusbar), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(app->statusbar), app->lbl_status_offset, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(app->statusbar), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(app->statusbar), app->lbl_status_sel, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(app->statusbar), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(app->statusbar), app->lbl_status_size, FALSE, FALSE, 4);

    GtkWidget *st_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(app->statusbar), st_spacer, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(app->statusbar), app->lbl_status_mode, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(app->main_vbox), app->statusbar, FALSE, FALSE, 0);

    // Create initial blank document
    app_new_document(app);

    return app;
}

document_t *app_get_active_document(App *app) {
    return (app && app->active_view) ? app->active_view->doc : NULL;
}

HexView *app_get_active_view(App *app) {
    return app ? app->active_view : NULL;
}

void app_update_ui(App *app) {
    if (!app) return;
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    // Update window title
    char title[256];
    snprintf(title, sizeof(title), "Hex Editor Neo - [%s%s]", doc->display_name, doc->is_modified ? "*" : "");
    gtk_window_set_title(GTK_WINDOW(app->window), title);

    // Update status bar
    gtk_label_set_text(GTK_LABEL(app->lbl_status_edit), doc->is_modified ? "Modified" : "No changes");

    char off_str[64];
    snprintf(off_str, sizeof(off_str), "Offset: 0x%08llX (%llu)", (unsigned long long)doc->cursor_offset, (unsigned long long)doc->cursor_offset);
    gtk_label_set_text(GTK_LABEL(app->lbl_status_offset), off_str);

    char sel_str[64];
    snprintf(sel_str, sizeof(sel_str), "Sel: 0x%llX (%llu bytes)", (unsigned long long)doc->selection.length, (unsigned long long)doc->selection.length);
    gtk_label_set_text(GTK_LABEL(app->lbl_status_sel), sel_str);

    hxd_size_t sz = pt_get_size(doc->pt);
    char sz_str[64];
    if (sz >= 1048576) {
        snprintf(sz_str, sizeof(sz_str), "Size: %.2f MB (%llu bytes)", sz / (1024.0 * 1024.0), (unsigned long long)sz);
    } else if (sz >= 1024) {
        snprintf(sz_str, sizeof(sz_str), "Size: %.2f KB (%llu bytes)", sz / 1024.0, (unsigned long long)sz);
    } else {
        snprintf(sz_str, sizeof(sz_str), "Size: %llu bytes", (unsigned long long)sz);
    }
    gtk_label_set_text(GTK_LABEL(app->lbl_status_size), sz_str);

    if (doc->edit_mode == HXD_MODE_INSERT) {
        gtk_label_set_text(GTK_LABEL(app->lbl_status_mode), "INS");
        gtk_style_context_remove_class(gtk_widget_get_style_context(app->lbl_status_mode), "mode-badge-ovr");
        gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_status_mode), "mode-badge-ins");
    } else {
        gtk_label_set_text(GTK_LABEL(app->lbl_status_mode), "OVR");
        gtk_style_context_remove_class(gtk_widget_get_style_context(app->lbl_status_mode), "mode-badge-ins");
        gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_status_mode), "mode-badge-ovr");
    }

    toolbar_update_state(app->toolbar, app);
    sidebars_update(app->sidebars, app);
    bottombar_update(app->bottombar, app);

    if (app->active_view) {
        hex_view_refresh(app->active_view);
    }
}

void app_new_document(App *app) {
    document_t *doc = doc_create_new();
    add_document_tab(app, doc);
}

void app_open_document(App *app, const char *path) {
    if (!path) return;
    document_t *doc = doc_open_file(path);
    if (doc) {
        add_document_tab(app, doc);
    }
}

void app_open_dialog(App *app) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Open Binary File",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );

    GtkFileFilter *filter_all = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_all, "All Supported Files (*.*, .hex, .srec)");
    gtk_file_filter_add_pattern(filter_all, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_all);

    GtkFileFilter *filter_hex = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_hex, "Intel HEX / Motorola S-Record (*.hex, *.srec, *.s19)");
    gtk_file_filter_add_pattern(filter_hex, "*.hex");
    gtk_file_filter_add_pattern(filter_hex, "*.ihex");
    gtk_file_filter_add_pattern(filter_hex, "*.srec");
    gtk_file_filter_add_pattern(filter_hex, "*.s19");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_hex);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        app_open_document(app, filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

void app_save_document(App *app) {
    document_t *doc = app_get_active_document(app);
    if (!doc) return;
    if (doc->filepath) {
        doc_save(doc);
        app_update_ui(app);
    } else {
        app_save_as_document(app);
    }
}

void app_save_as_document(App *app) {
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Save File As",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        NULL
    );
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    if (doc->filepath) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), doc->filepath);
    } else {
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "output.bin");
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        doc_save_as(doc, filename);
        g_free(filename);
        app_update_ui(app);
    }
    gtk_widget_destroy(dialog);
}

void app_close_active_document(App *app) {
    if (!app || !app->active_view) return;
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->doc_notebook));
    if (page >= 0) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(app->doc_notebook), page);
        HexView *view = app->active_view;
        app->hex_views = g_list_remove(app->hex_views, view);
        doc_free(view->doc);
        hex_view_free(view);

        if (g_list_length(app->hex_views) > 0) {
            int cur_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->doc_notebook));
            app->active_view = (HexView *)g_list_nth_data(app->hex_views, cur_page >= 0 ? cur_page : 0);
        } else {
            app->active_view = NULL;
            app_new_document(app);
        }
        app_update_ui(app);
    }
}

void app_undo(App *app) {
    document_t *doc = app_get_active_document(app);
    if (doc && doc_undo(doc)) {
        app_update_ui(app);
    }
}

void app_redo(App *app) {
    document_t *doc = app_get_active_document(app);
    if (doc && doc_redo(doc)) {
        app_update_ui(app);
    }
}

void app_cut(App *app) {
    app_copy(app);
    app_delete(app);
}

void app_copy(App *app) {
    document_t *doc = app_get_active_document(app);
    if (!doc || doc->selection.length == 0) return;

    hxd_size_t len = doc->selection.length;
    uint8_t *buf = malloc(len);
    pt_read_bytes(doc->pt, doc->selection.start, buf, len, NULL);

    // Format hex string e.g. "AA BB CC DD"
    char *hex_str = malloc(len * 3 + 1);
    char *p = hex_str;
    for (hxd_size_t i = 0; i < len; i++) {
        p += sprintf(p, "%02X ", buf[i]);
    }
    if (len > 0) *(p - 1) = '\0';
    else *p = '\0';

    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), hex_str, -1);
    free(hex_str);
    free(buf);
}

void app_paste(App *app) {
    document_t *doc = app_get_active_document(app);
    if (!doc) return;

    gchar *text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
    if (!text) return;

    uint8_t *pat = NULL;
    size_t pat_len = 0;
    // Try parsing as hex first, fallback to raw ascii text
    if (!search_parse_pattern(text, SEARCH_TYPE_HEX, doc->endian, &pat, &pat_len)) {
        pat = (uint8_t *)strdup(text);
        pat_len = strlen(text);
    }

    if (pat && pat_len > 0) {
        doc_paste(doc, pat, pat_len);
        free(pat);
        app_update_ui(app);
    }
    g_free(text);
}

void app_delete(App *app) {
    document_t *doc = app_get_active_document(app);
    if (doc && doc_delete(doc)) {
        app_update_ui(app);
    }
}

void app_toggle_mode(App *app) {
    document_t *doc = app_get_active_document(app);
    if (doc) {
        doc->edit_mode = (doc->edit_mode == HXD_MODE_OVERWRITE) ? HXD_MODE_INSERT : HXD_MODE_OVERWRITE;
        app_update_ui(app);
    }
}

void app_show_find(App *app) {
    gtk_notebook_set_current_page(GTK_NOTEBOOK(app->sidebars->right_notebook), 0);
    gtk_widget_grab_focus(app->sidebars->find_entry);
}

void app_show_replace(App *app) {
    gtk_notebook_set_current_page(GTK_NOTEBOOK(app->sidebars->right_notebook), 0);
    gtk_widget_grab_focus(app->sidebars->replace_entry);
}

void app_show_fill_dialog(App *app) { dialog_show_fill(app); }
void app_show_insert_dialog(App *app) { dialog_show_insert(app); }
void app_show_gap_dialog(App *app) { dialog_show_gap(app); }
void app_show_size_dialog(App *app) { dialog_show_size(app); }
void app_show_insert_file_dialog(App *app) { dialog_show_insert_file(app); }
void app_show_settings_dialog(App *app) { dialog_show_settings(app); }
void app_show_controls_dialog(App *app) { dialog_show_controls(app); }
void app_show_about_dialog(App *app) { dialog_show_about(app); }

void app_run(App *app, int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            app_open_document(app, argv[i]);
        }
    }

    gtk_widget_show_all(app->window);
    gtk_main();
}
