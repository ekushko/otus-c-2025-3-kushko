cmake_minimum_required(VERSION 3.5)

project(hw16-cube LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(GL REQUIRED gl)
pkg_check_modules(GLU REQUIRED glu)

add_executable(${PROJECT_NAME} main.c)

target_include_directories(${PROJECT_NAME} PUBLIC  ${SDL2_INCLUDE_DIRS}  ${GL_INCLUDE_DIRS} ${GLU_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${SDL2_LIBRARIES} ${GLU_LIBRARIES} ${GL_LIBRARIES})
