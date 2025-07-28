#pragma once

#include <glib.h>

GSList* get_file_list_by_mask(const gchar* directory, const gchar* pattern);

void print_file_list(GSList* file_list);

void free_file_list(GSList* file_list);
