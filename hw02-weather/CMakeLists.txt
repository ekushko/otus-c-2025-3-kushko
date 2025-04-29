cmake_minimum_required(VERSION 3.5)

project(hw02-weather LANGUAGES C)

add_executable(${PROJECT_NAME} main.c)

find_package(CURL REQUIRED)

include_directories(${CURL_INCLUDE_DIR})

find_package(json-c CONFIG)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    ${CURL_LIBRARIES}
    json-c::json-c
)
