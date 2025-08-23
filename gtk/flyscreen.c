#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

// FlyScreen - GTK-совместимое средство вывода сообщений на экран. Версия 1.0
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
//  _____ _       ____                           
// |  ___| |_   _/ ___|  ___ _ __ ___  ___ _ __  
// | |_  | | | | \___ \ / __| '__/ _ \/ _ \ '_ \ 
// |  _| | | |_| |___) | (__| | |  __/  __/ | | |
// |_|   |_|\__, |____/ \___|_|  \___|\___|_| |_|
//          |___/                                

int main(int argc, char *argv[]) {
    char *input_text = NULL;
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "--input") == 0 || strcmp(argv[i], "-i") == 0) && i + 1 < argc) {
            input_text = argv[i + 1];
            i++;
        }
    }
    if (!input_text) {
        fprintf(stderr, "Использование: %s --input <text>\n", argv[0]);
        return 1;
    }
    gtk_init(&argc, &argv);
    GtkWidget *dialog = gtk_message_dialog_new(
        NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s",
        input_text
    );
    gtk_window_set_title(GTK_WINDOW(dialog), "");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return 0;
}
