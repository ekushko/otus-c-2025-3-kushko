cmake_minimum_required(VERSION 3.5)

project(logger LANGUAGES C)

add_library(${PROJECT_NAME} STATIC
            logger.h logger.c)

set(LOGGER_EXE logger_test)

add_executable(${LOGGER_EXE}
               main.c)

target_link_libraries(${LOGGER_EXE} ${PROJECT_NAME})
