cmake_minimum_required(VERSION 3.12)
project(water C)

set(CMAKE_C_STANDARD 99)

add_executable(water main.c)

target_link_libraries(water glut GLU GL X11 pthread Xrandr Xi dl m)
