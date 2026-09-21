# nico_testlib_example CMake configuration

add_library(nico_testlib_example STATIC src/lib.cpp)
target_include_directories(nico_testlib_example PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
