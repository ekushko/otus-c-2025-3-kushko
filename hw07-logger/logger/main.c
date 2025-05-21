#include <stdio.h>

#include "logger.h"

int main() {
    printf("Generating logs...\n");
    Logger* logger = logger_init("log.txt");
    logger_debug(logger, "Debug message!");
    logger_info(logger, "Info message!");
    logger_warn(logger, "Warning message!");
    logger_error(logger, "Error message!");
    logger_free(logger);
    printf("Done!\n");
    return 0;
}
