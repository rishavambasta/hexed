#ifndef HXD_TOOLBAR_H
#define HXD_TOOLBAR_H

#include "../core/types.h"
#include <gtk/gtk.h>

typedef struct _App App;

typedef struct _app_toolbar_t {
    GtkWidget *toolbar;
    GtkWidget *btn_menu;
    GtkWidget *btn_new;
    GtkWidget *btn_open;
    GtkWidget *btn_save;
    GtkWidget *btn_save_as;
    GtkWidget *btn_undo;
    GtkWidget *btn_redo;
    GtkWidget *btn_copy;
    GtkWidget *btn_paste;
    GtkWidget *btn_find;
    GtkWidget *btn_replace;
    GtkWidget *btn_delete;
    GtkWidget *btn_insert;
    GtkWidget *btn_fill;
    GtkWidget *btn_gap;
    GtkWidget *btn_insert_file;
    GtkWidget *btn_size;
    GtkWidget *btn_select_range;
    GtkWidget *btn_goto;
    GtkWidget *btn_diff;
    GtkWidget *btn_inspector_toggle;
    GtkWidget *btn_baseconv_toggle;
    GtkWidget *btn_mode;
    GtkWidget *lbl_mode;
    GtkWidget *btn_controls;
    GtkWidget *btn_settings;
    GtkWidget *btn_about;
} app_toolbar_t;

app_toolbar_t *toolbar_create(App *app);
void           toolbar_update_state(app_toolbar_t *tb, App *app);

#endif // HXD_TOOLBAR_H
