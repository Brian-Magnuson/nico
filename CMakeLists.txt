# CMake configuration
cmake_minimum_required(VERSION 3.19)
project(nico)
include(FetchContent)
include(CTest)

# Compiler configuration
set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall)

# Catch2
find_package(Catch2 3.4.0 QUIET)

if(NOT Catch2_FOUND)
    message(STATUS "Fetching Catch2 from GitHub.")
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
else()
    message(STATUS "Found Catch2 ${Catch2_VERSION}")
endif()

include(Catch)

# Source files
set(LEXER_SRC src/lexer/token.cpp src/lexer/lexer.cpp)
set(LOGGER_SRC src/logger/logger.cpp)
set(SOURCES
    ${LEXER_SRC}
    ${LOGGER_SRC}
)

set(MAIN_SRC src/main.cpp)

# Test files
set(TEST_SRC test/test.cpp)

# Main executable
add_executable(nico ${MAIN_SRC} ${SOURCES})

# Test executable
add_executable(tests ${TEST_SRC} ${SOURCES})
target_link_libraries(tests Catch2::Catch2WithMain ${llvm_libs})
enable_testing()
catch_discover_tests(tests)
