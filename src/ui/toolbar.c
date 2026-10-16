#include "toolbar.h"
#include "app.h"
#include "sidebars.h"
#include <stdio.h>
#include <stdlib.h>

static GtkWidget *create_toolbar_button(const char *icon_file, const char *fallback_name, const char *tooltip) {
    GtkWidget *btn = gtk_button_new();
    gtk_widget_set_tooltip_text(btn, tooltip);

    char full_icon_path[256];
    snprintf(full_icon_path, sizeof(full_icon_path), "assets/icons/%s", icon_file);

    GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(full_icon_path, 18, 18, TRUE, NULL);
    GtkWidget *img = NULL;
    if (pb) {
        img = gtk_image_new_from_pixbuf(pb);
        g_object_unref(pb);
    } else {
        img = gtk_image_new_from_icon_name(fallback_name, GTK_ICON_SIZE_MENU);
    }

    gtk_button_set_image(GTK_BUTTON(btn), img);
    gtk_button_set_always_show_image(GTK_BUTTON(btn), TRUE);
    return btn;
}

static void on_toggle_inspector(GtkButton *btn, gpointer user_data) {
    (void)btn;
    App *app = (App *)user_data;
    if (!app || !app->sidebars) return;
    sidebars_set_data_inspector_visible(app->sidebars, app, !app->sidebars->di_visible);
}

static void on_toggle_baseconv(GtkButton *btn, gpointer user_data) {
    (void)btn;
    App *app = (App *)user_data;
    if (!app || !app->sidebars) return;
    sidebars_set_base_converter_visible(app->sidebars, app, !app->sidebars->bc_visible);
}

static void on_toggle_sidebar(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    App *app = (App *)user_data;
    if (!app || !app->sidebars) return;
    sidebars_toggle_left_collapse(app->sidebars, app);
}

static void on_menu_clicked(GtkButton *btn, gpointer user_data) {
    App *app = (App *)user_data;
    GtkWidget *menu = gtk_menu_new();

    // File
    GtkWidget *m_new = gtk_menu_item_new_with_label("New\tCtrl+N");
    GtkWidget *m_open = gtk_menu_item_new_with_label("Open...\tCtrl+O");
    GtkWidget *m_save = gtk_menu_item_new_with_label("Save\tCtrl+S");
    GtkWidget *m_saveas = gtk_menu_item_new_with_label("Save As...\tCtrl+Shift+S");
    GtkWidget *m_close = gtk_menu_item_new_with_label("Close Tab\tCtrl+W");
    GtkWidget *m_sep1 = gtk_separator_menu_item_new();
    GtkWidget *m_diff = gtk_menu_item_new_with_label("Compare / Diff Files...\tCtrl+D");
    GtkWidget *m_sel_range = gtk_menu_item_new_with_label("Select Range...\tCtrl+E");
    GtkWidget *m_goto = gtk_menu_item_new_with_label("Go to Address...\tCtrl+G");
    GtkWidget *m_bitwise = gtk_menu_item_new_with_label("Bitwise Operations... (AND/OR/NOT/XOR)");
    GtkWidget *m_sep2 = gtk_separator_menu_item_new();

    // View panels
    GtkWidget *m_view_sidebar = gtk_menu_item_new_with_label("Toggle Left Sidebar\tCtrl+B");
    GtkWidget *m_view_di = gtk_check_menu_item_new_with_label("Data Inspector");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_view_di), app->sidebars ? app->sidebars->di_visible : TRUE);
    GtkWidget *m_view_bc = gtk_check_menu_item_new_with_label("Base Converter");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_view_bc), app->sidebars ? app->sidebars->bc_visible : TRUE);

    GtkWidget *m_sep3 = gtk_separator_menu_item_new();
    GtkWidget *m_exit = gtk_menu_item_new_with_label("Exit\tCtrl+Q");

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_open);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_save);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_saveas);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_close);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_sep1);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_diff);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_sel_range);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_goto);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_bitwise);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_sep2);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_view_sidebar);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_view_di);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_view_bc);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_sep3);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_exit);

    g_signal_connect_swapped(m_new, "activate", G_CALLBACK(app_new_document), app);
    g_signal_connect_swapped(m_open, "activate", G_CALLBACK(app_open_dialog), app);
    g_signal_connect_swapped(m_save, "activate", G_CALLBACK(app_save_document), app);
    g_signal_connect_swapped(m_saveas, "activate", G_CALLBACK(app_save_as_document), app);
    g_signal_connect_swapped(m_close, "activate", G_CALLBACK(app_close_active_document), app);
    g_signal_connect_swapped(m_diff, "activate", G_CALLBACK(app_show_diff_dialog), app);
    g_signal_connect_swapped(m_sel_range, "activate", G_CALLBACK(app_show_select_range_dialog), app);
    g_signal_connect_swapped(m_goto, "activate", G_CALLBACK(app_show_goto_dialog), app);
    g_signal_connect_swapped(m_bitwise, "activate", G_CALLBACK(app_show_bitwise_op_dialog), app);
    g_signal_connect(m_view_sidebar, "activate", G_CALLBACK(on_toggle_sidebar), app);
    g_signal_connect_swapped(m_view_di, "toggled", G_CALLBACK(on_toggle_inspector), app);
    g_signal_connect_swapped(m_view_bc, "toggled", G_CALLBACK(on_toggle_baseconv), app);
    g_signal_connect_swapped(m_exit, "activate", G_CALLBACK(app_quit), app);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), GTK_WIDGET(btn), GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
}

app_toolbar_t *toolbar_create(App *app) {
    app_toolbar_t *tb = calloc(1, sizeof(app_toolbar_t));
    tb->toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    gtk_style_context_add_class(gtk_widget_get_style_context(tb->toolbar), "he-main-toolbar");

    // Menu button
    tb->btn_menu = create_toolbar_button("menu.svg", "open-menu", "Main Menu");
    g_signal_connect(tb->btn_menu, "clicked", G_CALLBACK(on_menu_clicked), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_menu, FALSE, FALSE, 0);

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), sep, FALSE, FALSE, 2);

    // New, Open, Save, Save As
    tb->btn_new = create_toolbar_button("filenew.svg", "document-new", "New (Ctrl+N)");
    g_signal_connect_swapped(tb->btn_new, "clicked", G_CALLBACK(app_new_document), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_new, FALSE, FALSE, 0);

    tb->btn_open = create_toolbar_button("fileopen.svg", "document-open", "Open (Ctrl+O)");
    g_signal_connect_swapped(tb->btn_open, "clicked", G_CALLBACK(app_open_dialog), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_open, FALSE, FALSE, 0);

    tb->btn_save = create_toolbar_button("filesave.svg", "document-save", "Save (Ctrl+S)");
    g_signal_connect_swapped(tb->btn_save, "clicked", G_CALLBACK(app_save_document), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_save, FALSE, FALSE, 0);

    tb->btn_save_as = create_toolbar_button("filesaveas.svg", "document-save-as", "Save As (Ctrl+Shift+S)");
    g_signal_connect_swapped(tb->btn_save_as, "clicked", G_CALLBACK(app_save_as_document), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_save_as, FALSE, FALSE, 0);

    sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), sep, FALSE, FALSE, 2);

    // Undo, Redo, Copy, Paste
    tb->btn_undo = create_toolbar_button("editundo.svg", "edit-undo", "Undo (Ctrl+Z)");
    g_signal_connect_swapped(tb->btn_undo, "clicked", G_CALLBACK(app_undo), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_undo, FALSE, FALSE, 0);

    tb->btn_redo = create_toolbar_button("editredo.svg", "edit-redo", "Redo (Ctrl+Y)");
    g_signal_connect_swapped(tb->btn_redo, "clicked", G_CALLBACK(app_redo), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_redo, FALSE, FALSE, 0);

    tb->btn_copy = create_toolbar_button("editcopy.svg", "edit-copy", "Copy (Ctrl+C)");
    g_signal_connect_swapped(tb->btn_copy, "clicked", G_CALLBACK(app_copy), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_copy, FALSE, FALSE, 0);

    tb->btn_paste = create_toolbar_button("editpaste.svg", "edit-paste", "Paste (Ctrl+V)");
    g_signal_connect_swapped(tb->btn_paste, "clicked", G_CALLBACK(app_paste), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_paste, FALSE, FALSE, 0);

    sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), sep, FALSE, FALSE, 2);

    // Find, Replace, Delete, Insert, Fill, Gap, Ins. File, Size
    tb->btn_find = create_toolbar_button("editfind.svg", "edit-find", "Find (Ctrl+F)");
    g_signal_connect_swapped(tb->btn_find, "clicked", G_CALLBACK(app_show_find), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_find, FALSE, FALSE, 0);

    tb->btn_replace = create_toolbar_button("editreplace.svg", "edit-find-replace", "Replace (Ctrl+H)");
    g_signal_connect_swapped(tb->btn_replace, "clicked", G_CALLBACK(app_show_replace), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_replace, FALSE, FALSE, 0);

    tb->btn_delete = create_toolbar_button("editclear.svg", "edit-clear", "Delete (Del)");
    g_signal_connect_swapped(tb->btn_delete, "clicked", G_CALLBACK(app_delete), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_delete, FALSE, FALSE, 0);

    tb->btn_insert = create_toolbar_button("editinsert.svg", "list-add", "Insert Block");
    g_signal_connect_swapped(tb->btn_insert, "clicked", G_CALLBACK(app_show_insert_dialog), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_insert, FALSE, FALSE, 0);

    tb->btn_fill = create_toolbar_button("editfill.svg", "color-fill", "Fill Pattern");
    g_signal_connect_swapped(tb->btn_fill, "clicked", G_CALLBACK(app_show_fill_dialog), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_fill, FALSE, FALSE, 0);

    tb->btn_gap = create_toolbar_button("cmd.selection.gap.svg", "bookmark-new", "Make Gap (Sparse)");
    g_signal_connect_swapped(tb->btn_gap, "clicked", G_CALLBACK(app_show_gap_dialog), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_gap, FALSE, FALSE, 0);

    tb->btn_insert_file = create_toolbar_button("editinsertfile.svg", "insert-object", "Insert File...");
    g_signal_connect_swapped(tb->btn_insert_file, "clicked", G_CALLBACK(app_show_insert_file_dialog), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_insert_file, FALSE, FALSE, 0);

    tb->btn_size = create_toolbar_button("editsetsize.svg", "zoom-fit-best", "Set Size...");
    g_signal_connect_swapped(tb->btn_size, "clicked", G_CALLBACK(app_show_size_dialog), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_size, FALSE, FALSE, 0);

    tb->btn_select_range = create_toolbar_button("twin_selection.svg", "edit-select-all", "Select Range / Block (Ctrl+E)");
    g_signal_connect_swapped(tb->btn_select_range, "clicked", G_CALLBACK(app_show_select_range_dialog), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_select_range, FALSE, FALSE, 0);

    tb->btn_goto = create_toolbar_button("twin_pc.svg", "go-jump", "Go to Address / Offset (Ctrl+G)");
    g_signal_connect_swapped(tb->btn_goto, "clicked", G_CALLBACK(app_show_goto_dialog), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_goto, FALSE, FALSE, 0);

    sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), sep, FALSE, FALSE, 2);

    // Diff / Comparison & Sidebar Panel Toggles
    tb->btn_diff = create_toolbar_button("twin_compare.svg", "preferences-system", "Compare / Diff Files (Ctrl+D)");
    g_signal_connect_swapped(tb->btn_diff, "clicked", G_CALLBACK(app_show_diff_dialog), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_diff, FALSE, FALSE, 0);

    tb->btn_inspector_toggle = create_toolbar_button("twin_inspector.svg", "view-list-details", "Toggle Data Inspector Panel");
    g_signal_connect(tb->btn_inspector_toggle, "clicked", G_CALLBACK(on_toggle_inspector), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_inspector_toggle, FALSE, FALSE, 0);

    tb->btn_baseconv_toggle = create_toolbar_button("twin_baseconv2.svg", "accessories-calculator", "Toggle Base Converter Panel");
    g_signal_connect(tb->btn_baseconv_toggle, "clicked", G_CALLBACK(on_toggle_baseconv), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_baseconv_toggle, FALSE, FALSE, 0);

    sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), sep, FALSE, FALSE, 2);

    // Mode Toggle Button (Ovr / Ins)
    tb->btn_mode = gtk_button_new();
    tb->lbl_mode = gtk_label_new("Ovr");
    gtk_style_context_add_class(gtk_widget_get_style_context(tb->lbl_mode), "mode-badge-ovr");
    gtk_container_add(GTK_CONTAINER(tb->btn_mode), tb->lbl_mode);
    gtk_widget_set_tooltip_text(tb->btn_mode, "Toggle Insert/Overwrite Mode (Insert key)");
    g_signal_connect_swapped(tb->btn_mode, "clicked", G_CALLBACK(app_toggle_mode), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_mode, FALSE, FALSE, 0);

    // Spacer to push right buttons
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), spacer, TRUE, TRUE, 0);

    // Controls, Settings, About
    tb->btn_controls = create_toolbar_button("helpkeyboardmap.svg", "help-browser", "Keyboard Shortcuts");
    g_signal_connect_swapped(tb->btn_controls, "clicked", G_CALLBACK(app_show_controls_dialog), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_controls, FALSE, FALSE, 0);

    tb->btn_settings = create_toolbar_button("toolssettings.svg", "preferences-system", "Settings");
    g_signal_connect_swapped(tb->btn_settings, "clicked", G_CALLBACK(app_show_settings_dialog), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_settings, FALSE, FALSE, 0);

    tb->btn_about = create_toolbar_button("helpabout.svg", "help-about", "About Hex Editor Neo");
    g_signal_connect_swapped(tb->btn_about, "clicked", G_CALLBACK(app_show_about_dialog), app);
    gtk_box_pack_start(GTK_BOX(tb->toolbar), tb->btn_about, FALSE, FALSE, 0);

    return tb;
}

void toolbar_update_state(app_toolbar_t *tb, App *app) {
    if (!tb || !app) return;
    document_t *doc = app_get_active_document(app);

    bool has_doc = (doc != NULL);
    bool can_undo = has_doc && doc_can_undo(doc);
    bool can_redo = has_doc && doc_can_redo(doc);
    bool has_sel = has_doc && (doc->selection.length > 0);

    gtk_widget_set_sensitive(tb->btn_save, has_doc && doc->is_modified);
    gtk_widget_set_sensitive(tb->btn_save_as, has_doc);
    gtk_widget_set_sensitive(tb->btn_undo, can_undo);
    gtk_widget_set_sensitive(tb->btn_redo, can_redo);
    gtk_widget_set_sensitive(tb->btn_copy, has_sel);
    gtk_widget_set_sensitive(tb->btn_paste, has_doc);
    gtk_widget_set_sensitive(tb->btn_find, has_doc);
    gtk_widget_set_sensitive(tb->btn_replace, has_doc);
    gtk_widget_set_sensitive(tb->btn_delete, has_doc);
    gtk_widget_set_sensitive(tb->btn_insert, has_doc);
    gtk_widget_set_sensitive(tb->btn_fill, has_doc);
    gtk_widget_set_sensitive(tb->btn_gap, has_doc);
    gtk_widget_set_sensitive(tb->btn_insert_file, has_doc);
    gtk_widget_set_sensitive(tb->btn_size, has_doc);
    gtk_widget_set_sensitive(tb->btn_select_range, has_doc);
    gtk_widget_set_sensitive(tb->btn_goto, has_doc);
    gtk_widget_set_sensitive(tb->btn_diff, TRUE);
    gtk_widget_set_sensitive(tb->btn_inspector_toggle, TRUE);
    gtk_widget_set_sensitive(tb->btn_baseconv_toggle, TRUE);

    if (has_doc) {
        if (doc->edit_mode == HXD_MODE_INSERT) {
            gtk_label_set_text(GTK_LABEL(tb->lbl_mode), "Ins");
            gtk_style_context_remove_class(gtk_widget_get_style_context(tb->lbl_mode), "mode-badge-ovr");
            gtk_style_context_add_class(gtk_widget_get_style_context(tb->lbl_mode), "mode-badge-ins");
        } else {
            gtk_label_set_text(GTK_LABEL(tb->lbl_mode), "Ovr");
            gtk_style_context_remove_class(gtk_widget_get_style_context(tb->lbl_mode), "mode-badge-ins");
            gtk_style_context_add_class(gtk_widget_get_style_context(tb->lbl_mode), "mode-badge-ovr");
        }
    }
}
