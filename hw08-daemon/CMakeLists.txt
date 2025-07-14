cmake_minimum_required(VERSION 3.5)

project(hw08-daemon LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} PkgConfig::deps)
