#include "file_list.h"

#include <stdio.h>

GSList* get_file_list_by_mask(const gchar *directory, const gchar *pattern) {
    GDir *dir;
    const gchar *filename;
    GSList *file_list = NULL;
    GPatternSpec *pspec;

    pspec = g_pattern_spec_new(pattern);
    if (!pspec) {
        g_warning("Failed to create pattern spec");
        return NULL;
    }

    dir = g_dir_open(directory, 0, NULL);
    if (!dir) {
        g_pattern_spec_free(pspec);
        g_warning("Failed to open directory: %s", directory);
        return NULL;
    }

    while ((filename = g_dir_read_name(dir))) {
        if (g_pattern_spec_match_string(pspec, filename)) {
            gchar *full_path = g_build_filename(directory, filename, NULL);
            file_list = g_slist_append(file_list, full_path);
        }
    }

    g_dir_close(dir);
    g_pattern_spec_free(pspec);

    return file_list;
}

void print_file_list(GSList *file_list) {
    GSList *iter;
    for (iter = file_list; iter != NULL; iter = iter->next) {
        printf("%s\n", (gchar*)iter->data);
    }
}

void free_file_list(GSList *file_list) {
    g_slist_foreach(file_list, (GFunc)g_free, NULL);
    g_slist_free(file_list);
}
