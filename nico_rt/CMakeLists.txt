add_library(nico_rt STATIC src/functions.c)
target_include_directories(nico_rt PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
