#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

// Контейнерный терминал VTerm. Версия 1.0
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
// __     _______                   
// \ \   / /_   _|__ _ __ _ __ ___  
//  \ \ / /  | |/ _ \ '__| '_ ` _ \ 
//   \ V /   | |  __/ |  | | | | | |
//    \_/    |_|\___|_|  |_| |_| |_|

typedef struct {
    GtkWidget *window;
    GtkWidget *text_view;
    GtkTextBuffer *buffer;
    GtkWidget *entry;
    GPid child_pid;
    gint master_fd;
    GIOChannel *io_channel;
    gchar *chroot_path;
} TerminalApp;
static gboolean read_pty(GIOChannel *source, GIOCondition cond, gpointer data) {
    TerminalApp *app = (TerminalApp *)data;
    gchar buf[1024];
    gsize bytes_read;
    GError *error = NULL;
    if (cond & (G_IO_HUP | G_IO_ERR)) {
        g_io_channel_shutdown(source, TRUE, NULL);
        return FALSE;
    }
    GIOStatus status = g_io_channel_read_chars(source, buf, sizeof(buf) - 1, &bytes_read, &error);
    if (status == G_IO_STATUS_ERROR) {
        g_printerr("Ошибка чтения из PTY: %s\n", error->message);
        g_error_free(error);
        return FALSE;
    }
    if (bytes_read > 0) {
        buf[bytes_read] = '\0';
        GtkTextIter end_iter;
        gtk_text_buffer_get_end_iter(app->buffer, &end_iter);
        gtk_text_buffer_insert(app->buffer, &end_iter, buf, -1);
        GtkTextMark *mark = gtk_text_buffer_create_mark(app->buffer, NULL, &end_iter, FALSE);
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app->text_view), mark, 0.0, TRUE, 0.0, 1.0);
        gtk_text_buffer_delete_mark(app->buffer, mark);
    }
    return TRUE;
}
static void on_entry_activate(GtkEntry *entry, gpointer data) {
    TerminalApp *app = (TerminalApp *)data;
    const gchar *input = gtk_entry_get_text(entry);
    if (app->master_fd != -1) {
        gchar *cmd = g_strdup_printf("%s\n", input);
        ssize_t written = write(app->master_fd, cmd, strlen(cmd));
        if (written == -1) {
            g_printerr("Ошибка записи в PTY: %s\n", strerror(errno));
        }
        g_free(cmd);
    }
    gtk_entry_set_text(entry, "");
}
static void start_terminal(TerminalApp *app) {
    int master_fd, slave_fd;
    pid_t pid;
    if (openpty(&master_fd, &slave_fd, NULL, NULL, NULL) == -1) {
        g_printerr("Ошибка OpenPTY: %s\n", strerror(errno));
        return;
    }
    pid = fork();
    if (pid == -1) {
        g_printerr("Ошибка Fork: %s\n", strerror(errno));
        return;
    }
    if (pid == 0) {
        close(master_fd);
        if (chroot(app->chroot_path) == -1) {
            perror("chroot");
            _exit(1);
        }
        if (chdir("/") == -1) {
            perror("chdir");
            _exit(1);
        }
        setsid();
        if (ioctl(slave_fd, TIOCSCTTY, 0) == -1) {
            perror("ioctl TIOCSCTTY");
            _exit(1);
        }
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);
        if (slave_fd > STDERR_FILENO)
            close(slave_fd);
        execlp("bash", "bash", NULL);
        perror("execlp bash");
        _exit(1);
    }
    close(slave_fd);
    app->master_fd = master_fd;
    app->child_pid = pid;
    int flags = fcntl(master_fd, F_GETFL);
    fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);
    app->io_channel = g_io_channel_unix_new(master_fd);
    g_io_add_watch(app->io_channel, G_IO_IN | G_IO_HUP | G_IO_ERR, read_pty, app);
}
static void on_folder_selected(GtkFileChooserButton *button, gpointer data) {
    TerminalApp *app = (TerminalApp *)data;
    if (app->chroot_path)
        g_free(app->chroot_path);
    app->chroot_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(button));
    gtk_text_buffer_set_text(app->buffer, "", -1);
    gchar *msg = g_strdup_printf("Выбранный chroot: %s\nЗапуск bash внутри chroot...\n", app->chroot_path);
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(app->buffer, &end_iter);
    gtk_text_buffer_insert(app->buffer, &end_iter, msg, -1);
    g_free(msg);
    start_terminal(app);
}
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    TerminalApp app = {0};
    app.master_fd = -1;
    app.chroot_path = NULL;
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Контейнерный терминал VTerm");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 800, 600);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(app.window), vbox);
    GtkWidget *chooser = gtk_file_chooser_button_new("Выберите путь к chroot", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_box_pack_start(GTK_BOX(vbox), chooser, FALSE, FALSE, 5);
    g_signal_connect(chooser, "file-set", G_CALLBACK(on_folder_selected), &app);
    app.text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app.text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(app.text_view), FALSE);
    app.buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app.text_view));
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled), app.text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 5);
    app.entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), app.entry, FALSE, FALSE, 5);
    g_signal_connect(app.entry, "activate", G_CALLBACK(on_entry_activate), &app);
    gtk_widget_show_all(app.window);
    gtk_main();
    if (app.io_channel)
        g_io_channel_unref(app.io_channel);
    if (app.master_fd != -1)
        close(app.master_fd);
    if (app.chroot_path)
        g_free(app.chroot_path);
    return 0;
}

