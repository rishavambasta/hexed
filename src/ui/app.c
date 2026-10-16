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

static gboolean on_window_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static gboolean on_app_window_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

static void on_tab_switch(GtkNotebook *nb, GtkWidget *page, guint page_num, gpointer user_data) {
    (void)nb;
    (void)page;
    App *app = (App *)user_data;
    GList *elem = g_list_nth(app->hex_views, page_num);
    if (elem) {
        app->active_view = (HexView *)elem->data;
        app_update_ui(app);
        if (app->active_view && app->active_view->drawing_area) {
            gtk_widget_grab_focus(app->active_view->drawing_area);
        }
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
    if (!app || !view) return;
    app_close_document_view(app, view, false);
}

static void on_tab_menu_close(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    HexView *view = (HexView *)user_data;
    if (!view) return;
    App *app = (App *)g_object_get_data(G_OBJECT(view->drawing_area), "app");
    if (!app && view->change_userdata) app = (App *)view->change_userdata;
    if (!app) return;
    app_close_document_view(app, view, false);
}

static void on_tab_menu_close_others(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    HexView *keep_view = (HexView *)user_data;
    if (!keep_view) return;
    App *app = (App *)g_object_get_data(G_OBJECT(keep_view->drawing_area), "app");
    if (!app && keep_view->change_userdata) app = (App *)keep_view->change_userdata;
    if (!app) return;
    app_close_all_except(app, keep_view);
}

static void on_tab_menu_close_all(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    App *app = (App *)user_data;
    if (!app) return;
    app_close_all(app);
}

static gboolean on_tab_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    HexView *view = (HexView *)user_data;
    App *app = (App *)g_object_get_data(G_OBJECT(widget), "app");
    if (!app && view) app = (App *)g_object_get_data(G_OBJECT(view->drawing_area), "app");
    if (!app && view && view->change_userdata) app = (App *)view->change_userdata;
    if (!app || !view) return FALSE;

    if (event->button == 3) { // Right click -> Context Menu
        int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(app->doc_notebook), hex_view_get_widget(view));
        if (page_num >= 0) {
            gtk_notebook_set_current_page(GTK_NOTEBOOK(app->doc_notebook), page_num);
            app->active_view = view;
            app_update_ui(app);
        }

        GtkWidget *menu = gtk_menu_new();

        GtkWidget *m_close = gtk_menu_item_new_with_label("Close Tab");
        GtkWidget *m_close_others = gtk_menu_item_new_with_label("Close all but this");
        GtkWidget *m_close_all = gtk_menu_item_new_with_label("Close all");

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_close);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_close_others);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_close_all);

        g_signal_connect(m_close, "activate", G_CALLBACK(on_tab_menu_close), view);
        g_signal_connect(m_close_others, "activate", G_CALLBACK(on_tab_menu_close_others), view);
        g_signal_connect(m_close_all, "activate", G_CALLBACK(on_tab_menu_close_all), app);

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
        return TRUE;
    } else if (event->button == 2) { // Middle click -> Close Tab
        app_close_document_view(app, view, false);
        return TRUE;
    }

    return FALSE;
}

static GtkNotebook *on_notebook_create_window(GtkNotebook *notebook, GtkWidget *page, gint x, gint y, gpointer user_data) {
    (void)notebook;
    (void)page;
    App *app = (App *)user_data;

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "hexed");
    gtk_window_set_default_size(GTK_WINDOW(win), 900, 620);
    g_signal_connect(win, "key-press-event", G_CALLBACK(on_app_window_key_press), app);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    GtkWidget *new_nb = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(new_nb), TRUE);
    gtk_notebook_set_group_name(GTK_NOTEBOOK(new_nb), "hxd-notebook-group");
    g_signal_connect(new_nb, "create-window", G_CALLBACK(on_notebook_create_window), app);

    gtk_box_pack_start(GTK_BOX(vbox), new_nb, TRUE, TRUE, 0);

    if (x > 0 && y > 0) {
        gtk_window_move(GTK_WINDOW(win), x, y);
    }
    gtk_widget_show_all(win);

    return GTK_NOTEBOOK(new_nb);
}

static void add_document_tab(App *app, document_t *doc) {
    HexView *view = hex_view_create(doc);
    hex_view_set_column_interlacing(view, app->column_interlacing);
    hex_view_set_cross_highlighting(view, app->cross_highlighting);
    hex_view_set_hex_case(view, app->hex_case);
    hex_view_set_bytes_per_row(view, app->bytes_per_row);
    hex_view_set_change_callback(view, on_hex_view_changed, app);
    g_object_set_data(G_OBJECT(view->drawing_area), "app", app);

    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *tab_label = gtk_label_new(NULL);
    hex_view_set_tab_label(view, tab_label);
    g_object_set_data(G_OBJECT(hex_view_get_widget(view)), "tab_label", tab_label);
    GtkWidget *close_btn = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(close_btn), "app", app);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_tab_close_clicked), view);

    gtk_box_pack_start(GTK_BOX(tab_box), tab_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tab_box), close_btn, FALSE, FALSE, 0);

    GtkWidget *event_box = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(event_box), FALSE);
    gtk_widget_add_events(event_box, GDK_BUTTON_PRESS_MASK);
    gtk_container_add(GTK_CONTAINER(event_box), tab_box);
    g_object_set_data(G_OBJECT(event_box), "app", app);
    g_object_set_data(G_OBJECT(event_box), "view", view);
    g_signal_connect(event_box, "button-press-event", G_CALLBACK(on_tab_button_press), view);
    gtk_widget_show_all(event_box);

    int page = gtk_notebook_append_page(GTK_NOTEBOOK(app->doc_notebook), hex_view_get_widget(view), event_box);
    gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(app->doc_notebook), hex_view_get_widget(view), TRUE);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(app->doc_notebook), hex_view_get_widget(view), TRUE);
    gtk_widget_show_all(hex_view_get_widget(view));

    app->hex_views = g_list_append(app->hex_views, view);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(app->doc_notebook), page);
    app->active_view = view;
    app_update_ui(app);
    if (view->drawing_area) {
        gtk_widget_grab_focus(view->drawing_area);
    }
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
    gtk_window_set_title(GTK_WINDOW(app->window), "hexed");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1260, 820);
    g_signal_connect(app->window, "delete-event", G_CALLBACK(on_window_delete_event), app);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(app->window, "key-press-event", G_CALLBACK(on_app_window_key_press), app);

    app->main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app->window), app->main_vbox);

    // Toolbar
    app->toolbar = toolbar_create(app);
    gtk_box_pack_start(GTK_BOX(app->main_vbox), app->toolbar->toolbar, FALSE, FALSE, 0);

    // Document Notebook
    app->doc_notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(app->doc_notebook), TRUE);
    gtk_notebook_set_group_name(GTK_NOTEBOOK(app->doc_notebook), "hxd-notebook-group");
    g_signal_connect(app->doc_notebook, "create-window", G_CALLBACK(on_notebook_create_window), app);
    g_signal_connect(app->doc_notebook, "switch-page", G_CALLBACK(on_tab_switch), app);

    // Notebook Top Actions (New and Open icons directly on top of tab bar)
    GtkWidget *tab_actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    GtkWidget *btn_nb_new = gtk_button_new_from_icon_name("tab-new-symbolic", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(btn_nb_new), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(btn_nb_new, "New Document (Ctrl+N)");
    g_signal_connect_swapped(btn_nb_new, "clicked", G_CALLBACK(app_new_document), app);
    gtk_box_pack_start(GTK_BOX(tab_actions), btn_nb_new, FALSE, FALSE, 0);

    GtkWidget *btn_nb_open = gtk_button_new_from_icon_name("document-open-symbolic", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(btn_nb_open), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(btn_nb_open, "Open File (Ctrl+O)");
    g_signal_connect_swapped(btn_nb_open, "clicked", G_CALLBACK(app_open_dialog), app);
    gtk_box_pack_start(GTK_BOX(tab_actions), btn_nb_open, FALSE, FALSE, 0);

    gtk_widget_show_all(tab_actions);
    gtk_notebook_set_action_widget(GTK_NOTEBOOK(app->doc_notebook), tab_actions, GTK_PACK_END);

    // Sidebars & Bottombar
    app->sidebars = sidebars_create(app);
    app->bottombar = bottombar_create(app);

    // Layout hierarchy using GtkPaned
    app->paned_h1 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL); // Left sidebar | Rest
    app->paned_h2 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL); // Center Docs | Right sidebar
    app->paned_v  = gtk_paned_new(GTK_ORIENTATION_VERTICAL);   // Top area | Bottombar

    gtk_paned_pack1(GTK_PANED(app->paned_h2), app->doc_notebook, TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(app->paned_h2), app->sidebars->right_notebook, FALSE, TRUE);
    // Right sidebar minimized by default
    gtk_paned_set_position(GTK_PANED(app->paned_h2), 99999);

    gtk_paned_pack1(GTK_PANED(app->paned_h1), app->sidebars->left_notebook, FALSE, TRUE);
    gtk_paned_pack2(GTK_PANED(app->paned_h1), app->paned_h2, TRUE, FALSE);
    // Left sidebar minimized by default
    gtk_paned_set_position(GTK_PANED(app->paned_h1), 0);

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
    snprintf(title, sizeof(title), "hexed - [%s%s]", doc->display_name, doc->is_modified ? "*" : "");
    gtk_window_set_title(GTK_WINDOW(app->window), title);

    // Update all document tab labels: bold followed by "*" when modified
    for (GList *l = app->hex_views; l != NULL; l = l->next) {
        HexView *v = (HexView *)l->data;
        if (!v || !v->doc) continue;
        GtkWidget *lbl = hex_view_get_tab_label(v);
        if (!lbl) lbl = (GtkWidget *)g_object_get_data(G_OBJECT(hex_view_get_widget(v)), "tab_label");
        if (lbl && GTK_IS_LABEL(lbl)) {
            const char *name = v->doc->display_name ? v->doc->display_name : "Untitled";
            if (v->doc->is_modified) {
                gchar *esc = g_markup_escape_text(name, -1);
                char markup[512];
                snprintf(markup, sizeof(markup), "<b>%s*</b>", esc);
                gtk_label_set_markup(GTK_LABEL(lbl), markup);
                g_free(esc);
            } else {
                gtk_label_set_text(GTK_LABEL(lbl), name);
            }
        }
    }

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

bool app_close_document_view(App *app, HexView *view, bool is_quitting) {
    if (!app || !view || !view->doc) return true;

    // Warn if document is modified
    if (view->doc->is_modified) {
        // Bring this document tab into focus so user sees what is being saved/closed
        int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(app->doc_notebook), hex_view_get_widget(view));
        if (page_num >= 0) {
            gtk_notebook_set_current_page(GTK_NOTEBOOK(app->doc_notebook), page_num);
            app->active_view = view;
            app_update_ui(app);
        }

        const char *name = view->doc->display_name ? view->doc->display_name : "Untitled";
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_NONE,
            "Save changes to \"%s\" before closing?",
            name
        );
        gtk_message_dialog_format_secondary_text(
            GTK_MESSAGE_DIALOG(dialog),
            "If you close without saving, your changes will be permanently discarded."
        );
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Don't Save", GTK_RESPONSE_NO);
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Cancel", GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Save", GTK_RESPONSE_YES);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);

        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        if (response == GTK_RESPONSE_YES) {
            if (view->doc->filepath) {
                if (!doc_save(view->doc)) {
                    GtkWidget *err_dialog = gtk_message_dialog_new(
                        GTK_WINDOW(app->window),
                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                        GTK_MESSAGE_ERROR,
                        GTK_BUTTONS_OK,
                        "Failed to save \"%s\".",
                        name
                    );
                    gtk_dialog_run(GTK_DIALOG(err_dialog));
                    gtk_widget_destroy(err_dialog);
                    return false;
                }
            } else {
                // Untitled document -> prompt Save As dialog
                GtkWidget *save_dialog = gtk_file_chooser_dialog_new(
                    "Save File As",
                    GTK_WINDOW(app->window),
                    GTK_FILE_CHOOSER_ACTION_SAVE,
                    "_Cancel", GTK_RESPONSE_CANCEL,
                    "_Save", GTK_RESPONSE_ACCEPT,
                    NULL
                );
                gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(save_dialog), TRUE);
                gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(save_dialog), name);

                gint sa_response = gtk_dialog_run(GTK_DIALOG(save_dialog));
                if (sa_response == GTK_RESPONSE_ACCEPT) {
                    char *chosen = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(save_dialog));
                    bool saved = doc_save_as(view->doc, chosen);
                    g_free(chosen);
                    gtk_widget_destroy(save_dialog);
                    if (!saved) return false;
                } else {
                    gtk_widget_destroy(save_dialog);
                    return false;
                }
            }
        } else if (response == GTK_RESPONSE_NO) {
            // User chose "Don't Save" -> discard changes and proceed with closing
        } else {
            // Cancel or window closed -> abort closing
            return false;
        }
    }

    int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(app->doc_notebook), hex_view_get_widget(view));
    if (page_num >= 0) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(app->doc_notebook), page_num);
    }
    app->hex_views = g_list_remove(app->hex_views, view);
    doc_free(view->doc);
    hex_view_free(view);

    if (g_list_length(app->hex_views) > 0) {
        int cur_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->doc_notebook));
        app->active_view = (HexView *)g_list_nth_data(app->hex_views, cur_page >= 0 ? cur_page : 0);
        if (app->active_view && app->active_view->drawing_area) {
            gtk_widget_grab_focus(app->active_view->drawing_area);
        }
    } else {
        app->active_view = NULL;
        if (!is_quitting) {
            app_new_document(app);
        }
    }
    app_update_ui(app);
    return true;
}

void app_close_active_document(App *app) {
    if (!app || !app->active_view) return;
    app_close_document_view(app, app->active_view, false);
}

bool app_close_all_documents(App *app) {
    if (!app) return true;
    while (app->hex_views != NULL) {
        HexView *v = (HexView *)app->hex_views->data;
        if (!app_close_document_view(app, v, true)) {
            return false;
        }
    }
    return true;
}

static gboolean on_window_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    (void)widget;
    (void)event;
    App *app = (App *)user_data;
    if (!app_close_all_documents(app)) {
        return TRUE; // Cancel window close
    }
    return FALSE; // Allow close and trigger destroy
}

void app_quit(App *app) {
    if (!app) return;
    if (app_close_all_documents(app)) {
        gtk_widget_destroy(app->window);
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
void app_show_select_range_dialog(App *app) { dialog_show_select_range(app); }
void app_show_goto_dialog(App *app) { dialog_show_goto(app); }
void app_show_bitwise_op_dialog(App *app) { dialog_show_bitwise_op(app); }
void app_show_diff_dialog(App *app) { dialog_show_diff(app); }

typedef struct {
    hxd_offset_t offset;
    hxd_size_t   length;
} app_diff_chunk_t;

typedef struct {
    App              *app;
    document_t       *doc1;
    document_t       *doc2;
    HexView          *v1;
    HexView          *v2;
    GtkWidget        *paned;
    GtkWidget        *tab_content;
    GtkWidget        *lbl_info;
    GtkWidget        *btn_orient;
    app_diff_chunk_t *chunks;
    size_t            chunk_count;
    size_t            current_chunk;
    bool              syncing_scroll;
    bool              is_horizontal;
} app_diff_view_state_t;

static void on_editor_diff_v1_scrolled(GtkAdjustment *adj, gpointer user_data) {
    app_diff_view_state_t *st = (app_diff_view_state_t *)user_data;
    if (st->syncing_scroll) return;
    st->syncing_scroll = true;
    gtk_adjustment_set_value(st->v2->v_adj, gtk_adjustment_get_value(adj));
    st->syncing_scroll = false;
}

static void on_editor_diff_v2_scrolled(GtkAdjustment *adj, gpointer user_data) {
    app_diff_view_state_t *st = (app_diff_view_state_t *)user_data;
    if (st->syncing_scroll) return;
    st->syncing_scroll = true;
    gtk_adjustment_set_value(st->v1->v_adj, gtk_adjustment_get_value(adj));
    st->syncing_scroll = false;
}

static void editor_diff_update_ui(app_diff_view_state_t *st) {
    if (st->chunk_count == 0) {
        gtk_label_set_markup(GTK_LABEL(st->lbl_info), "<b>Files are 100% Identical!</b>");
        return;
    }

    if (st->current_chunk >= st->chunk_count) st->current_chunk = 0;
    app_diff_chunk_t c = st->chunks[st->current_chunk];

    doc_set_cursor(st->doc1, c.offset, false);
    doc_set_selection(st->doc1, c.offset, c.length);
    doc_set_cursor(st->doc2, c.offset, false);
    doc_set_selection(st->doc2, c.offset, c.length);

    hex_view_scroll_to_offset(st->v1, c.offset);
    hex_view_scroll_to_offset(st->v2, c.offset);
    hex_view_refresh(st->v1);
    hex_view_refresh(st->v2);

    char buf[256];
    snprintf(buf, sizeof(buf),
             "Diff <b>%zu of %zu</b> | Offset: <b>0x%08llX</b> | Length: <b>%llu byte%s</b>",
             st->current_chunk + 1, st->chunk_count,
             (unsigned long long)c.offset, (unsigned long long)c.length,
             c.length == 1 ? "" : "s");
    gtk_label_set_markup(GTK_LABEL(st->lbl_info), buf);
}

static void on_editor_diff_prev(GtkButton *btn, gpointer user_data) {
    (void)btn;
    app_diff_view_state_t *st = (app_diff_view_state_t *)user_data;
    if (st->chunk_count == 0) return;
    if (st->current_chunk > 0) st->current_chunk--;
    else st->current_chunk = st->chunk_count - 1;
    editor_diff_update_ui(st);
}

static void on_editor_diff_next(GtkButton *btn, gpointer user_data) {
    (void)btn;
    app_diff_view_state_t *st = (app_diff_view_state_t *)user_data;
    if (st->chunk_count == 0) return;
    if (st->current_chunk + 1 < st->chunk_count) st->current_chunk++;
    else st->current_chunk = 0;
    editor_diff_update_ui(st);
}

static void on_editor_diff_orient(GtkButton *btn, gpointer user_data) {
    app_diff_view_state_t *st = (app_diff_view_state_t *)user_data;
    st->is_horizontal = !st->is_horizontal;
    gtk_orientable_set_orientation(GTK_ORIENTABLE(st->paned),
        st->is_horizontal ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
    gtk_button_set_label(btn, st->is_horizontal ? "Split: Horizontal ⬌" : "Split: Vertical ⬍");
}

static void on_editor_diff_close(GtkButton *btn, gpointer user_data) {
    (void)btn;
    app_diff_view_state_t *st = (app_diff_view_state_t *)user_data;
    int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(st->app->doc_notebook), st->tab_content);
    if (page_num >= 0) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(st->app->doc_notebook), page_num);
    }
    hex_view_free(st->v1);
    hex_view_free(st->v2);
    free(st->chunks);
    free(st);
}

static gboolean on_diff_tab_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    app_diff_view_state_t *st = (app_diff_view_state_t *)user_data;
    if (!st || !st->app) return FALSE;

    if (event->button == 3) {
        int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(st->app->doc_notebook), st->tab_content);
        if (page_num >= 0) {
            gtk_notebook_set_current_page(GTK_NOTEBOOK(st->app->doc_notebook), page_num);
        }

        GtkWidget *menu = gtk_menu_new();
        GtkWidget *m_close = gtk_menu_item_new_with_label("Close Diff");
        GtkWidget *m_close_all = gtk_menu_item_new_with_label("Close all");

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_close);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_close_all);

        g_signal_connect_swapped(m_close, "activate", G_CALLBACK(on_editor_diff_close), st);
        g_signal_connect_swapped(m_close_all, "activate", G_CALLBACK(app_close_all), st->app);

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
        return TRUE;
    } else if (event->button == 2) {
        on_editor_diff_close(NULL, st);
        return TRUE;
    }

    return FALSE;
}

void app_close_all(App *app) {
    if (!app) return;

    // 1. Close any diff tabs
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(app->doc_notebook));
    for (int i = n_pages - 1; i >= 0; i--) {
        GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->doc_notebook), i);
        if (page) {
            void *st = g_object_get_data(G_OBJECT(page), "diff_state");
            if (st) {
                on_editor_diff_close(NULL, st);
            }
        }
    }

    // 2. Close all document tabs
    while (app->hex_views != NULL) {
        HexView *v = (HexView *)app->hex_views->data;
        if (!app_close_document_view(app, v, true)) {
            break;
        }
    }

    // If all views were closed, start a new blank document
    if (app->hex_views == NULL) {
        app_new_document(app);
    }
}

void app_close_all_except(App *app, HexView *keep_view) {
    if (!app || !keep_view) return;

    // 1. Close any diff tabs
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(app->doc_notebook));
    for (int i = n_pages - 1; i >= 0; i--) {
        GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->doc_notebook), i);
        if (page) {
            void *st = g_object_get_data(G_OBJECT(page), "diff_state");
            if (st) {
                on_editor_diff_close(NULL, st);
            }
        }
    }

    // 2. Close other document tabs
    GList *cur = app->hex_views;
    while (cur != NULL) {
        HexView *v = (HexView *)cur->data;
        cur = cur->next;
        if (v != keep_view) {
            if (!app_close_document_view(app, v, false)) {
                break;
            }
        }
    }

    // Keep target view active
    int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(app->doc_notebook), hex_view_get_widget(keep_view));
    if (page_num >= 0) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(app->doc_notebook), page_num);
        app->active_view = keep_view;
        app_update_ui(app);
        if (keep_view->drawing_area) {
            gtk_widget_grab_focus(keep_view->drawing_area);
        }
    }
}

void app_open_diff_view(App *app, document_t *doc1, document_t *doc2) {
    if (!app || !doc1 || !doc2) return;

    app_diff_view_state_t *st = calloc(1, sizeof(app_diff_view_state_t));
    st->app = app;
    st->doc1 = doc1;
    st->doc2 = doc2;
    st->is_horizontal = true;

    // Scan difference chunks
    hxd_size_t sz1 = pt_get_size(doc1->pt);
    hxd_size_t sz2 = pt_get_size(doc2->pt);
    hxd_size_t max_sz = (sz1 > sz2) ? sz1 : sz2;

    size_t cap = 32;
    st->chunks = malloc(cap * sizeof(app_diff_chunk_t));
    st->chunk_count = 0;

    bool in_diff = false;
    hxd_offset_t d_start = 0;
    hxd_size_t d_len = 0;

    const size_t CHUNK = 4096;
    uint8_t buf1[CHUNK], buf2[CHUNK];
    hxd_offset_t cur = 0;

    while (cur < max_sz) {
        size_t n = (max_sz - cur < CHUNK) ? (size_t)(max_sz - cur) : CHUNK;
        hxd_size_t r1 = pt_read_bytes(doc1->pt, cur, buf1, n, NULL);
        hxd_size_t r2 = pt_read_bytes(doc2->pt, cur, buf2, n, NULL);

        for (size_t i = 0; i < n; i++) {
            bool has1 = (cur + i < sz1 && i < (size_t)r1);
            bool has2 = (cur + i < sz2 && i < (size_t)r2);
            bool diff = (!has1 || !has2 || buf1[i] != buf2[i]);

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
                    if (st->chunk_count >= cap) {
                        cap *= 2;
                        st->chunks = realloc(st->chunks, cap * sizeof(app_diff_chunk_t));
                    }
                    st->chunks[st->chunk_count].offset = d_start;
                    st->chunks[st->chunk_count].length = d_len;
                    st->chunk_count++;
                    in_diff = false;
                }
            }
        }
        cur += n;
    }
    if (in_diff) {
        if (st->chunk_count >= cap) {
            cap *= 2;
            st->chunks = realloc(st->chunks, cap * sizeof(app_diff_chunk_t));
        }
        st->chunks[st->chunk_count].offset = d_start;
        st->chunks[st->chunk_count].length = d_len;
        st->chunk_count++;
    }

    // Build Tab Content
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    st->tab_content = vbox;
    g_object_set_data(G_OBJECT(vbox), "diff_state", st);

    // Diff Bar
    GtkWidget *hb_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(hb_bar), 4);

    GtkWidget *lbl_title = gtk_label_new(NULL);
    char title_buf[256];
    snprintf(title_buf, sizeof(title_buf), "<b>Meld-Style Diff:</b> %s ⟷ %s",
             doc1->display_name ? doc1->display_name : "Doc 1",
             doc2->display_name ? doc2->display_name : "Doc 2");
    gtk_label_set_markup(GTK_LABEL(lbl_title), title_buf);
    gtk_box_pack_start(GTK_BOX(hb_bar), lbl_title, FALSE, FALSE, 4);

    GtkWidget *btn_prev = gtk_button_new_with_label("◀ Prev Diff (Shift+F7)");
    g_signal_connect(btn_prev, "clicked", G_CALLBACK(on_editor_diff_prev), st);
    gtk_box_pack_start(GTK_BOX(hb_bar), btn_prev, FALSE, FALSE, 0);

    GtkWidget *btn_next = gtk_button_new_with_label("Next Diff ▶ (F7)");
    g_signal_connect(btn_next, "clicked", G_CALLBACK(on_editor_diff_next), st);
    gtk_box_pack_start(GTK_BOX(hb_bar), btn_next, FALSE, FALSE, 0);

    st->lbl_info = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(hb_bar), st->lbl_info, FALSE, FALSE, 8);

    st->btn_orient = gtk_button_new_with_label("Split: Horizontal ⬌");
    g_signal_connect(st->btn_orient, "clicked", G_CALLBACK(on_editor_diff_orient), st);
    gtk_box_pack_start(GTK_BOX(hb_bar), st->btn_orient, FALSE, FALSE, 4);

    GtkWidget *btn_close = gtk_button_new_with_label("Close Diff (×)");
    g_signal_connect(btn_close, "clicked", G_CALLBACK(on_editor_diff_close), st);
    gtk_box_pack_end(GTK_BOX(hb_bar), btn_close, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hb_bar, FALSE, FALSE, 0);

    // Paned with side-by-side HexViews
    st->paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    st->v1 = hex_view_create(doc1);
    st->v2 = hex_view_create(doc2);

    hex_view_set_column_interlacing(st->v1, app->column_interlacing);
    hex_view_set_column_interlacing(st->v2, app->column_interlacing);
    hex_view_set_hex_case(st->v1, app->hex_case);
    hex_view_set_hex_case(st->v2, app->hex_case);
    hex_view_set_bytes_per_row(st->v1, app->bytes_per_row);
    hex_view_set_bytes_per_row(st->v2, app->bytes_per_row);

    g_signal_connect(st->v1->v_adj, "value-changed", G_CALLBACK(on_editor_diff_v1_scrolled), st);
    g_signal_connect(st->v2->v_adj, "value-changed", G_CALLBACK(on_editor_diff_v2_scrolled), st);

    gtk_paned_pack1(GTK_PANED(st->paned), hex_view_get_widget(st->v1), TRUE, TRUE);
    gtk_paned_pack2(GTK_PANED(st->paned), hex_view_get_widget(st->v2), TRUE, TRUE);
    gtk_paned_set_position(GTK_PANED(st->paned), 500);

    gtk_box_pack_start(GTK_BOX(vbox), st->paned, TRUE, TRUE, 0);

    // Notebook Tab Header
    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    char tab_lbl[128];
    snprintf(tab_lbl, sizeof(tab_lbl), "Diff: %s ⟷ %s",
             doc1->display_name ? doc1->display_name : "Doc 1",
             doc2->display_name ? doc2->display_name : "Doc 2");
    GtkWidget *tab_label = gtk_label_new(tab_lbl);
    GtkWidget *tclose_btn = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(tclose_btn), GTK_RELIEF_NONE);
    g_signal_connect(tclose_btn, "clicked", G_CALLBACK(on_editor_diff_close), st);
    gtk_box_pack_start(GTK_BOX(tab_box), tab_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tab_box), tclose_btn, FALSE, FALSE, 0);

    GtkWidget *event_box = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(event_box), FALSE);
    gtk_widget_add_events(event_box, GDK_BUTTON_PRESS_MASK);
    gtk_container_add(GTK_CONTAINER(event_box), tab_box);
    g_signal_connect(event_box, "button-press-event", G_CALLBACK(on_diff_tab_button_press), st);
    gtk_widget_show_all(event_box);

    int page = gtk_notebook_append_page(GTK_NOTEBOOK(app->doc_notebook), vbox, event_box);
    gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(app->doc_notebook), vbox, TRUE);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(app->doc_notebook), vbox, TRUE);
    gtk_widget_show_all(vbox);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(app->doc_notebook), page);

    editor_diff_update_ui(st);
}

static gboolean on_app_window_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    App *app = (App *)user_data;
    if (!app) return FALSE;

    guint mod_mask = gtk_accelerator_get_default_mod_mask();
    guint state = event->state & mod_mask;
    guint keyval = gdk_keyval_to_lower(event->keyval);

    GtkWidget *focused = gtk_window_get_focus(GTK_WINDOW(app->window));
    gboolean in_text_entry = (focused && (GTK_IS_ENTRY(focused) || GTK_IS_TEXT_VIEW(focused)));

    // F7 / Shift+F7 for diff navigation if diff tab is open
    if (keyval == GDK_KEY_F7) {
        int cur_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->doc_notebook));
        if (cur_page >= 0) {
            GtkWidget *page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->doc_notebook), cur_page);
            if (page_widget) {
                app_diff_view_state_t *st = (app_diff_view_state_t *)g_object_get_data(G_OBJECT(page_widget), "diff_state");
                if (st) {
                    if (state & GDK_SHIFT_MASK) {
                        on_editor_diff_prev(NULL, st);
                    } else {
                        on_editor_diff_next(NULL, st);
                    }
                    return TRUE;
                }
            }
        }
    }

    // Zoom shortcuts: Ctrl + '+' / '=' / 'KP_Add' (zoom in), Ctrl + '-' / '_' / 'KP_Subtract' (zoom out), Ctrl + '0' (zoom reset)
    // Note: On most keyboards, '+' is typed as Ctrl+Shift+'=', so we handle zoom whenever Ctrl is pressed regardless of Shift.
    if (state & GDK_CONTROL_MASK) {
        if (keyval == GDK_KEY_plus || keyval == GDK_KEY_equal || keyval == GDK_KEY_KP_Add) {
            if (app->active_view) {
                hex_view_zoom_in(app->active_view);
                return TRUE;
            }
        } else if (keyval == GDK_KEY_minus || keyval == GDK_KEY_underscore || keyval == GDK_KEY_KP_Subtract) {
            if (app->active_view) {
                hex_view_zoom_out(app->active_view);
                return TRUE;
            }
        } else if (keyval == GDK_KEY_0 || keyval == GDK_KEY_KP_0) {
            if (app->active_view) {
                hex_view_zoom_reset(app->active_view);
                return TRUE;
            }
        }
    }

    // Ctrl + Shift shortcuts
    if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) {
        if (keyval == GDK_KEY_s) {
            app_save_as_document(app);
            return TRUE;
        } else if (keyval == GDK_KEY_z) {
            if (!in_text_entry) {
                app_redo(app);
                return TRUE;
            }
        }
    }

    // Standard Ctrl shortcuts
    if (state == GDK_CONTROL_MASK) {
        switch (keyval) {
            case GDK_KEY_w:
                app_close_active_document(app);
                return TRUE;
            case GDK_KEY_o:
                app_open_dialog(app);
                return TRUE;
            case GDK_KEY_n:
                app_new_document(app);
                return TRUE;
            case GDK_KEY_s:
                app_save_document(app);
                return TRUE;
            case GDK_KEY_q:
                app_quit(app);
                return TRUE;
            case GDK_KEY_f:
                app_show_find(app);
                return TRUE;
            case GDK_KEY_h:
                app_show_replace(app);
                return TRUE;
            case GDK_KEY_g:
                app_show_goto_dialog(app);
                return TRUE;
            case GDK_KEY_e:
                app_show_select_range_dialog(app);
                return TRUE;
            case GDK_KEY_d:
                app_show_diff_dialog(app);
                return TRUE;
            case GDK_KEY_b:
                if (app->sidebars) {
                    sidebars_toggle_left_collapse(app->sidebars, app);
                }
                return TRUE;
            case GDK_KEY_z:
                if (!in_text_entry) {
                    app_undo(app);
                    return TRUE;
                }
                break;
            case GDK_KEY_y:
                if (!in_text_entry) {
                    app_redo(app);
                    return TRUE;
                }
                break;
            case GDK_KEY_a:
                if (!in_text_entry) {
                    document_t *doc = app_get_active_document(app);
                    if (doc) {
                        doc_select_all(doc);
                        app_update_ui(app);
                    }
                    return TRUE;
                }
                break;
            case GDK_KEY_c:
                if (!in_text_entry) {
                    app_copy(app);
                    return TRUE;
                }
                break;
            case GDK_KEY_x:
                if (!in_text_entry) {
                    app_cut(app);
                    return TRUE;
                }
                break;
            case GDK_KEY_v:
                if (!in_text_entry) {
                    app_paste(app);
                    return TRUE;
                }
                break;
            case GDK_KEY_Page_Down: {
                int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(app->doc_notebook));
                if (n_pages > 1) {
                    int cur = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->doc_notebook));
                    gtk_notebook_set_current_page(GTK_NOTEBOOK(app->doc_notebook), (cur + 1) % n_pages);
                    return TRUE;
                }
                break;
            }
            case GDK_KEY_Page_Up: {
                int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(app->doc_notebook));
                if (n_pages > 1) {
                    int cur = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->doc_notebook));
                    gtk_notebook_set_current_page(GTK_NOTEBOOK(app->doc_notebook), (cur - 1 + n_pages) % n_pages);
                    return TRUE;
                }
                break;
            }
            default:
                break;
        }
    }

    return FALSE;
}

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
    if (app->active_view && app->active_view->drawing_area) {
        gtk_widget_grab_focus(app->active_view->drawing_area);
    }
    gtk_main();
}
