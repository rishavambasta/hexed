#ifndef HXD_BOTTOMBAR_H
#define HXD_BOTTOMBAR_H

#include "../core/types.h"
#include <gtk/gtk.h>

typedef struct _App App;

typedef struct _BottomBar {
    GtkWidget *notebook;

    // Checksums
    GtkWidget *cs_scope_combo;
    GtkWidget *cs_tree;
    GtkListStore *cs_store;
    bool       cs_display_hex;

    // Bookmarks
    GtkWidget *bm_tree;
    GtkListStore *bm_store;

    // Statistics
    GtkWidget *stat_lbl_entropy;
    GtkWidget *stat_lbl_total;
    GtkWidget *stat_lbl_nulls;
    GtkWidget *stat_lbl_print;
    GtkWidget *stat_lbl_control;
    GtkWidget *stat_lbl_high;
    GtkWidget *stat_lbl_mean;
    GtkWidget *stat_tree;
    GtkListStore *stat_store;

    // File Comparison
    GtkWidget *fc_entry_path;
    GtkWidget *fc_diff_tree;
    GtkListStore *fc_diff_store;
} BottomBar;

BottomBar *bottombar_create(App *app);
void       bottombar_update(BottomBar *bb, App *app);

#endif // HXD_BOTTOMBAR_H
