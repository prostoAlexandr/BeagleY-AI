cmake_minimum_required(VERSION 3.21)
project(cross_hello LANGUAGES C)
add_executable(hello
    src/main.c
)

target_compile_options(hello PRIVATE
    -Wall
    -Wextra
)