cmake_minimum_required(VERSION 3.20)
project(examplelib)

# Main library
add_library(examplelib STATIC src/lib.c)
target_include_directories(examplelib PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
