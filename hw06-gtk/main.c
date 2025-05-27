#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <gtk/gtk.h>

void fill_file_list_into_tree_view(const char* dir_path, GtkTreeStore* treestore, GtkTreeIter* parent) {
    DIR* dp = opendir(dir_path);

    if (!dp) {
        fprintf(stderr, "Can't open directory: %s!\n", dir_path);
        return;
    }

    struct dirent* de;
    while ((de = readdir(dp))) {
        const size_t len = strlen(de->d_name);
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }
        if (len > 2 && (de->d_type == DT_DIR || de->d_type == DT_REG)) {
            GtkTreeIter child;
            gtk_tree_store_append(treestore, &child, parent);
            gtk_tree_store_set(treestore, &child, 0, de->d_name, -1);
            if (de->d_type == DT_DIR) {
                const size_t nested_dir_path_len = strlen(dir_path) + strlen(de->d_name) + 2;
                char* nested_dir_path = malloc(nested_dir_path_len);
                snprintf(nested_dir_path, nested_dir_path_len, "%s/%s", dir_path, de->d_name);
                fill_file_list_into_tree_view(nested_dir_path, treestore, &child);
            }
        }
    }
    closedir(dp);
}

GtkWidget* create_dir_content_view(const char* path) {
    GtkWidget* view = gtk_tree_view_new();
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* col = gtk_tree_view_column_new_with_attributes("Directory content", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_title(col, "Files");
    gtk_tree_view_column_set_attributes(col, renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

    GtkTreeStore* treestore = gtk_tree_store_new(1, G_TYPE_STRING);
    fill_file_list_into_tree_view(path, treestore, NULL);
    gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(treestore));
    g_object_unref(treestore);

    return view;
}

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;
    GtkWidget* view;

    window = gtk_application_window_new(app);
    g_signal_connect(window, "delete_event", gtk_main_quit, NULL);

    gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);

    view = create_dir_content_view((char*)user_data);

    GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), view);
    gtk_container_add(GTK_CONTAINER(window), scrolled_window);

    gtk_widget_show_all(window);
}

static int handle_command_line(GtkApplication* app, GApplicationCommandLine* cmdline) {
    gchar **argv;
    gint argc;

    argv = g_application_command_line_get_arguments(cmdline, &argc);

    if (argc >= 2) {
        activate(app, (gpointer)argv[1]);
    } else {
        activate(app, (gpointer)".");
    }
    return 0;
}


int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.example.hw06-gtk", G_APPLICATION_HANDLES_COMMAND_LINE);
    //g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    g_signal_connect(app, "command-line", G_CALLBACK(handle_command_line), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
