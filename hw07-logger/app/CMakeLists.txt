cmake_minimum_required(VERSION 3.5)

project(app LANGUAGES C)

add_executable(${PROJECT_NAME} main.c)

set(LOGGER_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libs/logger/include)
set(LOGGER_LIBRARY ${CMAKE_SOURCE_DIR}/libs/logger/lib/liblogger.a)

target_include_directories(${PROJECT_NAME} PRIVATE ${LOGGER_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE ${LOGGER_LIBRARY})
