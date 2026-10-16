#ifndef HXD_DIALOGS_H
#define HXD_DIALOGS_H

#include "../core/types.h"
#include <gtk/gtk.h>

typedef struct _App App;

void dialog_show_fill(App *app);
void dialog_show_insert(App *app);
void dialog_show_gap(App *app);
void dialog_show_size(App *app);
void dialog_show_insert_file(App *app);
void dialog_show_diff(App *app);
void dialog_show_settings(App *app);
void dialog_show_controls(App *app);
void dialog_show_about(App *app);

#endif // HXD_DIALOGS_H
