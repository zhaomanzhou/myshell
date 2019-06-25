cmake_minimum_required(VERSION 3.13)
project(myshell C)

set(CMAKE_C_STANDARD 99)

add_executable(myshell src/shell.c src/helpers.c include/helpers.h
        include/linkedList.h include/shell_util.h  src/shell_util.c)