#pragma once

typedef enum {
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR
} LogType_t;

#define logger_debug(logger, message) \
    logger_log(logger, DEBUG, __LINE__, __FUNCTION__, __FILE__, message)

#define logger_info(logger, message) \
    logger_log(logger, INFO, __LINE__, __FUNCTION__, __FILE__, message)

#define logger_warn(logger, message) \
    logger_log(logger, WARNING, __LINE__, __FUNCTION__, __FILE__, message)

#define logger_error(logger, message) \
    logger_log(logger, ERROR, __LINE__, __FUNCTION__, __FILE__, message)

typedef struct Logger Logger;

Logger *logger_init(const char* file_path);

void logger_log(
    Logger *logger,
    const LogType_t log_type,
    const int line,
    const char *func,
    const char *file,
    const char *message
);

void logger_free(Logger *logger);

