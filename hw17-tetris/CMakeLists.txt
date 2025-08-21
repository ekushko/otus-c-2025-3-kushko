cmake_minimum_required(VERSION 3.5)

project(hw17-tetris LANGUAGES C)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(${PROJECT_NAME} main.c
                               game.h game.c
                               scoreboard.h scoreboard.c)

target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})

# Указываем исходный файл и destination
set(SCRIPT_NAME "run.sh")
set(SCRIPT_SOURCE "${CMAKE_SOURCE_DIR}/${SCRIPT_NAME}")
set(SCRIPT_DESTINATION "${CMAKE_BINARY_DIR}/${SCRIPT_NAME}")

# Копируем файл и устанавливаем права
add_custom_command(
    OUTPUT ${SCRIPT_DESTINATION}
    COMMAND ${CMAKE_COMMAND} -E copy ${SCRIPT_SOURCE} ${SCRIPT_DESTINATION}
    COMMAND chmod +x ${SCRIPT_DESTINATION}
    DEPENDS ${SCRIPT_SOURCE}
    COMMENT "Copying and making executable: ${SCRIPT_NAME}"
)

# Добавляем цель для копирования
add_custom_target(
    copy_script ALL
    DEPENDS ${SCRIPT_DESTINATION}
)
