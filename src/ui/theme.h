#ifndef HXD_THEME_H
#define HXD_THEME_H

#include "../core/types.h"
#include <gtk/gtk.h>
#include <stdbool.h>

typedef struct {
    // Colors for Cairo drawing
    double bg_r, bg_g, bg_b;
    double text_r, text_g, text_b;
    double offset_bg_r, offset_bg_g, offset_bg_b;
    double offset_text_r, offset_text_g, offset_text_b;
    double header_bg_r, header_bg_g, header_bg_b;
    double header_text_r, header_text_g, header_text_b;
    double odd_col_bg_r, odd_col_bg_g, odd_col_bg_b;
    double cross_bg_r, cross_bg_g, cross_bg_b;
    double sel_bg_r, sel_bg_g, sel_bg_b;
    double cursor_border_r, cursor_border_g, cursor_border_b;
    double cursor_bg_r, cursor_bg_g, cursor_bg_b;
    double cursor_text_r, cursor_text_g, cursor_text_b;
    double gap_bg_r, gap_bg_g, gap_bg_b;
    double gap_text_r, gap_text_g, gap_text_b;
    double modified_r, modified_g, modified_b;
} theme_colors_t;

void                  theme_init(void);
void                  theme_set(hxd_theme_t theme);
hxd_theme_t           theme_get_current(void);
const theme_colors_t *theme_get_colors(void);
bool                  theme_is_dark(void);

#endif // HXD_THEME_H
