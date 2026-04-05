cmake_minimum_required(VERSION 3.20)
project(example)

# Main library
add_library(example STATIC src/lib.c)
target_include_directories(example PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
