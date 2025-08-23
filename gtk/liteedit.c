#include <gtk/gtk.h>

// Легковесный текстовый редактор LiteEdit. Версия 1.0
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
//  _     _ _       _____    _ _ _   
// | |   (_) |_ ___| ____|__| (_) |_ 
// | |   | | __/ _ \  _| / _` | | __|
// | |___| | ||  __/ |__| (_| | | |_ 
// |_____|_|\__\___|_____\__,_|_|\__|

typedef struct {
    GtkWidget *window;
    GtkWidget *text_view;
    GtkTextBuffer *text_buffer;
    gchar *current_file;
} AppWidgets;
static void open_file(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Открыть файл",
                                                    GTK_WINDOW(app->window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Отмена", GTK_RESPONSE_CANCEL,
                                                    "_Открыть", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gchar *content = NULL;
        gsize length = 0;
        GError *error = NULL;
        if (g_file_get_contents(filename, &content, &length, &error)) {
            gtk_text_buffer_set_text(app->text_buffer, content, length);
            g_free(content);
            if (app->current_file)
                g_free(app->current_file);
            app->current_file = g_strdup(filename);
            gchar *title = g_strdup_printf("LiteEdit - %s", filename);
            gtk_window_set_title(GTK_WINDOW(app->window), title);
            g_free(title);
        } else {
            GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                                    GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_OK,
                                                    "Ошибка при открытии файла:\n%s", error->message);
            gtk_dialog_run(GTK_DIALOG(msg));
            gtk_widget_destroy(msg);
            g_error_free(error);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}
static void save_file(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    if (app->current_file == NULL) {
        GtkWidget *dialog = gtk_file_chooser_dialog_new("Сохранить файл",
                                                        GTK_WINDOW(app->window),
                                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                                        "_Отмена", GTK_RESPONSE_CANCEL,
                                                        "_Сохранить", GTK_RESPONSE_ACCEPT,
                                                        NULL);
        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            GtkTextIter start, end;
            gtk_text_buffer_get_start_iter(app->text_buffer, &start);
            gtk_text_buffer_get_end_iter(app->text_buffer, &end);
            gchar *text = gtk_text_buffer_get_text(app->text_buffer, &start, &end, FALSE);
            GError *error = NULL;
            if (!g_file_set_contents(filename, text, -1, &error)) {
                GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                                        GTK_DIALOG_MODAL,
                                                        GTK_MESSAGE_ERROR,
                                                        GTK_BUTTONS_OK,
                                                        "Ошибка при сохранении файла:\n%s", error->message);
                gtk_dialog_run(GTK_DIALOG(msg));
                gtk_widget_destroy(msg);
                g_error_free(error);
            } else {
                if (app->current_file)
                    g_free(app->current_file);
                app->current_file = g_strdup(filename);
                gchar *title = g_strdup_printf("LiteEdit - %s", filename);
                gtk_window_set_title(GTK_WINDOW(app->window), title);
                g_free(title);
            }
            g_free(text);
            g_free(filename);
        }
        gtk_widget_destroy(dialog);
    } else {
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(app->text_buffer, &start);
        gtk_text_buffer_get_end_iter(app->text_buffer, &end);
        gchar *text = gtk_text_buffer_get_text(app->text_buffer, &start, &end, FALSE);
        GError *error = NULL;
        if (!g_file_set_contents(app->current_file, text, -1, &error)) {
            GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                                    GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_OK,
                                                    "Ошибка при сохранении файла:\n%s", error->message);
            gtk_dialog_run(GTK_DIALOG(msg));
            gtk_widget_destroy(msg);
            g_error_free(error);
        }
        g_free(text);
    }
}
static void quit_app(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    if (app->current_file)
        g_free(app->current_file);
    gtk_main_quit();
}
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    AppWidgets *app = g_slice_new(AppWidgets);
    app->current_file = NULL;
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Текстовый редактор LiteEdit");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 600, 400);
    g_signal_connect(app->window, "destroy", G_CALLBACK(quit_app), app);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app->window), vbox);
    GtkWidget *menu_bar = gtk_menu_bar_new();
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *file_item = gtk_menu_item_new_with_label("Файл");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
    GtkWidget *open_item = gtk_menu_item_new_with_label("Открыть");
    GtkWidget *save_item = gtk_menu_item_new_with_label("Сохранить");
    GtkWidget *quit_item = gtk_menu_item_new_with_label("Выход");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_item);
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
    app->text_view = gtk_text_view_new();
    app->text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->text_view));
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), app->text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    g_signal_connect(open_item, "activate", G_CALLBACK(open_file), app);
    g_signal_connect(save_item, "activate", G_CALLBACK(save_file), app);
    g_signal_connect(quit_item, "activate", G_CALLBACK(quit_app), app);
    gtk_widget_show_all(app->window);
    gtk_main();
    g_slice_free(AppWidgets, app);
    return 0;
}

