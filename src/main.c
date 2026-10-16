#include "ui/app.h"
#include <gtk/gtk.h>

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    App *app = app_create();
    if (!app) return 1;

    app_run(app, argc, argv);
    return 0;
}
