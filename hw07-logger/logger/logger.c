#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <execinfo.h>

struct Logger {
    FILE* fp;
    char* file_path;
    pthread_mutex_t mutex;
};

void logger_log_trace(Logger *logger) {
    if (logger->fp == NULL) {
        return;
    }

    void *array[10];
    char **strings;
    int size, i;

    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);
    if (strings != NULL) {
        for (i = 0; i < size; i++)
            fprintf(logger->fp, "%s\n", strings[i]);
    }

    free(strings);
}

Logger *logger_init(const char *file_path) {
    Logger* logger = (Logger*)malloc(sizeof(Logger));
    const size_t file_path_len = strlen(file_path) + 1;
    logger->file_path = (char*)malloc(file_path_len);
    memcpy(logger->file_path, file_path, file_path_len);
    logger->fp = fopen(file_path, "a");
    if (logger->fp == NULL) {
        perror(file_path);
    }
    pthread_mutex_init(&logger->mutex, NULL);
    return logger;
}

void logger_log(Logger* logger,
                const LogType_t log_type,
                const int line,
                const char *func,
                const char *file,
                const char *message) {
    if (logger == NULL) {
        perror("Logger is NULL");
        return;
    }

    if (logger->fp == NULL) {
        perror(logger->file_path);
        return;
    }

    const char* prefix;

    switch(log_type) {
        case DEBUG  : prefix = "[DEBUG]"; break;
        case INFO   : prefix = "[INFO ]"; break;
        case WARNING: prefix = "[WARN ]"; break;
        case ERROR  : prefix = "[ERROR]"; break;
        default     : prefix = "[LOG  ]"; break;
    }

    pthread_mutex_lock(&logger->mutex);

    fprintf(logger->fp, "%s\t%s:%d\tfunction \'%s\'\t\t%s\n", prefix, file, line, func, message);

    if (log_type == ERROR)
        logger_log_trace(logger);

    pthread_mutex_unlock(&logger->mutex);
}

void logger_free(Logger* logger) {
    if (logger->fp != NULL) {
        fclose(logger->fp);
    }
    pthread_mutex_destroy(&logger->mutex);
    free(logger);
    logger = NULL;
}
