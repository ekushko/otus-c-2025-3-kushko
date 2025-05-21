#include <stdio.h>

#include "logger.h"

Logger* logger = NULL;

void bar() {
    FILE* fp = NULL;
    if (fp == NULL) {
        logger_error(logger, "FP is NULL!!!");
    }
}

void foo() {
    bar();
}

int main() {
    printf("Generating logs...\n");
    logger = logger_init("log.txt");
    logger_debug(logger, "Debug message!");
    logger_info(logger, "Info message!");
    logger_warn(logger, "Warning message!");
    foo();
    logger_free(logger);
    printf("Done!\n");
    return 0;
}
