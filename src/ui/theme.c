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
    "window, window:backdrop, dialog, dialog:backdrop, messagedialog, messagedialog:backdrop { background-color: #1e1e1e; color: #e2e8f0; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; }\n"
    "box, grid { background-color: transparent; }\n"
    "headerbar, .titlebar, headerbar:backdrop, .titlebar:backdrop, window.csd > .titlebar, window.csd > headerbar, .titlebar:not(headerbar) { background: #252525; background-color: #252525; background-image: none; box-shadow: none; border-bottom: 1px solid #333333; color: #ffffff; min-height: 38px; }\n"
    "headerbar .title, .titlebar .title { color: #ffffff; font-weight: bold; font-size: 13px; }\n"
    "headerbar .subtitle, .titlebar .subtitle { color: #94a3b8; font-size: 11px; }\n"
    "headerbar button.titlebutton, .titlebar button.titlebutton, button.titlebutton, button.titlebutton:backdrop { background: transparent; background-color: transparent; background-image: none; border: 1px solid transparent; color: #e2e8f0; box-shadow: none; text-shadow: none; -gtk-icon-shadow: none; }\n"
    "headerbar button.titlebutton:hover, .titlebar button.titlebutton:hover, button.titlebutton:hover { background-color: #383838; background-image: none; border-color: #484848; color: #ffffff; }\n"
    "headerbar button.titlebutton:active, .titlebar button.titlebutton:active, button.titlebutton:active { background-color: #4a4a4a; background-image: none; border-color: #0087b8; color: #ffffff; }\n"
    "dialog headerbar, dialog headerbar:backdrop, dialog .titlebar { background: #252525; background-color: #252525; background-image: none; border-bottom: 1px solid #333333; }\n"
    "dialog .dialog-action-area { background-color: #252525; border-top: 1px solid #333333; }\n"
    "dialog .dialog-action-area button, dialog .dialog-action-area button:backdrop { background-color: #2d2d2d; background-image: none; color: #ffffff; border: 1px solid #444444; border-radius: 4px; padding: 5px 14px; font-size: 12px; }\n"
    "dialog .dialog-action-area button:hover { background-color: #3d3d3d; background-image: none; border-color: #555555; }\n"
    "toolbar, toolbar:backdrop, .he-main-toolbar, .he-main-toolbar:backdrop { background-color: #252525; border-bottom: 1px solid #333333; padding: 2px 4px; }\n"
    "toolbar button, toolbar button:backdrop, .he-main-toolbar button, .he-main-toolbar button:backdrop, .he-main-toolbar button.flat, .he-main-toolbar button.flat:backdrop { background: transparent; background-image: none; border: 1px solid transparent; border-radius: 3px; padding: 3px 5px; color: #e2e8f0; font-size: 11px; margin: 1px; }\n"
    "toolbar button:hover, .he-main-toolbar button:hover, .he-main-toolbar button.flat:hover { background-color: #383838; background-image: none; border-color: #484848; }\n"
    "toolbar button:active, .he-main-toolbar button:active, .he-main-toolbar button.flat:active { background-color: #4a4a4a; background-image: none; border-color: #0087b8; }\n"
    "toolbar button:disabled, toolbar button:disabled:backdrop, .he-main-toolbar button:disabled, .he-main-toolbar button:disabled:backdrop, .he-main-toolbar button.flat:disabled, .he-main-toolbar button.flat:disabled:backdrop { opacity: 0.35; }\n"
    "button, button:backdrop { background-color: #2a2a2a; background-image: none; color: #e2e8f0; border: 1px solid #444444; border-radius: 4px; padding: 4px 10px; font-size: 12px; }\n"
    "button:hover { background-color: #383838; background-image: none; border-color: #555555; color: #ffffff; }\n"
    "button:active, button:checked { background-color: #1e1e1e; background-image: none; border-color: #0087b8; color: #ffffff; }\n"
    "button:disabled, button:disabled:backdrop { background-color: #222222; background-image: none; color: #555555; border-color: #333333; opacity: 0.45; }\n"
    "button label, button:backdrop label { color: inherit; }\n"
    "entry, entry:backdrop, spinbutton, spinbutton:backdrop { background-color: #252525; background-image: none; color: #ffffff; border: 1px solid #444444; border-radius: 4px; padding: 4px 8px; font-size: 12px; }\n"
    "entry:focus, spinbutton:focus { border-color: #0087b8; background-color: #282828; }\n"
    "entry selection, spinbutton selection { background-color: #0c4075; color: #ffffff; }\n"
    "spinbutton button, spinbutton button:backdrop { background-color: #2c2c2c; background-image: none; border: none; color: #e2e8f0; }\n"
    "spinbutton button:hover { background-color: #3a3a3a; background-image: none; }\n"
    "combobox, combobox:backdrop { background: transparent; background-color: transparent; border: none; padding: 0; box-shadow: none; }\n"
    "combobox > box > button.combo, combobox button.combo, combobox > box.linked > button.combo, combobox button, combobox button:backdrop { background: #2a2a2a; background-color: #2a2a2a; background-image: none; box-shadow: none; text-shadow: none; color: #ffffff; border: 1px solid #444444; border-radius: 4px; padding: 3px 8px; font-size: 12px; }\n"
    "combobox > box > button.combo:hover, combobox button.combo:hover, combobox button:hover { background-color: #383838; background-image: none; border-color: #555555; color: #ffffff; }\n"
    "combobox > box > button.combo:checked, combobox button.combo:checked, combobox button:checked, combobox > box > button.combo:active { background-color: #1e1e1e; background-image: none; border-color: #0087b8; color: #ffffff; }\n"
    "combobox cellview, combobox cellview:backdrop, combobox > box > button.combo cellview { color: #ffffff; }\n"
    "combobox arrow, combobox > box > button.combo arrow { color: #94a3b8; min-height: 16px; min-width: 16px; -gtk-icon-source: -gtk-icontheme('pan-down-symbolic'); }\n"
    "combobox window, combobox menu, combobox popover, combobox window.popup { background-color: #252525; color: #ffffff; border: 1px solid #444444; border-radius: 4px; padding: 2px; box-shadow: 0 4px 16px rgba(0,0,0,0.5); }\n"
    "combobox menu menuitem, combobox popover modelbutton { background-color: transparent; color: #ffffff; padding: 4px 10px; border-radius: 3px; font-size: 12px; }\n"
    "combobox menu menuitem:hover, combobox popover modelbutton:hover { background-color: #0c4075; color: #ffffff; }\n"
    "popover, popover:backdrop { background-color: #252525; color: #ffffff; border: 1px solid #444444; }\n"
    "menu, .menu { background-color: #252525; color: #e2e8f0; border: 1px solid #3f3f46; border-radius: 6px; padding: 4px; box-shadow: 0 8px 24px rgba(0,0,0,0.6); }\n"
    "menu menuitem, .menuitem { background-color: transparent; color: #e2e8f0; padding: 5px 12px; border-radius: 4px; font-size: 12px; }\n"
    "menu menuitem:hover, .menuitem:hover { background-color: #0c4075; color: #ffffff; }\n"
    "menu menuitem:disabled, .menuitem:disabled { color: #52525b; }\n"
    "menu menuitem label { color: inherit; }\n"
    "menu menuitem accelerator { color: #71717a; font-size: 11px; }\n"
    "menu menuitem:hover accelerator { color: #bfdbfe; }\n"
    "menu separator { background-color: #3f3f46; min-height: 1px; margin: 4px 2px; }\n"
    "notebook, notebook:backdrop, notebook stack, notebook stack:backdrop { background-color: #1e1e1e; }\n"
    "notebook header, notebook header:backdrop { background-color: #252525; border-bottom: 1px solid #333333; }\n"
    "notebook header.top tab, notebook header.top tab:backdrop { background-color: #2a2a2a; background-image: none; color: #94a3b8; border: 1px solid #383838; border-bottom: none; border-radius: 4px 4px 0 0; padding: 5px 12px; font-size: 12px; margin-right: 2px; }\n"
    "notebook header.top tab:hover { background-color: #343434; background-image: none; color: #e2e8f0; }\n"
    "notebook header.top tab:checked, notebook header.top tab:checked:backdrop { background-color: #1e1e1e; background-image: none; color: #ffffff; border: 1px solid #383838; border-bottom: 2px solid #0087b8; font-weight: bold; }\n"
    "notebook header button, notebook header button:backdrop, notebook header button.flat, notebook header button.flat:backdrop { background: transparent; background-image: none; border: 1px solid transparent; border-radius: 3px; padding: 2px 4px; color: #94a3b8; }\n"
    "notebook header button:hover { background-color: #383838; background-image: none; color: #ffffff; }\n"
    "treeview, treeview:backdrop { background-color: #1e1e1e; color: #e2e8f0; font-size: 12px; }\n"
    "treeview:selected, treeview:selected:focus, treeview:selected:backdrop { background-color: #0c4075; color: #ffffff; }\n"
    "treeview header button, treeview header button:backdrop { background-color: #252525; background-image: none; color: #94a3b8; border: 1px solid #383838; font-weight: bold; font-size: 11px; padding: 4px 8px; }\n"
    "treeview header button:hover { background-color: #333333; background-image: none; color: #e2e8f0; }\n"
    "scrollbar { background-color: transparent; }\n"
    "scrollbar trough, scrollbar trough:backdrop { background-color: #1e1e1e; border: none; }\n"
    "scrollbar slider, scrollbar slider:backdrop { background-color: #484848; background-image: none; border-radius: 4px; min-width: 8px; min-height: 8px; margin: 2px; }\n"
    "scrollbar slider:hover { background-color: #666666; background-image: none; }\n"
    "scrollbar slider:active { background-color: #888888; background-image: none; }\n"
    "paned > separator { background-color: #2d2d2d; min-width: 4px; min-height: 4px; }\n"
    "paned > separator:hover { background-color: #0087b8; }\n"
    "statusbar, statusbar:backdrop { background-color: #252525; border-top: 1px solid #333333; color: #94a3b8; font-size: 11px; padding: 2px 8px; }\n"
    "statusbar label, statusbar:backdrop label { color: #94a3b8; }\n"
    "label, label:backdrop { color: #e2e8f0; }\n"
    "label:disabled, label:disabled:backdrop { color: #52525b; }\n"
    "frame, frame:backdrop, frame > border, frame > border:backdrop { border: 1px solid #383838; border-radius: 4px; }\n"
    "checkbutton, checkbutton:backdrop, radiobutton, radiobutton:backdrop { color: #e2e8f0; }\n"
    "checkbutton check, checkbutton check:backdrop, radiobutton radio, radiobutton radio:backdrop { background-color: #252525; background-image: none; border: 1px solid #555555; color: #ffffff; min-width: 14px; min-height: 14px; border-radius: 3px; }\n"
    "checkbutton check:checked, radiobutton radio:checked { background-color: #0087b8; background-image: none; border-color: #0087b8; color: #ffffff; }\n"
    "scrolledwindow, scrolledwindow:backdrop, viewport, viewport:backdrop { background-color: #1e1e1e; color: #e2e8f0; }\n"
    "progressbar, progressbar:backdrop { background-color: transparent; }\n"
    "progressbar trough, progressbar trough:backdrop { background-color: #252525; border: 1px solid #444444; border-radius: 3px; }\n"
    "progressbar progress, progressbar progress:backdrop { background-color: #0087b8; background-image: none; border-radius: 2px; }\n"
    "tooltip, tooltip.background, tooltip.csd, tooltip window, .tooltip { background-color: #252525; background-image: none; border: 1px solid #444444; border-radius: 4px; box-shadow: none; text-shadow: none; padding: 2px 4px; }\n"
    "tooltip.background { background-color: #252525; background-image: none; border: 1px solid #444444; }\n"
    "tooltip decoration, tooltip.csd decoration { background-color: transparent; border-radius: 4px; box-shadow: none; }\n"
    "tooltip *, tooltip.background *, tooltip label { background-color: transparent; color: #f1f5f9; text-shadow: none; font-size: 11px; padding: 3px 6px; }\n"
    ".mode-badge-ovr { background-color: #2e7d32; color: #ffffff; font-weight: bold; border-radius: 3px; padding: 2px 6px; font-size: 11px; }\n"
    ".mode-badge-ins { background-color: #d84315; color: #ffffff; font-weight: bold; border-radius: 3px; padding: 2px 6px; font-size: 11px; }\n";

static const char *s_light_css =
    "window, window:backdrop, dialog, dialog:backdrop, messagedialog, messagedialog:backdrop { background-color: #f8fafc; color: #0f172a; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; }\n"
    "box, grid { background-color: transparent; }\n"
    "headerbar, .titlebar, headerbar:backdrop, .titlebar:backdrop, window.csd > .titlebar, window.csd > headerbar, .titlebar:not(headerbar) { background: #f1f5f9; background-color: #f1f5f9; background-image: none; box-shadow: none; border-bottom: 1px solid #cbd5e1; color: #0f172a; min-height: 38px; }\n"
    "headerbar .title, .titlebar .title { color: #0f172a; font-weight: bold; font-size: 13px; }\n"
    "headerbar .subtitle, .titlebar .subtitle { color: #64748b; font-size: 11px; }\n"
    "headerbar button.titlebutton, .titlebar button.titlebutton, button.titlebutton, button.titlebutton:backdrop { background: transparent; background-color: transparent; background-image: none; border: 1px solid transparent; color: #334155; box-shadow: none; text-shadow: none; -gtk-icon-shadow: none; }\n"
    "headerbar button.titlebutton:hover, .titlebar button.titlebutton:hover, button.titlebutton:hover { background-color: #e2e8f0; background-image: none; border-color: #cbd5e1; color: #0f172a; }\n"
    "headerbar button.titlebutton:active, .titlebar button.titlebutton:active, button.titlebutton:active { background-color: #cbd5e1; background-image: none; border-color: #94a3b8; color: #0f172a; }\n"
    "dialog headerbar, dialog headerbar:backdrop, dialog .titlebar { background: #f1f5f9; background-color: #f1f5f9; background-image: none; border-bottom: 1px solid #cbd5e1; }\n"
    "dialog .dialog-action-area { background-color: #f1f5f9; border-top: 1px solid #cbd5e1; }\n"
    "dialog .dialog-action-area button, dialog .dialog-action-area button:backdrop { background-color: #ffffff; background-image: none; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 4px; padding: 5px 14px; font-size: 12px; }\n"
    "dialog .dialog-action-area button:hover { background-color: #e2e8f0; background-image: none; border-color: #94a3b8; }\n"
    "toolbar, toolbar:backdrop, .he-main-toolbar, .he-main-toolbar:backdrop { background-color: #f1f5f9; border-bottom: 1px solid #cbd5e1; padding: 2px 4px; }\n"
    "toolbar button, toolbar button:backdrop, .he-main-toolbar button, .he-main-toolbar button:backdrop, .he-main-toolbar button.flat, .he-main-toolbar button.flat:backdrop { background: transparent; background-image: none; border: 1px solid transparent; border-radius: 4px; padding: 3px 5px; color: #1e293b; font-size: 11px; margin: 1px; }\n"
    "toolbar button:hover, .he-main-toolbar button:hover, .he-main-toolbar button.flat:hover { background-color: #e2e8f0; background-image: none; border-color: #cbd5e1; }\n"
    "toolbar button:active, .he-main-toolbar button:active, .he-main-toolbar button.flat:active { background-color: #cbd5e1; background-image: none; border-color: #94a3b8; }\n"
    "toolbar button:disabled, toolbar button:disabled:backdrop, .he-main-toolbar button:disabled, .he-main-toolbar button:disabled:backdrop, .he-main-toolbar button.flat:disabled, .he-main-toolbar button.flat:disabled:backdrop { opacity: 0.35; }\n"
    "button, button:backdrop { background-color: #ffffff; background-image: none; color: #1e293b; border: 1px solid #cbd5e1; border-radius: 4px; padding: 4px 10px; font-size: 12px; }\n"
    "button:hover { background-color: #f1f5f9; background-image: none; border-color: #94a3b8; color: #0f172a; }\n"
    "button:active, button:checked { background-color: #e2e8f0; background-image: none; border-color: #0d9488; color: #0f172a; }\n"
    "button:disabled, button:disabled:backdrop { background-color: #f8fafc; background-image: none; color: #94a3b8; border-color: #e2e8f0; opacity: 0.45; }\n"
    "button label, button:backdrop label { color: inherit; }\n"
    "entry, entry:backdrop, spinbutton, spinbutton:backdrop { background-color: #ffffff; background-image: none; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 4px; padding: 4px 8px; font-size: 12px; }\n"
    "entry:focus, spinbutton:focus { border-color: #0d9488; background-color: #ffffff; }\n"
    "entry selection, spinbutton selection { background-color: #c7eaf7; color: #0f172a; }\n"
    "spinbutton button, spinbutton button:backdrop { background-color: #f8fafc; background-image: none; border: none; color: #1e293b; }\n"
    "spinbutton button:hover { background-color: #e2e8f0; background-image: none; }\n"
    "combobox, combobox:backdrop { background: transparent; background-color: transparent; border: none; padding: 0; box-shadow: none; }\n"
    "combobox > box > button.combo, combobox button.combo, combobox > box.linked > button.combo, combobox button, combobox button:backdrop { background: #ffffff; background-color: #ffffff; background-image: none; box-shadow: none; text-shadow: none; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 4px; padding: 3px 8px; font-size: 12px; }\n"
    "combobox > box > button.combo:hover, combobox button.combo:hover, combobox button:hover { background-color: #f1f5f9; background-image: none; border-color: #94a3b8; color: #0f172a; }\n"
    "combobox > box > button.combo:checked, combobox button.combo:checked, combobox button:checked, combobox > box > button.combo:active { background-color: #e2e8f0; background-image: none; border-color: #0d9488; color: #0f172a; }\n"
    "combobox cellview, combobox cellview:backdrop, combobox > box > button.combo cellview { color: #0f172a; }\n"
    "combobox arrow, combobox > box > button.combo arrow { color: #475569; min-height: 16px; min-width: 16px; -gtk-icon-source: -gtk-icontheme('pan-down-symbolic'); }\n"
    "combobox window, combobox menu, combobox popover, combobox window.popup { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 4px; padding: 2px; box-shadow: 0 4px 16px rgba(0,0,0,0.12); }\n"
    "combobox menu menuitem, combobox popover modelbutton { background-color: transparent; color: #0f172a; padding: 4px 10px; border-radius: 3px; font-size: 12px; }\n"
    "combobox menu menuitem:hover, combobox popover modelbutton:hover { background-color: #e0f2fe; color: #0369a1; }\n"
    "popover, popover:backdrop { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; }\n"
    "menu, .menu { background-color: #ffffff; color: #1e293b; border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px; box-shadow: 0 8px 24px rgba(0,0,0,0.12); }\n"
    "menu menuitem, .menuitem { background-color: transparent; color: #1e293b; padding: 5px 12px; border-radius: 4px; font-size: 12px; }\n"
    "menu menuitem:hover, .menuitem:hover { background-color: #e0f2fe; color: #0369a1; }\n"
    "menu menuitem:disabled, .menuitem:disabled { color: #94a3b8; }\n"
    "menu menuitem label { color: inherit; }\n"
    "menu menuitem accelerator { color: #64748b; font-size: 11px; }\n"
    "menu menuitem:hover accelerator { color: #0284c7; }\n"
    "menu separator { background-color: #e2e8f0; min-height: 1px; margin: 4px 2px; }\n"
    "notebook, notebook:backdrop, notebook stack, notebook stack:backdrop { background-color: #ffffff; }\n"
    "notebook header, notebook header:backdrop { background-color: #f1f5f9; border-bottom: 1px solid #cbd5e1; }\n"
    "notebook header.top tab, notebook header.top tab:backdrop { background-color: #e2e8f0; background-image: none; color: #64748b; border: 1px solid #cbd5e1; border-bottom: none; border-radius: 4px 4px 0 0; padding: 5px 12px; font-size: 12px; margin-right: 2px; }\n"
    "notebook header.top tab:hover { background-color: #f8fafc; background-image: none; color: #1e293b; }\n"
    "notebook header.top tab:checked, notebook header.top tab:checked:backdrop { background-color: #ffffff; background-image: none; color: #0f172a; border: 1px solid #cbd5e1; border-bottom: 2px solid #0d9488; font-weight: bold; }\n"
    "notebook header button, notebook header button:backdrop, notebook header button.flat, notebook header button.flat:backdrop { background: transparent; background-image: none; border: 1px solid transparent; border-radius: 4px; padding: 2px 4px; color: #64748b; }\n"
    "notebook header button:hover { background-color: #e2e8f0; background-image: none; border-color: #cbd5e1; color: #0f172a; }\n"
    "treeview, treeview:backdrop { background-color: #ffffff; color: #0f172a; font-size: 12px; }\n"
    "treeview:selected, treeview:selected:focus, treeview:selected:backdrop { background-color: #ccfbf1; color: #0f766e; }\n"
    "treeview header button, treeview header button:backdrop { background-color: #f1f5f9; background-image: none; color: #475569; border: 1px solid #cbd5e1; font-weight: bold; font-size: 11px; padding: 4px 8px; }\n"
    "treeview header button:hover { background-color: #e2e8f0; background-image: none; color: #0f172a; }\n"
    "scrollbar { background-color: transparent; }\n"
    "scrollbar trough, scrollbar trough:backdrop { background-color: #f8fafc; border: none; }\n"
    "scrollbar slider, scrollbar slider:backdrop { background-color: #cbd5e1; background-image: none; border-radius: 4px; min-width: 8px; min-height: 8px; margin: 2px; }\n"
    "scrollbar slider:hover { background-color: #94a3b8; background-image: none; }\n"
    "scrollbar slider:active { background-color: #64748b; background-image: none; }\n"
    "paned > separator { background-color: #cbd5e1; min-width: 4px; min-height: 4px; }\n"
    "paned > separator:hover { background-color: #0d9488; }\n"
    "statusbar, statusbar:backdrop { background-color: #f1f5f9; border-top: 1px solid #cbd5e1; color: #475569; font-size: 11px; padding: 2px 8px; }\n"
    "statusbar label, statusbar:backdrop label { color: #475569; }\n"
    "label, label:backdrop { color: #0f172a; }\n"
    "label:disabled, label:disabled:backdrop { color: #94a3b8; }\n"
    "frame, frame:backdrop, frame > border, frame > border:backdrop { border: 1px solid #cbd5e1; border-radius: 4px; }\n"
    "checkbutton, checkbutton:backdrop, radiobutton, radiobutton:backdrop { color: #0f172a; }\n"
    "checkbutton check, checkbutton check:backdrop, radiobutton radio, radiobutton radio:backdrop { background-color: #ffffff; background-image: none; border: 1px solid #94a3b8; color: #0f172a; min-width: 14px; min-height: 14px; border-radius: 3px; }\n"
    "checkbutton check:checked, radiobutton radio:checked { background-color: #0d9488; background-image: none; border-color: #0d9488; color: #ffffff; }\n"
    "scrolledwindow, scrolledwindow:backdrop, viewport, viewport:backdrop { background-color: #ffffff; color: #0f172a; }\n"
    "progressbar, progressbar:backdrop { background-color: transparent; }\n"
    "progressbar trough, progressbar trough:backdrop { background-color: #e2e8f0; border: 1px solid #cbd5e1; border-radius: 3px; }\n"
    "progressbar progress, progressbar progress:backdrop { background-color: #0d9488; background-image: none; border-radius: 2px; }\n"
    "tooltip, tooltip.background, tooltip.csd, tooltip window, .tooltip { background-color: #ffffff; background-image: none; border: 1px solid #cbd5e1; border-radius: 4px; box-shadow: 0 2px 8px rgba(0, 0, 0, 0.12); text-shadow: none; padding: 2px 4px; }\n"
    "tooltip.background { background-color: #ffffff; background-image: none; border: 1px solid #cbd5e1; }\n"
    "tooltip decoration, tooltip.csd decoration { background-color: transparent; border-radius: 4px; box-shadow: none; }\n"
    "tooltip *, tooltip.background *, tooltip label { background-color: transparent; color: #0f172a; text-shadow: none; font-size: 11px; padding: 3px 6px; }\n"
    ".mode-badge-ovr { background-color: #16a34a; color: #ffffff; font-weight: bold; border-radius: 3px; padding: 2px 6px; font-size: 11px; }\n"
    ".mode-badge-ins { background-color: #ea580c; color: #ffffff; font-weight: bold; border-radius: 3px; padding: 2px 6px; font-size: 11px; }\n";

static const char *s_bw_css =
    "window, window:backdrop, dialog, dialog:backdrop, messagedialog, messagedialog:backdrop { background-color: #ffffff; color: #000000; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; }\n"
    "box, grid { background-color: transparent; }\n"
    "headerbar, .titlebar, headerbar:backdrop, .titlebar:backdrop, window.csd > .titlebar, window.csd > headerbar, .titlebar:not(headerbar) { background: #ffffff; background-color: #ffffff; background-image: none; box-shadow: none; border-bottom: 2px solid #000000; color: #000000; min-height: 38px; }\n"
    "headerbar .title, .titlebar .title { color: #000000; font-weight: bold; font-size: 13px; }\n"
    "headerbar .subtitle, .titlebar .subtitle { color: #000000; font-size: 11px; }\n"
    "headerbar button.titlebutton, .titlebar button.titlebutton, button.titlebutton, button.titlebutton:backdrop { background: #ffffff; background-color: #ffffff; background-image: none; border: 1px solid #000000; border-radius: 2px; color: #000000; box-shadow: none; text-shadow: none; -gtk-icon-shadow: none; }\n"
    "headerbar button.titlebutton:hover, .titlebar button.titlebutton:hover, button.titlebutton:hover { background-color: #000000; background-image: none; border-color: #000000; color: #ffffff; }\n"
    "headerbar button.titlebutton:active, .titlebar button.titlebutton:active, button.titlebutton:active { background-color: #000000; background-image: none; border-color: #000000; color: #ffffff; }\n"
    "dialog headerbar, dialog headerbar:backdrop, dialog .titlebar { background: #ffffff; background-color: #ffffff; background-image: none; border-bottom: 2px solid #000000; }\n"
    "dialog .dialog-action-area { background-color: #ffffff; border-top: 2px solid #000000; }\n"
    "dialog .dialog-action-area button, dialog .dialog-action-area button:backdrop { background-color: #ffffff; background-image: none; color: #000000; border: 2px solid #000000; border-radius: 2px; padding: 5px 14px; font-size: 12px; font-weight: bold; }\n"
    "dialog .dialog-action-area button:hover { background-color: #000000; background-image: none; color: #ffffff; }\n"
    "toolbar, toolbar:backdrop, .he-main-toolbar, .he-main-toolbar:backdrop { background-color: #ffffff; border-bottom: 2px solid #000000; padding: 2px 4px; }\n"
    "toolbar button, toolbar button:backdrop, .he-main-toolbar button, .he-main-toolbar button:backdrop, .he-main-toolbar button.flat, .he-main-toolbar button.flat:backdrop { background: transparent; background-image: none; border: 1px solid transparent; border-radius: 2px; padding: 3px 5px; color: #000000; font-size: 11px; margin: 1px; }\n"
    "toolbar button:hover, .he-main-toolbar button:hover, .he-main-toolbar button.flat:hover { background-color: #eeeeee; background-image: none; border-color: #000000; }\n"
    "toolbar button:active, .he-main-toolbar button:active, .he-main-toolbar button.flat:active { background-color: #000000; background-image: none; color: #ffffff; border-color: #000000; }\n"
    "toolbar button:disabled, toolbar button:disabled:backdrop, .he-main-toolbar button:disabled, .he-main-toolbar button:disabled:backdrop, .he-main-toolbar button.flat:disabled, .he-main-toolbar button.flat:disabled:backdrop { opacity: 0.35; }\n"
    "button, button:backdrop { background-color: #ffffff; background-image: none; color: #000000; border: 1px solid #000000; border-radius: 2px; padding: 4px 10px; font-size: 12px; }\n"
    "button:hover { background-color: #e0e0e0; background-image: none; }\n"
    "button:active, button:checked { background-color: #000000; background-image: none; color: #ffffff; }\n"
    "button:disabled, button:disabled:backdrop { background-color: #ffffff; background-image: none; color: #888888; border-color: #888888; opacity: 0.35; }\n"
    "button label, button:backdrop label { color: inherit; }\n"
    "entry, entry:backdrop, spinbutton, spinbutton:backdrop { background-color: #ffffff; background-image: none; color: #000000; border: 1px solid #000000; border-radius: 2px; padding: 4px 8px; font-size: 12px; }\n"
    "entry:focus, spinbutton:focus { border: 2px solid #000000; }\n"
    "entry selection, spinbutton selection { background-color: #000000; color: #ffffff; }\n"
    "spinbutton button, spinbutton button:backdrop { background-color: #ffffff; background-image: none; border: none; color: #000000; }\n"
    "spinbutton button:hover { background-color: #e0e0e0; background-image: none; }\n"
    "combobox, combobox:backdrop { background: transparent; background-color: transparent; border: none; padding: 0; box-shadow: none; }\n"
    "combobox > box > button.combo, combobox button.combo, combobox > box.linked > button.combo, combobox button, combobox button:backdrop { background: #ffffff; background-color: #ffffff; background-image: none; box-shadow: none; text-shadow: none; color: #000000; border: 2px solid #000000; border-radius: 2px; padding: 3px 8px; font-size: 12px; font-weight: bold; }\n"
    "combobox > box > button.combo:hover, combobox button.combo:hover, combobox button:hover { background-color: #e0e0e0; background-image: none; border-color: #000000; color: #000000; }\n"
    "combobox > box > button.combo:checked, combobox button.combo:checked, combobox button:checked, combobox > box > button.combo:active { background-color: #000000; background-image: none; border-color: #000000; color: #ffffff; }\n"
    "combobox cellview, combobox cellview:backdrop, combobox > box > button.combo cellview { color: #000000; }\n"
    "combobox arrow, combobox > box > button.combo arrow { color: #000000; min-height: 16px; min-width: 16px; -gtk-icon-source: -gtk-icontheme('pan-down-symbolic'); }\n"
    "combobox window, combobox menu, combobox popover, combobox window.popup { background-color: #ffffff; color: #000000; border: 2px solid #000000; border-radius: 2px; padding: 2px; box-shadow: none; }\n"
    "combobox menu menuitem, combobox popover modelbutton { background-color: #ffffff; color: #000000; padding: 4px 10px; font-size: 12px; font-weight: bold; }\n"
    "combobox menu menuitem:hover, combobox popover modelbutton:hover { background-color: #000000; color: #ffffff; }\n"
    "popover, popover:backdrop { background-color: #ffffff; color: #000000; border: 2px solid #000000; }\n"
    "menu, .menu { background-color: #ffffff; color: #000000; border: 2px solid #000000; border-radius: 2px; padding: 2px; }\n"
    "menu menuitem, .menuitem { background-color: transparent; color: #000000; padding: 5px 12px; font-size: 12px; }\n"
    "menu menuitem:hover, .menuitem:hover { background-color: #000000; color: #ffffff; }\n"
    "menu menuitem:disabled, .menuitem:disabled { color: #888888; }\n"
    "menu menuitem label { color: inherit; }\n"
    "menu menuitem accelerator { color: #666666; font-size: 11px; }\n"
    "menu menuitem:hover accelerator { color: #ffffff; }\n"
    "menu separator { background-color: #000000; min-height: 1px; margin: 2px 0; }\n"
    "notebook, notebook:backdrop, notebook stack, notebook stack:backdrop { background-color: #ffffff; }\n"
    "notebook header, notebook header:backdrop { background-color: #f0f0f0; border-bottom: 2px solid #000000; }\n"
    "notebook header.top tab, notebook header.top tab:backdrop { background-color: #e0e0e0; background-image: none; color: #000000; border: 1px solid #000000; border-bottom: none; border-radius: 2px 2px 0 0; padding: 5px 12px; font-size: 12px; margin-right: 2px; }\n"
    "notebook header.top tab:hover { background-color: #ffffff; background-image: none; }\n"
    "notebook header.top tab:checked, notebook header.top tab:checked:backdrop { background-color: #ffffff; background-image: none; color: #000000; border: 2px solid #000000; border-bottom: 3px solid #000000; font-weight: bold; }\n"
    "notebook header button, notebook header button:backdrop, notebook header button.flat, notebook header button.flat:backdrop { background: transparent; background-image: none; border: 1px solid transparent; border-radius: 2px; padding: 2px 4px; color: #000000; }\n"
    "notebook header button:hover { background-color: #eeeeee; background-image: none; border-color: #000000; color: #000000; }\n"
    "treeview, treeview:backdrop { background-color: #ffffff; color: #000000; font-size: 12px; }\n"
    "treeview:selected, treeview:selected:focus, treeview:selected:backdrop { background-color: #000000; color: #ffffff; }\n"
    "treeview header button, treeview header button:backdrop { background-color: #eeeeee; background-image: none; color: #000000; border: 1px solid #000000; font-weight: bold; font-size: 11px; padding: 4px 8px; }\n"
    "treeview header button:hover { background-color: #dddddd; background-image: none; }\n"
    "scrollbar { background-color: transparent; }\n"
    "scrollbar trough, scrollbar trough:backdrop { background-color: #ffffff; border: 1px solid #000000; }\n"
    "scrollbar slider, scrollbar slider:backdrop { background-color: #666666; background-image: none; border-radius: 2px; min-width: 8px; min-height: 8px; margin: 1px; }\n"
    "scrollbar slider:hover { background-color: #000000; background-image: none; }\n"
    "paned > separator { background-color: #000000; min-width: 3px; min-height: 3px; }\n"
    "statusbar, statusbar:backdrop { background-color: #ffffff; border-top: 2px solid #000000; color: #000000; font-size: 11px; padding: 2px 8px; }\n"
    "statusbar label, statusbar:backdrop label { color: #000000; }\n"
    "label, label:backdrop { color: #000000; }\n"
    "label:disabled, label:disabled:backdrop { color: #888888; }\n"
    "frame, frame:backdrop, frame > border, frame > border:backdrop { border: 1px solid #000000; border-radius: 2px; }\n"
    "checkbutton, checkbutton:backdrop, radiobutton, radiobutton:backdrop { color: #000000; }\n"
    "checkbutton check, checkbutton check:backdrop, radiobutton radio, radiobutton radio:backdrop { background-color: #ffffff; background-image: none; border: 1px solid #000000; color: #000000; min-width: 14px; min-height: 14px; border-radius: 2px; }\n"
    "checkbutton check:checked, radiobutton radio:checked { background-color: #000000; background-image: none; border-color: #000000; color: #ffffff; }\n"
    "scrolledwindow, scrolledwindow:backdrop, viewport, viewport:backdrop { background-color: #ffffff; color: #000000; }\n"
    "progressbar, progressbar:backdrop { background-color: transparent; }\n"
    "progressbar trough, progressbar trough:backdrop { background-color: #ffffff; border: 1px solid #000000; border-radius: 2px; }\n"
    "progressbar progress, progressbar progress:backdrop { background-color: #000000; background-image: none; border-radius: 2px; }\n"
    "tooltip, tooltip.background, tooltip.csd, tooltip window, .tooltip { background-color: #ffffff; background-image: none; border: 2px solid #000000; border-radius: 2px; box-shadow: none; text-shadow: none; padding: 2px 4px; }\n"
    "tooltip.background { background-color: #ffffff; background-image: none; border: 2px solid #000000; }\n"
    "tooltip decoration, tooltip.csd decoration { background-color: transparent; border-radius: 2px; box-shadow: none; }\n"
    "tooltip *, tooltip.background *, tooltip label { background-color: transparent; color: #000000; text-shadow: none; font-weight: bold; font-size: 11px; padding: 3px 6px; }\n"
    "button .mode-badge-ovr, statusbar .mode-badge-ovr, label.mode-badge-ovr, .mode-badge-ovr { background-color: #000000; color: #ffffff; font-weight: bold; border: 1px solid #000000; border-radius: 2px; padding: 2px 6px; font-size: 11px; }\n"
    "button .mode-badge-ins, statusbar .mode-badge-ins, label.mode-badge-ins, .mode-badge-ins { background-color: #ffffff; color: #000000; font-weight: bold; border: 2px solid #000000; border-radius: 2px; padding: 1px 5px; font-size: 11px; }\n";

void theme_init(void) {
    if (!s_css_provider) {
        s_css_provider = gtk_css_provider_new();
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(s_css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER + 50
        );
    }
    theme_set(s_current_theme);
}

void theme_set(hxd_theme_t theme) {
    s_current_theme = theme;

    // Synchronize host OS GTK theme dark mode preference
    GtkSettings *settings = gtk_settings_get_default();
    if (settings) {
        g_object_set(settings, "gtk-application-prefer-dark-theme", (theme == HXD_THEME_DARK), NULL);
    }

    if (!s_css_provider) return;

    const char *css = s_dark_css;
    if (theme == HXD_THEME_LIGHT) css = s_light_css;
    else if (theme == HXD_THEME_BW) css = s_bw_css;

    gtk_css_provider_load_from_data(s_css_provider, css, -1, NULL);
    gtk_style_context_reset_widgets(gdk_screen_get_default());
}

hxd_theme_t theme_get_current(void) {
    return s_current_theme;
}

bool theme_is_dark(void) {
    return s_current_theme == HXD_THEME_DARK;
}

const theme_colors_t *theme_get_colors(void) {
    if (s_current_theme == HXD_THEME_LIGHT) return &s_light_colors;
    if (s_current_theme == HXD_THEME_BW) return &s_bw_colors;
    return &s_dark_colors;
}
