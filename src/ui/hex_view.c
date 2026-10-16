#include "hex_view.h"
#include "theme.h"
#include "../core/checksums.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define DEFAULT_FONT "Monospace 10"

static void update_metrics(HexView *view) {
    if (!view || !view->drawing_area) return;

    PangoContext *context = gtk_widget_get_pango_context(view->drawing_area);
    PangoLayout *layout = pango_layout_new(context);
    pango_layout_set_font_description(layout, view->font_desc);
    pango_layout_set_text(layout, "0", -1);

    int w, h;
    pango_layout_get_pixel_size(layout, &w, &h);
    g_object_unref(layout);

    view->char_width = w > 0 ? w : 8;
    view->char_height = h > 0 ? h : 16;
    view->line_height = view->char_height + 4;
    view->header_height = view->line_height + 4;

    int bpr = view->doc ? view->doc->bytes_per_row : 16;
    if (bpr <= 0) bpr = 16;

    // Offset column: e.g. "00000000: " (10 chars)
    view->offset_col_x = 0;
    view->offset_col_w = 11 * view->char_width + 8;

    // Hex column: each byte is 2 chars + 1 space = 3 chars, plus extra space in middle
    view->hex_col_x = view->offset_col_w;
    view->hex_col_w = (bpr * 3 + 1) * view->char_width + 12;

    // ASCII column: bpr chars + padding
    view->ascii_col_x = view->hex_col_x + view->hex_col_w;
    view->ascii_col_w = (bpr + 2) * view->char_width + 16;
}

static void update_scroll_adjustment(HexView *view) {
    if (!view || !view->doc || !view->v_adj) return;

    hxd_size_t doc_size = pt_get_size(view->doc->pt);
    int bpr = view->doc->bytes_per_row;
    if (bpr <= 0) bpr = 16;

    uint64_t total_lines = (doc_size + bpr - 1) / bpr + 1;

    GtkAllocation alloc;
    gtk_widget_get_allocation(view->drawing_area, &alloc);
    int visible_height = alloc.height - view->header_height;
    int page_size = visible_height > 0 ? visible_height / view->line_height : 10;

    gtk_adjustment_configure(
        view->v_adj,
        gtk_adjustment_get_value(view->v_adj),
        0.0,
        (double)total_lines,
        1.0,
        (double)page_size,
        (double)page_size
    );
}

static void notify_change(HexView *view) {
    if (view && view->on_change) {
        view->on_change(view, view->change_userdata);
    }
    gtk_widget_queue_draw(view->drawing_area);
}

void hex_view_scroll_to_offset(HexView *view, hxd_offset_t offset) {
    if (!view || !view->doc || !view->v_adj) return;
    int bpr = view->doc->bytes_per_row;
    if (bpr <= 0) bpr = 16;

    double target_line = (double)(offset / bpr);
    double cur_val = gtk_adjustment_get_value(view->v_adj);
    double page_size = gtk_adjustment_get_page_size(view->v_adj);

    if (target_line < cur_val || target_line >= cur_val + page_size - 1) {
        double new_val = target_line - page_size / 2;
        if (new_val < 0) new_val = 0;
        gtk_adjustment_set_value(view->v_adj, new_val);
    }
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    HexView *view = (HexView *)user_data;
    if (!view || !view->doc) return FALSE;

    const theme_colors_t *tc = theme_get_colors();

    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);

    // 1. Draw overall background
    cairo_set_source_rgb(cr, tc->bg_r, tc->bg_g, tc->bg_b);
    cairo_paint(cr);

    // 2. Draw Header Area
    cairo_set_source_rgb(cr, tc->header_bg_r, tc->header_bg_g, tc->header_bg_b);
    cairo_rectangle(cr, 0, 0, alloc.width, view->header_height);
    cairo_fill(cr);

    // Header divider line
    cairo_set_source_rgb(cr, tc->header_text_r * 0.4, tc->header_text_g * 0.4, tc->header_text_b * 0.4);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, 0, view->header_height - 0.5);
    cairo_line_to(cr, alloc.width, view->header_height - 0.5);
    cairo_stroke(cr);

    // Setup Pango
    PangoContext *ctx = gtk_widget_get_pango_context(widget);
    PangoLayout *layout = pango_layout_new(ctx);
    pango_layout_set_font_description(layout, view->font_desc);

    int bpr = view->doc->bytes_per_row;
    if (bpr <= 0) bpr = 16;

    // Draw Header text
    cairo_set_source_rgb(cr, tc->header_text_r, tc->header_text_g, tc->header_text_b);
    pango_layout_set_text(layout, "Offset(h)", -1);
    cairo_move_to(cr, 8, 2);
    pango_cairo_show_layout(cr, layout);

    for (int col = 0; col < bpr; col++) {
        char col_hdr[8];
        snprintf(col_hdr, sizeof(col_hdr), view->hex_case == HXD_HEX_LOWERCASE ? "%02x" : "%02X", col);
        pango_layout_set_text(layout, col_hdr, -1);
        int col_x = view->hex_col_x + col * (3 * view->char_width) + (col >= 8 ? view->char_width : 0) + 4;
        cairo_move_to(cr, col_x, 2);
        pango_cairo_show_layout(cr, layout);
    }

    pango_layout_set_text(layout, "Decoded text", -1);
    cairo_move_to(cr, view->ascii_col_x + 4, 2);
    pango_cairo_show_layout(cr, layout);

    // 3. Draw Offset Gutter Background
    cairo_set_source_rgb(cr, tc->offset_bg_r, tc->offset_bg_g, tc->offset_bg_b);
    cairo_rectangle(cr, 0, view->header_height, view->offset_col_w, alloc.height - view->header_height);
    cairo_fill(cr);

    // Vertical column divider lines
    cairo_set_source_rgb(cr, tc->header_text_r * 0.3, tc->header_text_g * 0.3, tc->header_text_b * 0.3);
    cairo_move_to(cr, view->hex_col_x - 0.5, 0);
    cairo_line_to(cr, view->hex_col_x - 0.5, alloc.height);
    cairo_move_to(cr, view->ascii_col_x - 0.5, 0);
    cairo_line_to(cr, view->ascii_col_x - 0.5, alloc.height);
    cairo_stroke(cr);

    // 4. Virtual lines rendering
    double scroll_y = gtk_adjustment_get_value(view->v_adj);
    int first_line = (int)scroll_y;
    int lines_to_draw = (alloc.height - view->header_height) / view->line_height + 2;

    hxd_size_t doc_size = pt_get_size(view->doc->pt);
    uint8_t line_buffer[128];
    bool is_gap_buffer[128];

    hxd_offset_t cur_row = view->doc->cursor_offset / bpr;
    int cur_col = (int)(view->doc->cursor_offset % bpr);

    // Column Interlacing
    if (view->column_interlacing) {
        cairo_set_source_rgb(cr, tc->odd_col_bg_r, tc->odd_col_bg_g, tc->odd_col_bg_b);
        for (int col = 1; col < bpr; col += 2) {
            int cx = view->hex_col_x + col * (3 * view->char_width) + (col >= 8 ? view->char_width : 0) + 2;
            cairo_rectangle(cr, cx, view->header_height, 2 * view->char_width + 4, alloc.height - view->header_height);
            cairo_fill(cr);
        }
    }

    // Crosshair column highlight
    if (view->cross_highlighting && cur_col < bpr) {
        cairo_set_source_rgba(cr, tc->cross_bg_r, tc->cross_bg_g, tc->cross_bg_b, 0.4);
        int cx = view->hex_col_x + cur_col * (3 * view->char_width) + (cur_col >= 8 ? view->char_width : 0) + 2;
        cairo_rectangle(cr, cx, view->header_height, 2 * view->char_width + 4, alloc.height - view->header_height);
        cairo_fill(cr);
    }

    for (int l = 0; l < lines_to_draw; l++) {
        uint64_t line_idx = first_line + l;
        hxd_offset_t line_offset = line_idx * bpr;
        if (line_offset > doc_size) break;

        int y = view->header_height + l * view->line_height;

        // Crosshair row highlight
        if (view->cross_highlighting && line_idx == (uint64_t)cur_row) {
            cairo_set_source_rgba(cr, tc->cross_bg_r, tc->cross_bg_g, tc->cross_bg_b, 0.4);
            cairo_rectangle(cr, 0, y, alloc.width, view->line_height);
            cairo_fill(cr);
        }

        // Draw offset string
        char offset_str[32];
        snprintf(offset_str, sizeof(offset_str), "%08llX:", (unsigned long long)line_offset);
        pango_layout_set_text(layout, offset_str, -1);
        cairo_set_source_rgb(cr, tc->offset_text_r, tc->offset_text_g, tc->offset_text_b);
        cairo_move_to(cr, 8, y + 2);
        pango_cairo_show_layout(cr, layout);

        // Read bytes for this line
        int bytes_in_line = 0;
        if (line_offset < doc_size) {
            bytes_in_line = (int)((doc_size - line_offset < (hxd_size_t)bpr) ? (doc_size - line_offset) : (hxd_size_t)bpr);
            pt_read_bytes(view->doc->pt, line_offset, line_buffer, bytes_in_line, is_gap_buffer);
        }

        // Render each byte in Hex and ASCII
        for (int col = 0; col < bpr; col++) {
            hxd_offset_t byte_off = line_offset + col;
            int byte_x = view->hex_col_x + col * (3 * view->char_width) + (col >= 8 ? view->char_width : 0) + 4;
            int ascii_x = view->ascii_col_x + col * view->char_width + 4;

            bool is_selected = (view->doc->selection.length > 0 &&
                                byte_off >= view->doc->selection.start &&
                                byte_off < view->doc->selection.start + view->doc->selection.length);
            bool is_cursor = (byte_off == view->doc->cursor_offset);

            // Selection background
            if (is_selected) {
                cairo_set_source_rgb(cr, tc->sel_bg_r, tc->sel_bg_g, tc->sel_bg_b);
                cairo_rectangle(cr, byte_x - 2, y + 1, 2 * view->char_width + 4, view->line_height - 2);
                cairo_fill(cr);
                cairo_rectangle(cr, ascii_x, y + 1, view->char_width, view->line_height - 2);
                cairo_fill(cr);
            }

            // Cursor highlight
            if (is_cursor) {
                if (view->doc->cursor_in_ascii) {
                    cairo_set_source_rgb(cr, tc->cursor_bg_r, tc->cursor_bg_g, tc->cursor_bg_b);
                    cairo_rectangle(cr, ascii_x, y + 1, view->char_width, view->line_height - 2);
                    cairo_fill(cr);

                    // Secondary border in hex
                    cairo_set_source_rgb(cr, tc->cursor_border_r, tc->cursor_border_g, tc->cursor_border_b);
                    cairo_rectangle(cr, byte_x - 2, y + 1, 2 * view->char_width + 4, view->line_height - 2);
                    cairo_stroke(cr);
                } else {
                    cairo_set_source_rgb(cr, tc->cursor_bg_r, tc->cursor_bg_g, tc->cursor_bg_b);
                    cairo_rectangle(cr, byte_x - 2, y + 1, 2 * view->char_width + 4, view->line_height - 2);
                    cairo_fill(cr);

                    // Secondary border in ascii
                    cairo_set_source_rgb(cr, tc->cursor_border_r, tc->cursor_border_g, tc->cursor_border_b);
                    cairo_rectangle(cr, ascii_x, y + 1, view->char_width, view->line_height - 2);
                    cairo_stroke(cr);

                    // Nibble caret indicator
                    if (view->doc->cursor_nibble == 1) {
                        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
                        cairo_rectangle(cr, byte_x + view->char_width - 1, y + view->line_height - 3, view->char_width, 2);
                        cairo_fill(cr);
                    }
                }
            }

            if (col < bytes_in_line) {
                bool is_gap = is_gap_buffer[col];
                char hex_text[4];
                char ascii_text[2];

                if (is_gap) {
                    strcpy(hex_text, "??");
                    ascii_text[0] = '.';
                    ascii_text[1] = '\0';
                    cairo_set_source_rgb(cr, tc->gap_text_r, tc->gap_text_g, tc->gap_text_b);
                } else {
                    uint8_t val = line_buffer[col];
                    snprintf(hex_text, sizeof(hex_text), view->hex_case == HXD_HEX_LOWERCASE ? "%02x" : "%02X", val);
                    ascii_text[0] = (val >= 32 && val <= 126) ? (char)val : '.';
                    ascii_text[1] = '\0';

                    if (is_cursor) {
                        cairo_set_source_rgb(cr, tc->cursor_text_r, tc->cursor_text_g, tc->cursor_text_b);
                    } else if (is_selected) {
                        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
                    } else {
                        cairo_set_source_rgb(cr, tc->text_r, tc->text_g, tc->text_b);
                    }
                }

                // Show hex
                pango_layout_set_text(layout, hex_text, -1);
                cairo_move_to(cr, byte_x, y + 2);
                pango_cairo_show_layout(cr, layout);

                // Show ascii
                pango_layout_set_text(layout, ascii_text, -1);
                cairo_move_to(cr, ascii_x, y + 2);
                pango_cairo_show_layout(cr, layout);
            }
        }
    }

    g_object_unref(layout);
    return FALSE;
}

static hxd_offset_t coords_to_offset(HexView *view, double x, double y, bool *out_in_ascii, int *out_nibble) {
    if (!view || !view->doc) return 0;
    int bpr = view->doc->bytes_per_row;
    if (bpr <= 0) bpr = 16;

    double scroll_y = gtk_adjustment_get_value(view->v_adj);
    int row = (int)scroll_y + (int)((y - view->header_height) / view->line_height);
    if (row < 0) row = 0;

    bool in_ascii = (x >= view->ascii_col_x);
    if (out_in_ascii) *out_in_ascii = in_ascii;

    int col = 0;
    int nibble = 0;

    if (in_ascii) {
        col = (int)((x - view->ascii_col_x - 4) / view->char_width);
    } else if (x >= view->hex_col_x) {
        int rel_x = (int)(x - view->hex_col_x - 4);
        int byte_w = 3 * view->char_width;
        if (rel_x > 8 * byte_w) rel_x -= view->char_width; // account for mid-gap
        col = rel_x / byte_w;
        int in_byte_x = rel_x % byte_w;
        if (in_byte_x > view->char_width) nibble = 1;
    }

    if (col < 0) col = 0;
    if (col >= bpr) col = bpr - 1;
    if (out_nibble) *out_nibble = nibble;

    hxd_offset_t off = (hxd_offset_t)row * bpr + col;
    hxd_size_t sz = pt_get_size(view->doc->pt);
    if (off > sz) off = sz;
    return off;
}

/* =========================================================================
 * Context Menu & Checksum Helpers
 * ========================================================================= */

static void show_checksum_result_dialog(GtkWidget *parent, const char *algo_name, hxd_range_t range, const char *result_hex, const char *extra_info) {
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        algo_name,
        parent ? GTK_WINDOW(gtk_widget_get_toplevel(parent)) : NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Copy", GTK_RESPONSE_ACCEPT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );
    gtk_window_set_default_size(GTK_WINDOW(dlg), 520, 180);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 16);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    // Header title
    char title_buf[128];
    snprintf(title_buf, sizeof(title_buf), "<b>%s Result</b>", algo_name);
    GtkWidget *lbl_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_title), title_buf);
    gtk_label_set_xalign(GTK_LABEL(lbl_title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_title, FALSE, FALSE, 0);

    // Range info
    char range_buf[128];
    snprintf(range_buf, sizeof(range_buf), "Offset Range: 0x%08llX - 0x%08llX (%llu bytes)",
             (unsigned long long)range.start,
             (unsigned long long)(range.start + range.length),
             (unsigned long long)range.length);
    GtkWidget *lbl_range = gtk_label_new(range_buf);
    gtk_label_set_xalign(GTK_LABEL(lbl_range), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_range, FALSE, FALSE, 0);

    // Text entry for checksum
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), result_hex);
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    gtk_widget_set_can_focus(entry, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

    if (extra_info && strlen(extra_info) > 0) {
        GtkWidget *lbl_extra = gtk_label_new(extra_info);
        gtk_label_set_xalign(GTK_LABEL(lbl_extra), 0.0);
        gtk_box_pack_start(GTK_BOX(vbox), lbl_extra, FALSE, FALSE, 0);
    }

    // Copied feedback
    GtkWidget *lbl_note = gtk_label_new("<small>✔ Copied to clipboard automatically</small>");
    gtk_label_set_use_markup(GTK_LABEL(lbl_note), TRUE);
    gtk_label_set_xalign(GTK_LABEL(lbl_note), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_note, FALSE, FALSE, 0);

    gtk_widget_show_all(dlg);

    // Automatically copy to clipboard
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), result_hex, -1);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), result_hex, -1);
    }
    gtk_widget_destroy(dlg);
}

void hex_view_compute_sha256(HexView *view) {
    if (!view || !view->doc || !view->doc->pt) return;

    hxd_range_t range;
    if (view->doc->selection.length > 0) {
        range = view->doc->selection;
    } else {
        range.start = 0;
        range.length = pt_get_size(view->doc->pt);
    }

    checksum_list_t *list = checksums_calculate(view->doc->pt, range, view->doc->endian);
    if (!list) return;

    const char *sha256_hex = NULL;
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].id, "sha256") == 0) {
            sha256_hex = list->items[i].result_hex;
            break;
        }
    }

    if (sha256_hex) {
        show_checksum_result_dialog(view->drawing_area, "SHA-256", range, sha256_hex, NULL);
    }

    checksums_free(list);
}

void hex_view_compute_crc32(HexView *view) {
    if (!view || !view->doc || !view->doc->pt) return;

    hxd_range_t range;
    if (view->doc->selection.length > 0) {
        range = view->doc->selection;
    } else {
        range.start = 0;
        range.length = pt_get_size(view->doc->pt);
    }

    checksum_list_t *list = checksums_calculate(view->doc->pt, range, view->doc->endian);
    if (!list) return;

    const char *crc32_hex = NULL;
    const char *crc32_dec = NULL;
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].id, "crc32") == 0) {
            crc32_hex = list->items[i].result_hex;
            crc32_dec = list->items[i].result_dec;
            break;
        }
    }

    if (crc32_hex) {
        char extra[160];
        snprintf(extra, sizeof(extra), "Decimal value: %s", crc32_dec ? crc32_dec : "-");
        show_checksum_result_dialog(view->drawing_area, "CRC-32 (IEEE 802.3)", range, crc32_hex, extra);
    }

    checksums_free(list);
}

static void on_menu_cut(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    HexView *view = (HexView *)user_data;
    if (!view || !view->doc || view->doc->selection.length == 0) return;
    
    // Copy
    hxd_size_t len = view->doc->selection.length;
    uint8_t *buf = malloc(len);
    pt_read_bytes(view->doc->pt, view->doc->selection.start, buf, len, NULL);
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

    // Delete
    doc_delete(view->doc);
    update_scroll_adjustment(view);
    notify_change(view);
}

static void on_menu_copy(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    HexView *view = (HexView *)user_data;
    if (!view || !view->doc || view->doc->selection.length == 0) return;

    hxd_size_t len = view->doc->selection.length;
    uint8_t *buf = malloc(len);
    pt_read_bytes(view->doc->pt, view->doc->selection.start, buf, len, NULL);
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

static void on_menu_paste(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    HexView *view = (HexView *)user_data;
    if (!view || !view->doc) return;

    gchar *text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
    if (!text) return;

    size_t text_len = strlen(text);
    uint8_t *bytes = malloc(text_len + 1);
    size_t num_bytes = 0;
    const char *cur = text;
    bool is_hex = true;
    while (*cur) {
        while (*cur && isspace((unsigned char)*cur)) cur++;
        if (!*cur) break;
        if (isxdigit((unsigned char)cur[0]) && isxdigit((unsigned char)cur[1])) {
            char bstr[3] = { cur[0], cur[1], '\0' };
            bytes[num_bytes++] = (uint8_t)strtoul(bstr, NULL, 16);
            cur += 2;
        } else {
            is_hex = false;
            break;
        }
    }

    if (is_hex && num_bytes > 0) {
        doc_paste(view->doc, bytes, num_bytes);
    } else {
        doc_paste(view->doc, (const uint8_t *)text, text_len);
    }
    free(bytes);
    g_free(text);

    update_scroll_adjustment(view);
    notify_change(view);
}

static void on_menu_delete(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    HexView *view = (HexView *)user_data;
    if (!view || !view->doc) return;
    doc_delete(view->doc);
    update_scroll_adjustment(view);
    notify_change(view);
}

static void on_menu_zoom_in(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    HexView *view = (HexView *)user_data;
    hex_view_zoom_in(view);
}

static void on_menu_zoom_out(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    HexView *view = (HexView *)user_data;
    hex_view_zoom_out(view);
}

static void on_menu_zoom_reset(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    HexView *view = (HexView *)user_data;
    hex_view_zoom_reset(view);
}

static void on_menu_sha256(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    HexView *view = (HexView *)user_data;
    hex_view_compute_sha256(view);
}

static void on_menu_crc32(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    HexView *view = (HexView *)user_data;
    hex_view_compute_crc32(view);
}

static void show_context_menu(HexView *view, GdkEventButton *event) {
    GtkWidget *menu = gtk_menu_new();

    bool has_sel = (view->doc && view->doc->selection.length > 0);
    hxd_size_t range_len = has_sel ? view->doc->selection.length : (view->doc ? pt_get_size(view->doc->pt) : 0);

    GtkWidget *m_cut = gtk_menu_item_new_with_label("Cut\tCtrl+X");
    GtkWidget *m_copy = gtk_menu_item_new_with_label("Copy\tCtrl+C");
    GtkWidget *m_paste = gtk_menu_item_new_with_label("Paste\tCtrl+V");
    GtkWidget *m_del = gtk_menu_item_new_with_label("Delete\tDel");

    gtk_widget_set_sensitive(m_cut, has_sel);
    gtk_widget_set_sensitive(m_copy, has_sel);

    g_signal_connect(m_cut, "activate", G_CALLBACK(on_menu_cut), view);
    g_signal_connect(m_copy, "activate", G_CALLBACK(on_menu_copy), view);
    g_signal_connect(m_paste, "activate", G_CALLBACK(on_menu_paste), view);
    g_signal_connect(m_del, "activate", G_CALLBACK(on_menu_delete), view);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_cut);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_copy);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_paste);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_del);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Checksums for range / selection
    char sha_label[128], crc_label[128];
    if (has_sel) {
        snprintf(sha_label, sizeof(sha_label), "Compute SHA-256 (Selection: %llu bytes)", (unsigned long long)range_len);
        snprintf(crc_label, sizeof(crc_label), "Compute CRC-32 (Selection: %llu bytes)", (unsigned long long)range_len);
    } else {
        snprintf(sha_label, sizeof(sha_label), "Compute SHA-256 (Entire File: %llu bytes)", (unsigned long long)range_len);
        snprintf(crc_label, sizeof(crc_label), "Compute CRC-32 (Entire File: %llu bytes)", (unsigned long long)range_len);
    }

    GtkWidget *m_sha256 = gtk_menu_item_new_with_label(sha_label);
    GtkWidget *m_crc32 = gtk_menu_item_new_with_label(crc_label);

    g_signal_connect(m_sha256, "activate", G_CALLBACK(on_menu_sha256), view);
    g_signal_connect(m_crc32, "activate", G_CALLBACK(on_menu_crc32), view);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_sha256);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_crc32);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Zoom options
    char zoom_lbl[64];
    snprintf(zoom_lbl, sizeof(zoom_lbl), "Zoom In\tCtrl++ (%d pt)", view->font_size_pt + 1);
    GtkWidget *m_zin = gtk_menu_item_new_with_label(zoom_lbl);
    snprintf(zoom_lbl, sizeof(zoom_lbl), "Zoom Out\tCtrl+- (%d pt)", view->font_size_pt > 6 ? view->font_size_pt - 1 : 6);
    GtkWidget *m_zout = gtk_menu_item_new_with_label(zoom_lbl);
    GtkWidget *m_zreset = gtk_menu_item_new_with_label("Reset Zoom (100%)\tCtrl+0");

    g_signal_connect(m_zin, "activate", G_CALLBACK(on_menu_zoom_in), view);
    g_signal_connect(m_zout, "activate", G_CALLBACK(on_menu_zoom_out), view);
    g_signal_connect(m_zreset, "activate", G_CALLBACK(on_menu_zoom_reset), view);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_zin);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_zout);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), m_zreset);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    HexView *view = (HexView *)user_data;
    if (!view || !view->doc) return FALSE;

    gtk_widget_grab_focus(widget);

    if (event->button == 1) { // Left click
        bool in_ascii = false;
        int nibble = 0;
        hxd_offset_t off = coords_to_offset(view, event->x, event->y, &in_ascii, &nibble);

        if (event->state & GDK_SHIFT_MASK) {
            // Extend selection
            hxd_offset_t anchor = view->sel_anchor_offset;
            if (off >= anchor) {
                doc_set_selection(view->doc, anchor, off - anchor + 1);
            } else {
                doc_set_selection(view->doc, off, anchor - off + 1);
            }
        } else {
            // New cursor position
            doc_set_cursor(view->doc, off, in_ascii);
            view->doc->cursor_nibble = nibble;
            doc_clear_selection(view->doc);
            view->sel_anchor_offset = off;
            view->mouse_selecting = true;
        }

        notify_change(view);
        return TRUE;
    } else if (event->button == 3) { // Right click
        bool in_ascii = false;
        int nibble = 0;
        hxd_offset_t off = coords_to_offset(view, event->x, event->y, &in_ascii, &nibble);
        hxd_size_t sz = pt_get_size(view->doc->pt);

        bool inside_selection = false;
        if (view->doc->selection.length > 0) {
            hxd_offset_t sel_start = view->doc->selection.start;
            hxd_offset_t sel_end = sel_start + view->doc->selection.length;
            if (off >= sel_start && off < sel_end) {
                inside_selection = true;
            }
        }

        if (!inside_selection && off <= sz) {
            doc_set_cursor(view->doc, off, in_ascii);
            view->doc->cursor_nibble = nibble;
            notify_change(view);
        }

        show_context_menu(view, event);
        return TRUE;
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    (void)widget;
    HexView *view = (HexView *)user_data;
    if (!view || !view->doc || !view->mouse_selecting) return FALSE;

    bool in_ascii = false;
    int nibble = 0;
    hxd_offset_t off = coords_to_offset(view, event->x, event->y, &in_ascii, &nibble);

    hxd_offset_t anchor = view->sel_anchor_offset;
    if (off >= anchor) {
        doc_set_selection(view->doc, anchor, off - anchor + 1);
    } else {
        doc_set_selection(view->doc, off, anchor - off + 1);
    }

    doc_set_cursor(view->doc, off, in_ascii);
    notify_change(view);
    return TRUE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    (void)event;
    HexView *view = (HexView *)user_data;
    if (view) view->mouse_selecting = false;
    return FALSE;
}

static gboolean on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
    (void)widget;
    HexView *view = (HexView *)user_data;
    if (!view || !view->v_adj) return FALSE;

    if (event->state & GDK_CONTROL_MASK) {
        if (event->direction == GDK_SCROLL_UP) {
            hex_view_zoom_in(view);
            return TRUE;
        } else if (event->direction == GDK_SCROLL_DOWN) {
            hex_view_zoom_out(view);
            return TRUE;
        } else if (event->direction == GDK_SCROLL_SMOOTH) {
            gdouble dx, dy;
            if (gdk_event_get_scroll_deltas((GdkEvent *)event, &dx, &dy)) {
                if (dy < 0) hex_view_zoom_in(view);
                else if (dy > 0) hex_view_zoom_out(view);
                return TRUE;
            }
        }
        return TRUE;
    }

    double delta = 3.0;
    if (event->direction == GDK_SCROLL_UP) {
        double val = gtk_adjustment_get_value(view->v_adj) - delta;
        if (val < 0) val = 0;
        gtk_adjustment_set_value(view->v_adj, val);
        return TRUE;
    } else if (event->direction == GDK_SCROLL_DOWN) {
        double val = gtk_adjustment_get_value(view->v_adj) + delta;
        double max_val = gtk_adjustment_get_upper(view->v_adj) - gtk_adjustment_get_page_size(view->v_adj);
        if (val > max_val) val = max_val;
        gtk_adjustment_set_value(view->v_adj, val);
        return TRUE;
    } else if (event->direction == GDK_SCROLL_SMOOTH) {
        gdouble dx, dy;
        if (gdk_event_get_scroll_deltas((GdkEvent *)event, &dx, &dy)) {
            double val = gtk_adjustment_get_value(view->v_adj) + dy * delta;
            double max_val = gtk_adjustment_get_upper(view->v_adj) - gtk_adjustment_get_page_size(view->v_adj);
            if (val < 0) val = 0;
            if (val > max_val) val = max_val;
            gtk_adjustment_set_value(view->v_adj, val);
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    HexView *view = (HexView *)user_data;
    if (!view || !view->doc) return FALSE;

    if (event->state & GDK_CONTROL_MASK) {
        if (event->keyval == GDK_KEY_plus || event->keyval == GDK_KEY_equal || event->keyval == GDK_KEY_KP_Add) {
            hex_view_zoom_in(view);
            return TRUE;
        } else if (event->keyval == GDK_KEY_minus || event->keyval == GDK_KEY_underscore || event->keyval == GDK_KEY_KP_Subtract) {
            hex_view_zoom_out(view);
            return TRUE;
        } else if (event->keyval == GDK_KEY_0 || event->keyval == GDK_KEY_KP_0) {
            hex_view_zoom_reset(view);
            return TRUE;
        }
    }

    document_t *doc = view->doc;
    int bpr = doc->bytes_per_row;
    if (bpr <= 0) bpr = 16;
    hxd_size_t sz = pt_get_size(doc->pt);

    switch (event->keyval) {
        case GDK_KEY_Left:
            if (!doc->cursor_in_ascii && doc->cursor_nibble == 1) {
                doc->cursor_nibble = 0;
            } else if (doc->cursor_offset > 0) {
                doc->cursor_offset--;
                if (!doc->cursor_in_ascii) doc->cursor_nibble = 1;
            }
            doc_clear_selection(doc);
            hex_view_scroll_to_offset(view, doc->cursor_offset);
            notify_change(view);
            return TRUE;

        case GDK_KEY_Right:
            if (!doc->cursor_in_ascii && doc->cursor_nibble == 0) {
                doc->cursor_nibble = 1;
            } else if (doc->cursor_offset < sz) {
                doc->cursor_offset++;
                doc->cursor_nibble = 0;
            }
            doc_clear_selection(doc);
            hex_view_scroll_to_offset(view, doc->cursor_offset);
            notify_change(view);
            return TRUE;

        case GDK_KEY_Up:
            if (doc->cursor_offset >= (hxd_size_t)bpr) {
                doc->cursor_offset -= bpr;
            }
            doc_clear_selection(doc);
            hex_view_scroll_to_offset(view, doc->cursor_offset);
            notify_change(view);
            return TRUE;

        case GDK_KEY_Down:
            if (doc->cursor_offset + bpr <= sz) {
                doc->cursor_offset += bpr;
            } else {
                doc->cursor_offset = sz;
            }
            doc_clear_selection(doc);
            hex_view_scroll_to_offset(view, doc->cursor_offset);
            notify_change(view);
            return TRUE;

        case GDK_KEY_Page_Up: {
            double page_size = gtk_adjustment_get_page_size(view->v_adj);
            int step = (int)page_size * bpr;
            if (doc->cursor_offset > (hxd_size_t)step) doc->cursor_offset -= step;
            else doc->cursor_offset = 0;
            doc_clear_selection(doc);
            hex_view_scroll_to_offset(view, doc->cursor_offset);
            notify_change(view);
            return TRUE;
        }

        case GDK_KEY_Page_Down: {
            double page_size = gtk_adjustment_get_page_size(view->v_adj);
            int step = (int)page_size * bpr;
            if (doc->cursor_offset + step < sz) doc->cursor_offset += step;
            else doc->cursor_offset = sz;
            doc_clear_selection(doc);
            hex_view_scroll_to_offset(view, doc->cursor_offset);
            notify_change(view);
            return TRUE;
        }

        case GDK_KEY_Home:
            if (event->state & GDK_CONTROL_MASK) doc->cursor_offset = 0;
            else doc->cursor_offset = (doc->cursor_offset / bpr) * bpr;
            doc_clear_selection(doc);
            hex_view_scroll_to_offset(view, doc->cursor_offset);
            notify_change(view);
            return TRUE;

        case GDK_KEY_End:
            if (event->state & GDK_CONTROL_MASK) {
                doc->cursor_offset = sz;
            } else {
                hxd_offset_t end_of_line = (doc->cursor_offset / bpr) * bpr + bpr - 1;
                doc->cursor_offset = end_of_line < sz ? end_of_line : sz;
            }
            doc_clear_selection(doc);
            hex_view_scroll_to_offset(view, doc->cursor_offset);
            notify_change(view);
            return TRUE;

        case GDK_KEY_Insert:
            doc->edit_mode = (doc->edit_mode == HXD_MODE_OVERWRITE) ? HXD_MODE_INSERT : HXD_MODE_OVERWRITE;
            notify_change(view);
            return TRUE;

        case GDK_KEY_Delete:
            doc_delete(doc);
            notify_change(view);
            return TRUE;

        case GDK_KEY_BackSpace:
            if (doc->cursor_offset > 0) {
                doc->cursor_offset--;
                doc_delete(doc);
                notify_change(view);
            }
            return TRUE;

        case GDK_KEY_Tab:
            doc->cursor_in_ascii = !doc->cursor_in_ascii;
            doc->cursor_nibble = 0;
            notify_change(view);
            return TRUE;
    }

    // Text / Hex typing
    guint32 unicode = gdk_keyval_to_unicode(event->keyval);
    if (unicode >= 32 && unicode <= 126) {
        char c = (char)unicode;
        if (doc->cursor_in_ascii) {
            doc_type_ascii_char(doc, c);
            update_scroll_adjustment(view);
            hex_view_scroll_to_offset(view, doc->cursor_offset);
            notify_change(view);
            return TRUE;
        } else if (isxdigit((unsigned char)c)) {
            doc_type_hex_digit(doc, c);
            update_scroll_adjustment(view);
            hex_view_scroll_to_offset(view, doc->cursor_offset);
            notify_change(view);
            return TRUE;
        }
    }

    return FALSE;
}

static void on_adj_value_changed(GtkAdjustment *adj, gpointer user_data) {
    (void)adj;
    HexView *view = (HexView *)user_data;
    if (view && view->drawing_area) {
        gtk_widget_queue_draw(view->drawing_area);
    }
}

static void on_size_allocate(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data) {
    (void)widget;
    (void)alloc;
    HexView *view = (HexView *)user_data;
    if (view) {
        update_scroll_adjustment(view);
    }
}

HexView *hex_view_create(document_t *doc) {
    HexView *view = calloc(1, sizeof(HexView));
    view->doc = doc;
    view->font_desc = pango_font_description_from_string(DEFAULT_FONT);
    view->font_size_pt = 10;
    view->column_interlacing = true;
    view->cross_highlighting = true;
    view->hex_case = HXD_HEX_UPPERCASE;

    view->container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    view->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_can_focus(view->drawing_area, TRUE);
    gtk_widget_add_events(view->drawing_area,
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK |
                          GDK_SCROLL_MASK |
                          GDK_KEY_PRESS_MASK);

    view->v_adj = gtk_adjustment_new(0, 0, 100, 1, 10, 10);
    view->v_scrollbar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, view->v_adj);

    gtk_box_pack_start(GTK_BOX(view->container), view->drawing_area, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(view->container), view->v_scrollbar, FALSE, FALSE, 0);

    g_signal_connect(view->drawing_area, "draw", G_CALLBACK(on_draw), view);
    g_signal_connect(view->drawing_area, "button-press-event", G_CALLBACK(on_button_press), view);
    g_signal_connect(view->drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify), view);
    g_signal_connect(view->drawing_area, "button-release-event", G_CALLBACK(on_button_release), view);
    g_signal_connect(view->drawing_area, "scroll-event", G_CALLBACK(on_scroll), view);
    g_signal_connect(view->drawing_area, "key-press-event", G_CALLBACK(on_key_press), view);
    g_signal_connect(view->drawing_area, "size-allocate", G_CALLBACK(on_size_allocate), view);
    g_signal_connect(view->v_adj, "value-changed", G_CALLBACK(on_adj_value_changed), view);

    update_metrics(view);
    update_scroll_adjustment(view);

    return view;
}

void hex_view_set_document(HexView *view, document_t *doc) {
    if (!view) return;
    view->doc = doc;
    update_metrics(view);
    update_scroll_adjustment(view);
    notify_change(view);
}

document_t *hex_view_get_document(HexView *view) {
    return view ? view->doc : NULL;
}

GtkWidget *hex_view_get_widget(HexView *view) {
    return view ? view->container : NULL;
}

void hex_view_free(HexView *view) {
    if (!view) return;
    if (view->font_desc) pango_font_description_free(view->font_desc);
    free(view);
}

void hex_view_set_change_callback(HexView *view, hex_view_change_callback cb, void *userdata) {
    if (!view) return;
    view->on_change = cb;
    view->change_userdata = userdata;
}

void hex_view_set_column_interlacing(HexView *view, bool enable) {
    if (!view) return;
    view->column_interlacing = enable;
    gtk_widget_queue_draw(view->drawing_area);
}

void hex_view_set_cross_highlighting(HexView *view, bool enable) {
    if (!view) return;
    view->cross_highlighting = enable;
    gtk_widget_queue_draw(view->drawing_area);
}

void hex_view_set_hex_case(HexView *view, hxd_hex_case_t hcase) {
    if (!view) return;
    view->hex_case = hcase;
    gtk_widget_queue_draw(view->drawing_area);
}

void hex_view_set_bytes_per_row(HexView *view, int bpr) {
    if (!view || !view->doc || bpr <= 0) return;
    view->doc->bytes_per_row = bpr;
    update_metrics(view);
    update_scroll_adjustment(view);
    gtk_widget_queue_draw(view->drawing_area);
}

void hex_view_refresh(HexView *view) {
    if (!view) return;
    update_scroll_adjustment(view);
    gtk_widget_queue_draw(view->drawing_area);
}

void hex_view_zoom_in(HexView *view) {
    if (!view) return;
    if (view->font_size_pt < 36) {
        view->font_size_pt++;
        pango_font_description_set_size(view->font_desc, view->font_size_pt * PANGO_SCALE);
        update_metrics(view);
        update_scroll_adjustment(view);
        gtk_widget_queue_draw(view->drawing_area);
    }
}

void hex_view_zoom_out(HexView *view) {
    if (!view) return;
    if (view->font_size_pt > 6) {
        view->font_size_pt--;
        pango_font_description_set_size(view->font_desc, view->font_size_pt * PANGO_SCALE);
        update_metrics(view);
        update_scroll_adjustment(view);
        gtk_widget_queue_draw(view->drawing_area);
    }
}

void hex_view_zoom_reset(HexView *view) {
    if (!view) return;
    view->font_size_pt = 10;
    pango_font_description_set_size(view->font_desc, view->font_size_pt * PANGO_SCALE);
    update_metrics(view);
    update_scroll_adjustment(view);
    gtk_widget_queue_draw(view->drawing_area);
}
