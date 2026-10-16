#ifndef HXD_APP_H
#define HXD_APP_H

#include "../core/types.h"
#include "../core/document.h"
#include "hex_view.h"
#include <gtk/gtk.h>

typedef struct _Sidebars Sidebars;
typedef struct _BottomBar BottomBar;
typedef struct _app_toolbar_t app_toolbar_t;

typedef struct _App {
    GtkWidget     *window;
    GtkWidget     *main_vbox;
    GtkWidget     *toolbar_widget;
    GtkWidget     *paned_h1;
    GtkWidget     *paned_h2;
    GtkWidget     *paned_v;
    GtkWidget     *doc_notebook;
    GtkWidget     *statusbar;

    // Status bar labels
    GtkWidget     *lbl_status_edit;
    GtkWidget     *lbl_status_offset;
    GtkWidget     *lbl_status_sel;
    GtkWidget     *lbl_status_size;
    GtkWidget     *lbl_status_mode;

    // Subcomponents
    app_toolbar_t *toolbar;
    Sidebars      *sidebars;
    BottomBar     *bottombar;

    // Document management
    GList         *hex_views; // List of HexView*
    HexView       *active_view;

    // Application settings
    hxd_theme_t    theme;
    bool           column_interlacing;
    bool           cross_highlighting;
    hxd_hex_case_t hex_case;
    int            bytes_per_row;
} App;

/* App lifecycle */
App        *app_create(void);
void        app_run(App *app, int argc, char **argv);
void        app_update_ui(App *app);

/* Document operations */
document_t *app_get_active_document(App *app);
HexView    *app_get_active_view(App *app);
void        app_new_document(App *app);
void        app_open_document(App *app, const char *path);
void        app_open_dialog(App *app);
void        app_save_document(App *app);
void        app_save_as_document(App *app);
void        app_close_active_document(App *app);

/* Editing commands */
void        app_undo(App *app);
void        app_redo(App *app);
void        app_cut(App *app);
void        app_copy(App *app);
void        app_paste(App *app);
void        app_delete(App *app);
void        app_toggle_mode(App *app);

/* Dialog launchers */
void        app_show_find(App *app);
void        app_show_replace(App *app);
void        app_show_fill_dialog(App *app);
void        app_show_insert_dialog(App *app);
void        app_show_gap_dialog(App *app);
void        app_show_size_dialog(App *app);
void        app_show_insert_file_dialog(App *app);
void        app_show_settings_dialog(App *app);
void        app_show_controls_dialog(App *app);
void        app_show_about_dialog(App *app);

#endif // HXD_APP_H
