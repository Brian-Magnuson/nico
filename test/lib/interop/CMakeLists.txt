cmake_minimum_required(VERSION 3.20)
project(interop)

# Main library
add_library(interop STATIC src/lib.c)
target_include_directories(interop PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
