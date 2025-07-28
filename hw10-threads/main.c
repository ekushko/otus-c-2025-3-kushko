#define _GNU_SOURCE

#define SHOULD_SHOW_DEBUG 1

#include "topn_container.h"
#include "stat_parser.h"
#include "file_list.h"

#include <stdio.h>

#define TOP 10

typedef struct {
    int thread_id;
    GSList *file_list;
} ThreadData;

TopNContainer*  gl_url_to_sz;
TopNContainer*  gl_ref_to_ct;
TotalSize*      gl_total_size;

void* get_stat(void* arg) {
    ThreadData *data = (ThreadData*)arg;

#ifdef SHOULD_SHOW_DEBUG
    printf("Thread %d started. Files:\n", data->thread_id);
    print_file_list(data->file_list);
#endif

    char *logline = NULL;
    size_t len = 0;
    ssize_t read;

    GSList *iter;
    for (iter = data->file_list; iter != NULL; iter = iter->next) {
        TopNContainer *url_to_sz = NULL, *ref_to_ct = NULL;
        FILE *fd = fopen((char*)iter->data, "r");
        if (fd != NULL) {
#ifdef SHOULD_SHOW_DEBUG
            printf("File %s opened!\n", (char*)iter->data);
#endif
            url_to_sz = topn_cont_init(TOP * 100);
            ref_to_ct = topn_cont_init(TOP * 100);
            while ((read = getline(&logline, &len, fd)) != -1) {
                parse_combined_logline(url_to_sz, ref_to_ct, gl_total_size, logline);
            }
            fclose(fd);
#ifdef SHOULD_SHOW_DEBUG
            printf("File %s closed!\n", (char*)iter->data);
#endif

            topn_cont_merge(gl_url_to_sz, url_to_sz);
            topn_cont_free(url_to_sz);

            topn_cont_merge(gl_ref_to_ct, ref_to_ct);
            topn_cont_free(ref_to_ct);
        } else {
            fprintf(stderr, "File %s not opened!\n", (char*)iter->data);
        }
    }

#ifdef SHOULD_SHOW_DEBUG
    printf("Thread %d finished.\n", data->thread_id);
#endif

    free_file_list(data->file_list);

    int *result = malloc(sizeof(int));
    *result = 0;
    return (void*)result;
}

int main(int argc, char *argv[]) {
    size_t thread_count = 7;
    char log_path[256];

    if (argc >= 3) {
        strcpy(log_path, argv[2]);
    } else {
        fprintf(stderr, "Log path not specified!\n");
        strcpy(log_path, "/home/user/Temp/logs/");
    }

    GSList *file_list = get_file_list_by_mask(log_path, "*.log.*");
    if (!file_list) {
        fprintf(stderr, "Log files not found!\n");
        exit(EXIT_FAILURE);
    }

    if (argc >= 2) {
        thread_count = atoi(argv[1]);
    } else {
        fprintf(stderr, "Thread count not specified!\n");
    }

    gl_url_to_sz = topn_cont_init(TOP * 100);
    gl_ref_to_ct = topn_cont_init(TOP * 100);
    gl_total_size = ts_init();


    size_t count = g_slist_length(file_list);
    if (thread_count > count) {
        thread_count = count;
    }

    pthread_t threads[thread_count];
    ThreadData thread_data[thread_count];

    size_t j = 0, file_per_thread = count / thread_count;
    for (size_t i = 0; i < thread_count; ++i) {
        int thread_file_list_pos = (i * file_per_thread);
        GSList *thread_file_list = NULL;
        for (j = 0; j < file_per_thread; ++j) {
            const int file_index = thread_file_list_pos + j;
            thread_file_list = g_slist_append(thread_file_list, g_slist_nth_data(file_list, file_index));
        }
        if (file_per_thread * thread_count < count && i == thread_count - 1) {
            for (j = file_per_thread * thread_count; j < count; ++j) {
                printf("pos: %d j: %lu\n", thread_file_list_pos, j);
                thread_file_list = g_slist_append(thread_file_list, g_slist_nth_data(file_list, j));
            }
        }

        thread_data[i].thread_id = i + 1;
        thread_data[i].file_list = thread_file_list;

        int rc = pthread_create(&threads[i], NULL, get_stat, &thread_data[i]);
        if (rc) {
            fprintf(stderr, "Error creating thread %lu\n", i);
            exit(EXIT_FAILURE);
        }
    }

    for (unsigned int i = 0; i < thread_count; i++) {
        void *retval;
        pthread_join(threads[i], &retval);
    }

    printf("\n\n");
    printf("-------------------\n");
    printf("Top 10 URL by size:\n");
    printf("-------------------\n");
    topn_cont_print(gl_url_to_sz, TOP);

    printf("\n\n");
    printf("-------------------\n");
    printf("Top 10 Referer by count:\n");
    printf("-------------------\n");
    topn_cont_print(gl_ref_to_ct, TOP);

    printf("\n\n");
    printf("-------------------\n");
    printf("Total: %lu bytes\n", gl_total_size->total);
    printf("-------------------\n");

    ts_free(gl_total_size);
    topn_cont_free(gl_ref_to_ct);
    topn_cont_free(gl_url_to_sz);

    return 0;
}
