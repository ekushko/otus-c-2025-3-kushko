cmake_minimum_required(VERSION 3.5)

project(dpdk-analyzer LANGUAGES C)

find_package(PkgConfig REQUIRED)

pkg_check_modules(dpdk REQUIRED IMPORTED_TARGET libdpdk)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::dpdk)

set(LOGGER_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libs/logger/include)
set(LOGGER_LIBRARY ${CMAKE_SOURCE_DIR}/libs/logger/lib/liblogger.a)

target_include_directories(${PROJECT_NAME} PRIVATE ${LOGGER_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE ${LOGGER_LIBRARY})
