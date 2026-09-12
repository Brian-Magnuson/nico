# nico_testlib_interop CMake configuration

add_library(nico_testlib_interop STATIC src/lib.c)
target_include_directories(nico_testlib_interop PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
