#ifndef HXD_HEX_VIEW_H
#define HXD_HEX_VIEW_H

#include "../core/types.h"
#include "../core/document.h"
#include <gtk/gtk.h>

typedef struct _HexView HexView;

typedef void (*hex_view_change_callback)(HexView *view, void *userdata);

struct _HexView {
    GtkWidget                *container; // GtkBox containing drawing area and scrollbar
    GtkWidget                *drawing_area;
    GtkAdjustment            *v_adj;
    GtkWidget                *v_scrollbar;

    document_t               *doc;
    PangoFontDescription    *font_desc;
    int                       char_width;
    int                       char_height;
    int                       line_height;
    int                       header_height;

    // Layout metrics
    int                       offset_col_x;
    int                       offset_col_w;
    int                       hex_col_x;
    int                       hex_col_w;
    int                       ascii_col_x;
    int                       ascii_col_w;

    // View options
    bool                      column_interlacing;
    bool                      cross_highlighting;
    hxd_hex_case_t            hex_case;

    // Interaction state
    bool                      mouse_selecting;
    hxd_offset_t              sel_anchor_offset;

    // Callbacks
    hex_view_change_callback  on_change;
    void                     *change_userdata;
};

HexView   *hex_view_create(document_t *doc);
void       hex_view_set_document(HexView *view, document_t *doc);
document_t*hex_view_get_document(HexView *view);
GtkWidget *hex_view_get_widget(HexView *view);
void       hex_view_free(HexView *view);

/* View controls */
void       hex_view_set_change_callback(HexView *view, hex_view_change_callback cb, void *userdata);
void       hex_view_scroll_to_offset(HexView *view, hxd_offset_t offset);
void       hex_view_set_column_interlacing(HexView *view, bool enable);
void       hex_view_set_cross_highlighting(HexView *view, bool enable);
void       hex_view_set_hex_case(HexView *view, hxd_hex_case_t hcase);
void       hex_view_set_bytes_per_row(HexView *view, int bpr);
void       hex_view_refresh(HexView *view);

#endif // HXD_HEX_VIEW_H
