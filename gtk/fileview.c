#include <gtk/gtk.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

// Просмотрщик файловой системы FileView. Версия 1.0
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
//  _____ _ _    __     ___               
// |  ___(_) | __\ \   / (_) _____      __
// | |_  | | |/ _ \ \ / /| |/ _ \ \ /\ / /
// |  _| | | |  __/\ V / | |  __/\ V  V / 
// |_|   |_|_|\___| \_/  |_|\___| \_/\_/  

typedef struct {
    GtkWidget *window;
    GtkWidget *address_bar;
    GtkWidget *treeview;
    GtkListStore *list_store;
    char *current_path;
} AppWidgets;
enum {
    COL_ICON,
    COL_NAME,
    COL_IS_DIR,
    COL_FULL_PATH,
    NUM_COLS
};
static void update_file_list(AppWidgets *app, const char *path);
static void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                             GtkTreeViewColumn *column, gpointer userdata) {
    AppWidgets *app = (AppWidgets *)userdata;
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(model, &iter, path))
        return;
    gchar *name = NULL;
    gboolean is_dir = FALSE;
    gchar *full_path = NULL;
    gtk_tree_model_get(model, &iter,
                       COL_NAME, &name,
                       COL_IS_DIR, &is_dir,
                       COL_FULL_PATH, &full_path,
                       -1);
    if (name && is_dir) {
        char *new_path = NULL;
        if (strcmp(name, "..") == 0) {
            new_path = g_path_get_dirname(app->current_path);
        } else {
            new_path = g_strdup(full_path);
        }
        update_file_list(app, new_path);
        g_free(new_path);
    }
    g_free(name);
    g_free(full_path);
}
static void on_address_enter(GtkEntry *entry, gpointer userdata) {
    AppWidgets *app = (AppWidgets *)userdata;
    const char *path = gtk_entry_get_text(entry);
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        update_file_list(app, path);
    } else {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "Path is not exist:\n%s", path);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}
static void update_file_list(AppWidgets *app, const char *path) {
    gtk_list_store_clear(app->list_store);
    g_free(app->current_path);
    app->current_path = g_strdup(path);
    gtk_entry_set_text(GTK_ENTRY(app->address_bar), app->current_path);
    DIR *dir = opendir(app->current_path);
    if (!dir) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "Cannot open directory:\n%s\nError: %s", app->current_path, strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0)
            continue;
        char *full_entry_path = g_build_filename(app->current_path, entry->d_name, NULL);
        struct stat st;
        gboolean is_dir = FALSE;
        if (stat(full_entry_path, &st) == 0) {
            is_dir = S_ISDIR(st.st_mode);
        }
        GIcon *icon = NULL;
        if (is_dir) {
            icon = g_themed_icon_new("folder");
        } else {
            icon = g_themed_icon_new("text-x-generic");
        }
        gtk_list_store_insert_with_values(app->list_store, NULL, -1,
                                         COL_ICON, icon,
                                         COL_NAME, entry->d_name,
                                         COL_IS_DIR, is_dir,
                                         COL_FULL_PATH, full_entry_path,
                                         -1);
        g_object_unref(icon);
        g_free(full_entry_path);
    }
    closedir(dir);
}
static char* parse_path_argument(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "--path") == 0 || strcmp(argv[i], "-p") == 0) && i + 1 < argc) {
            return g_strdup(argv[i + 1]);
        }
    }
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            return g_strdup(argv[i]);
        }
    }
    return NULL;
}
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    AppWidgets app = {0};
    char *start_path = parse_path_argument(argc, argv);
    if (!start_path) {
        start_path = g_strdup("/");
    }
    struct stat st;
    if (stat(start_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        g_printerr("Path is not exist: %s\n", start_path);
        g_free(start_path);
        return 1;
    }
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Просмотрщик файловой системы FileView");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 600, 400);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(app.window), vbox);
    app.address_bar = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), app.address_bar, FALSE, FALSE, 0);
    g_signal_connect(app.address_bar, "activate", G_CALLBACK(on_address_enter), &app);
    app.list_store = gtk_list_store_new(NUM_COLS,
                                        G_TYPE_ICON,
                                        G_TYPE_STRING,
                                        G_TYPE_BOOLEAN,
                                        G_TYPE_STRING);
    app.treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.list_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(app.treeview), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), app.treeview, TRUE, TRUE, 0);
    GtkCellRenderer *renderer_icon = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *col_icon = gtk_tree_view_column_new_with_attributes("Icon", renderer_icon,
                                                                          "gicon", COL_ICON,
                                                                          NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.treeview), col_icon);
    GtkCellRenderer *renderer_text = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col_name = gtk_tree_view_column_new_with_attributes("Name", renderer_text,
                                                                          "text", COL_NAME,
                                                                          NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.treeview), col_name);
    g_signal_connect(app.treeview, "row-activated", G_CALLBACK(on_row_activated), &app);
    app.current_path = NULL;
    update_file_list(&app, start_path);
    gtk_widget_show_all(app.window);
    gtk_main();
    g_free(app.current_path);
    g_free(start_path);
    return 0;
}