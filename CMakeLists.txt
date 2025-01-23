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

# Core files
set(LEXER_SRC src/lexer/token.cpp src/lexer/lexer.cpp)
set(PARSER_SRC src/parser/parser.cpp)
set(LOGGER_SRC src/logger/logger.cpp)
set(CORE_SRC
    ${LEXER_SRC}
    ${PARSER_SRC}
    ${LOGGER_SRC}
)

# Main file
set(MAIN_SRC src/main.cpp)

# Debug files
set(DEBUG_SRC src/debug/ast_printer.cpp src/debug/test_utils.cpp)

# Test files
set(TEST_SRC test/lexer_tests.cpp test/parser_expr_tests.cpp test/parser_stmt_tests.cpp)

# Main executable
add_executable(nico ${MAIN_SRC} ${CORE_SRC})

# Test executable
add_executable(tests ${TEST_SRC} ${CORE_SRC} ${DEBUG_SRC})
target_link_libraries(tests Catch2::Catch2WithMain ${llvm_libs})
enable_testing()
catch_discover_tests(tests)
