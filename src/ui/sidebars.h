#ifndef HXD_SIDEBARS_H
#define HXD_SIDEBARS_H

#include "../core/types.h"
#include <gtk/gtk.h>

typedef struct _App App;

typedef struct _Sidebars {
    GtkWidget *left_notebook;
    GtkWidget *right_notebook;

    // Left: Data Inspector
    GtkWidget *di_tree;
    GtkListStore *di_store;
    GtkWidget *btn_di_endian;
    GtkWidget *btn_di_hex;
    GtkWidget *btn_di_editing;
    bool       di_editing_enabled;

    // Left: Base Converter
    GtkWidget *bc_entry_bin;
    GtkWidget *bc_entry_oct;
    GtkWidget *bc_entry_dec;
    GtkWidget *bc_entry_hex;
    GtkWidget *bc_chk_zeros;
    bool       bc_updating;

    // Right: Find & Replace
    GtkWidget *find_entry;
    GtkWidget *replace_entry;
    GtkWidget *find_type_combo;
    GtkWidget *find_chk_case;
    GtkWidget *find_chk_selection;
    GtkWidget *find_results_list;
    GtkListStore *find_results_store;

    // Right: Selection
    GtkWidget *sel_start_hex;
    GtkWidget *sel_start_dec;
    GtkWidget *sel_end_hex;
    GtkWidget *sel_len_hex;
    GtkWidget *sel_len_dec;

    // Right: History
    GtkWidget *history_tree;
    GtkListStore *history_store;
    GtkWidget *chk_history_group;

    // Right: Export
    GtkWidget *export_radio_file;
    GtkWidget *export_radio_url;
    GtkWidget *export_radio_snippet;
    GtkWidget *export_radio_ihex;
    GtkWidget *export_radio_srec;
    GtkWidget *export_combo_snippet_lang;
    GtkWidget *export_spin_bpr;
} Sidebars;

Sidebars *sidebars_create(App *app);
void      sidebars_update(Sidebars *sb, App *app);

#endif // HXD_SIDEBARS_H
