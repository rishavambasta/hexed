#include "theme.h"
#include <string.h>

static hxd_theme_t s_current_theme = HXD_THEME_DARK;
static GtkCssProvider *s_css_provider = NULL;

static const theme_colors_t s_dark_colors = {
    .bg_r = 0.118, .bg_g = 0.118, .bg_b = 0.118,           // #1e1e1e
    .text_r = 0.95, .text_g = 0.95, .text_b = 0.95,        // #f2f2f2
    .offset_bg_r = 0.176, .offset_bg_g = 0.176, .offset_bg_b = 0.176, // #2d2d2d
    .offset_text_r = 0.65, .offset_text_g = 0.75, .offset_text_b = 0.85, // muted cyan
    .header_bg_r = 0.176, .header_bg_g = 0.176, .header_bg_b = 0.176, // #2d2d2d
    .header_text_r = 0.80, .header_text_g = 0.80, .header_text_b = 0.80,
    .odd_col_bg_r = 0.192, .odd_col_bg_g = 0.192, .odd_col_bg_b = 0.200, // #313133 (high-contrast alternating column)
    .cross_bg_r = 0.14, .cross_bg_g = 0.22, .cross_bg_b = 0.32,   // subtle blue
    .sel_bg_r = 0.05, .sel_bg_g = 0.30, .sel_bg_b = 0.55,       // #0c4075
    .cursor_border_r = 1.0, .cursor_border_g = 0.85, .cursor_border_b = 0.2, // gold
    .cursor_bg_r = 0.66, .cursor_bg_g = 0.52, .cursor_bg_b = 0.12,  // #a9851f
    .cursor_text_r = 0.0, .cursor_text_g = 0.0, .cursor_text_b = 0.0,
    .gap_bg_r = 0.18, .gap_bg_g = 0.15, .gap_bg_b = 0.15,
    .gap_text_r = 0.50, .gap_text_g = 0.45, .gap_text_b = 0.45,
    .modified_r = 1.0, .modified_g = 0.20, .modified_b = 0.20   // vivid red for edited bytes
};

static const theme_colors_t s_light_colors = {
    .bg_r = 1.0, .bg_g = 1.0, .bg_b = 1.0,                 // #ffffff
    .text_r = 0.05, .text_g = 0.05, .text_b = 0.05,        // #0d0d0d
    .offset_bg_r = 0.94, .offset_bg_g = 0.94, .offset_bg_b = 0.94, // #f0f0f0
    .offset_text_r = 0.2, .offset_text_g = 0.4, .offset_text_b = 0.6,
    .header_bg_r = 0.94, .header_bg_g = 0.94, .header_bg_b = 0.94,
    .header_text_r = 0.3, .header_text_g = 0.3, .header_text_b = 0.3,
    .odd_col_bg_r = 0.910, .odd_col_bg_g = 0.920, .odd_col_bg_b = 0.935, // #e8ebef (high-contrast alternating column)
    .cross_bg_r = 0.92, .cross_bg_g = 0.96, .cross_bg_b = 1.0,   // subtle blue
    .sel_bg_r = 0.78, .sel_bg_g = 0.91, .sel_bg_b = 0.97,       // #c7eaf7
    .cursor_border_r = 0.8, .cursor_border_g = 0.5, .cursor_border_b = 0.1,
    .cursor_bg_r = 1.0, .cursor_bg_g = 0.83, .cursor_bg_b = 0.58,   // #ffd595
    .cursor_text_r = 0.0, .cursor_text_g = 0.0, .cursor_text_b = 0.0,
    .gap_bg_r = 0.93, .gap_bg_g = 0.90, .gap_bg_b = 0.90,
    .gap_text_r = 0.60, .gap_text_g = 0.60, .gap_text_b = 0.60,
    .modified_r = 0.88, .modified_g = 0.05, .modified_b = 0.05  // vivid red for edited bytes
};

static const theme_colors_t s_bw_colors = {
    .bg_r = 1.0, .bg_g = 1.0, .bg_b = 1.0,
    .text_r = 0.0, .text_g = 0.0, .text_b = 0.0,
    .offset_bg_r = 0.90, .offset_bg_g = 0.90, .offset_bg_b = 0.90,
    .offset_text_r = 0.2, .offset_text_g = 0.2, .offset_text_b = 0.2,
    .header_bg_r = 0.90, .header_bg_g = 0.90, .header_bg_b = 0.90,
    .header_text_r = 0.0, .header_text_g = 0.0, .header_text_b = 0.0,
    .odd_col_bg_r = 0.88, .odd_col_bg_g = 0.88, .odd_col_bg_b = 0.88,   // high-contrast alternating column
    .cross_bg_r = 0.88, .cross_bg_g = 0.88, .cross_bg_b = 0.88,
    .sel_bg_r = 0.70, .sel_bg_g = 0.70, .sel_bg_b = 0.70,
    .cursor_border_r = 0.0, .cursor_border_g = 0.0, .cursor_border_b = 0.0,
    .cursor_bg_r = 0.40, .cursor_bg_g = 0.40, .cursor_bg_b = 0.40,
    .cursor_text_r = 1.0, .cursor_text_g = 1.0, .cursor_text_b = 1.0,
    .gap_bg_r = 0.85, .gap_bg_g = 0.85, .gap_bg_b = 0.85,
    .gap_text_r = 0.50, .gap_text_g = 0.50, .gap_text_b = 0.50,
    .modified_r = 0.75, .modified_g = 0.0, .modified_b = 0.0
};

static const char *s_dark_css =
    "window, .main-grid { background-color: #1e1e1e; color: #ffffff; font-family: sans-serif; }\n"
    "toolbar { background-color: #252525; border-bottom: 1px solid #333333; padding: 2px 4px; }\n"
    "toolbar button { background: transparent; border: none; border-radius: 3px; padding: 3px 6px; color: #eee; font-size: 11px; }\n"
    "toolbar button:hover { background-color: #383838; }\n"
    "toolbar button:active { background-color: #4a4a4a; }\n"
    "toolbar button:disabled { opacity: 0.4; }\n"
    ".mode-badge-ovr { background-color: #2e7d32; color: #fff; font-weight: bold; border-radius: 3px; padding: 2px 6px; font-size: 11px; }\n"
    ".mode-badge-ins { background-color: #d84315; color: #fff; font-weight: bold; border-radius: 3px; padding: 2px 6px; font-size: 11px; }\n"
    "notebook { background-color: #252525; }\n"
    "notebook header.top tab { background-color: #2a2a2a; color: #aaa; border: 1px solid #383838; padding: 4px 10px; font-size: 12px; }\n"
    "notebook header.top tab:checked { background-color: #1e1e1e; color: #fff; border-bottom: 2px solid #0087b8; font-weight: bold; }\n"
    "statusbar { background-color: #252525; border-top: 1px solid #333333; color: #bbb; font-size: 11px; padding: 2px 8px; }\n"
    "treeview { background-color: #1e1e1e; color: #eee; font-size: 12px; }\n"
    "treeview:selected { background-color: #0c4075; color: #fff; }\n"
    "entry, spinbutton { background-color: #2d2d2d; color: #fff; border: 1px solid #444; border-radius: 3px; padding: 3px 6px; }\n"
    "paned > separator { background-color: #333333; }\n"
    "scrollbar slider { background-color: #555555; border-radius: 4px; }\n"
    "scrollbar slider:hover { background-color: #777777; }\n";

static const char *s_light_css =
    "window, .main-grid { background-color: #f8fafc; color: #0f172a; font-family: sans-serif; }\n"
    "toolbar { background-color: #f1f5f9; border-bottom: 1px solid #cbd5e1; padding: 2px 4px; }\n"
    "toolbar button { background: transparent; border: 1px solid transparent; border-radius: 4px; padding: 3px 5px; color: #1e293b; font-size: 11px; margin: 1px; }\n"
    "toolbar button:hover { background-color: #e2e8f0; border-color: #cbd5e1; }\n"
    "toolbar button:active { background-color: #cbd5e1; border-color: #94a3b8; }\n"
    "toolbar button:disabled { opacity: 0.35; }\n"
    ".mode-badge-ovr { background-color: #16a34a; color: #fff; font-weight: bold; border-radius: 3px; padding: 2px 6px; font-size: 11px; }\n"
    ".mode-badge-ins { background-color: #ea580c; color: #fff; font-weight: bold; border-radius: 3px; padding: 2px 6px; font-size: 11px; }\n"
    "notebook { background-color: #f1f5f9; }\n"
    "notebook header.top tab { background-color: #e2e8f0; color: #475569; border: 1px solid #cbd5e1; padding: 4px 10px; font-size: 12px; }\n"
    "notebook header.top tab:checked { background-color: #ffffff; color: #0f172a; border-bottom: 2px solid #0d9488; font-weight: bold; }\n"
    "statusbar { background-color: #f1f5f9; border-top: 1px solid #cbd5e1; color: #475569; font-size: 11px; padding: 2px 8px; }\n"
    "treeview { background-color: #ffffff; color: #0f172a; font-size: 12px; }\n"
    "treeview:selected { background-color: #ccfbf1; color: #0f766e; }\n"
    "entry, spinbutton { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 4px; padding: 3px 6px; }\n"
    "paned > separator { background-color: #cbd5e1; }\n"
    "scrollbar slider { background-color: #94a3b8; border-radius: 4px; }\n"
    "scrollbar slider:hover { background-color: #64748b; }\n";


void theme_init(void) {
    if (!s_css_provider) {
        s_css_provider = gtk_css_provider_new();
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(s_css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    theme_set(HXD_THEME_DARK);
}

void theme_set(hxd_theme_t theme) {
    s_current_theme = theme;
    if (!s_css_provider) return;

    const char *css = (theme == HXD_THEME_LIGHT || theme == HXD_THEME_BW) ? s_light_css : s_dark_css;
    gtk_css_provider_load_from_data(s_css_provider, css, -1, NULL);
}

hxd_theme_t theme_get_current(void) {
    return s_current_theme;
}

const theme_colors_t *theme_get_colors(void) {
    if (s_current_theme == HXD_THEME_LIGHT) return &s_light_colors;
    if (s_current_theme == HXD_THEME_BW) return &s_bw_colors;
    return &s_dark_colors;
}
