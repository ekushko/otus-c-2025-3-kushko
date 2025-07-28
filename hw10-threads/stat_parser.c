#include "stat_parser.h"

#include <stdio.h>

#define OVECCOUNT 30

#define REQUEST 4
#define SIZE 6
#define REFERER 7

const char *combined_fields[] = {
    "ip", "ident", "user", "time", "request", "status", "size", "referer", "agent"
};

TotalSize *ts_init() {
    TotalSize* total_size = (TotalSize*)malloc(sizeof(TotalSize));
    pthread_mutex_init(&total_size->mutex, NULL);
    total_size->total = 0;
    return total_size;
}

void ts_free(TotalSize *total_size) {
    total_size->total = 0;
    pthread_mutex_destroy(&total_size->mutex);
    free(total_size);
}

void ts_add(TotalSize *total_size, size_t size) {
    pthread_mutex_lock(&total_size->mutex);
    total_size->total += size;
    pthread_mutex_unlock(&total_size->mutex);
}

char* alloc_value_from_pcre(const char *logline, pcre *re, int *ovector, int field) {
    char *value;
    int index = pcre_get_stringnumber(re, combined_fields[field]);
    if (index >= 0) {
        const char *substring_start = logline + ovector[2 * index];
        const int substring_length = ovector[2 * index + 1] - ovector[2 * index];

        value = malloc(substring_length + 1);
        memcpy(value, substring_start, substring_length);
        value[substring_length] = '\0';
    } else {
        value = NULL;
    }
    return value;
}

void parse_combined_logline(TopNContainer* url_to_sz, TopNContainer* ref_to_ct, TotalSize *total_sz, const char* logline) {
    pcre *re;
    const char *error;
    int erroffset;
    int ovector[OVECCOUNT];
    int rc;

    const char *pattern =
        "^(?P<ip>\\S+) (?P<ident>\\S+) (?P<user>\\S+) \\[(?P<time>[^\\]]+)\\] "
        "\"(?P<request>[^\"]*)\" (?P<status>\\d+) (?P<size>\\d+) \"(?P<referer>[^\"]*)\" \"(?P<agent>[^\"]*)\"";

    re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
    if (re == NULL) {
        fprintf(stderr, "PCRE compilation failed at offset %d: %s\n", erroffset, error);
        return;
    }

    rc = pcre_exec(re, NULL, logline, strlen(logline), 0, 0, ovector, OVECCOUNT);

    if (rc < 0) {
    #ifdef SHOULD_SHOW_DEBUG
            switch(rc) {
                case PCRE_ERROR_NOMATCH:
                    printf("No match found: %s\n", logline);
                    break;
                default:
                    printf("Matching error %d\n", rc);
                    break;
            }
    #endif
            pcre_free(re);
            return;
        }

        char *request = alloc_value_from_pcre(logline, re, ovector, REQUEST);
        char *size = alloc_value_from_pcre(logline, re, ovector, SIZE);
        if (request != NULL && size != NULL) {
            char *url = NULL;
            if (strstr(request, "HTTP/")) {
                char method[16], path[256], protocol[16];
                if (sscanf(request, "%15s %255s %15s", method, path, protocol) == 3) {
                    const size_t len = strlen(path);
                    url = malloc(len + 1);
                    url[len] = '\0';
                    strcpy(url, path);
                } else {
    #ifdef SHOULD_SHOW_DEBUG
                    printf("Invalid request format: %s\n", request);
    #endif
                }
            } else {
                const size_t len = strlen(request);
                url = malloc(len + 1);
                url[len] = '\0';
                strcpy(url, request);
            }

            if (url != NULL) {
                topn_cont_add(url_to_sz, url, atoi(size));
                free(url);
            }

            ts_add(total_sz, atoi(size));

            free(size);
            free(request);
        } else {
    #ifdef SHOULD_SHOW_DEBUG
            if (request == NULL) printf("URL not parsed\n");
            if (size == NULL) printf("Size not parsed\n");
    #endif
        }

        char *referer = alloc_value_from_pcre(logline, re, ovector, REFERER);
        if (referer != NULL) {
            topn_cont_add(ref_to_ct, referer, 1);
            free(referer);
        } else {
    #ifdef SHOULD_SHOW_DEBUG
            printf("Referer not parsed\n");
    #endif
        }

        pcre_free(re);
}
