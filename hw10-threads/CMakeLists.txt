cmake_minimum_required(VERSION 3.5)

project(hw10-threads LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)
pkg_search_module(PCRE REQUIRED libpcre)

add_executable(${PROJECT_NAME}
    main.c
    topn_container.c topn_container.h
    stat_parser.c stat_parser.h
    file_list.c file_list.h
)

target_include_directories(${PROJECT_NAME} PRIVATE ${PCRE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PCRE_LIBRARIES} PkgConfig::deps)
