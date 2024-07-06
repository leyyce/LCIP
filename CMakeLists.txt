cmake_minimum_required(VERSION 3.28)
project(LCIP C)

set(CMAKE_C_STANDARD 11)

add_executable(LCIP main.c
        lib/lcip.c
        lib/lcip.h)
