cmake_minimum_required(VERSION 3.5)

project(hw15-database LANGUAGES C)

find_package(SQLite3 REQUIRED)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} PRIVATE SQLite::SQLite3)
