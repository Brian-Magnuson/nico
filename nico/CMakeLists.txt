# nico CMake configuration

add_executable(nico main.cpp)
target_link_libraries(nico PRIVATE nico_core)
