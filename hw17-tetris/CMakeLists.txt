cmake_minimum_required(VERSION 3.5)

project(hw17-tetris LANGUAGES C)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(${PROJECT_NAME} main.c
                               game.h game.c
                               scoreboard.h scoreboard.c)

target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})
