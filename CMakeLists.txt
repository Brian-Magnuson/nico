# CMake configuration
cmake_minimum_required(VERSION 3.20)
project(nico)
include(FetchContent)
include(CTest)

# Compiler configuration
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall)

# LLVM
find_package(LLVM 18.1.0 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
llvm_map_components_to_libnames(llvm_libs core support target native mc asmparser asmprinter targetparser orcjit)
message(STATUS "LLVM libraries: ${llvm_libs}")

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
set(LEXER_SRC
    src/lexer/lexer.cpp
)
set(PARSER_SRC
    src/parser/parser.cpp
    src/parser/type.cpp
)
set(CHECKER_SRC
    src/checker/global_checker.cpp
    src/checker/local_checker.cpp
    src/checker/symbol_tree.cpp
)
set(CODEGEN_SRC
    src/codegen/code_generator.cpp
)
set(COMPILER_SRC
    src/compiler/emitter.cpp
    src/compiler/jit.cpp
    src/compiler/optimizer.cpp
)
set(LOGGER_SRC
    src/logger/logger.cpp
)
set(CORE_SRC
    ${LEXER_SRC}
    ${PARSER_SRC}
    ${CHECKER_SRC}
    ${CODEGEN_SRC}
    ${COMPILER_SRC}
    ${LOGGER_SRC}
)

# Main file
set(MAIN_SRC src/main.cpp)

# Debug files
set(DEBUG_SRC
    src/debug/ast_printer.cpp
    src/debug/test_utils.cpp
)

# Test files
set(TEST_SRC
    test/lexer_tests.cpp
    test/parser_expr_tests.cpp
    test/parser_stmt_tests.cpp
    test/checker_local_tests.cpp
    test/jit_tests.cpp
)

# Main executable
add_executable(nico ${MAIN_SRC} ${CORE_SRC})
target_link_libraries(nico ${llvm_libs})

# Test executable
add_executable(tests ${TEST_SRC} ${CORE_SRC} ${DEBUG_SRC})
target_link_libraries(tests Catch2::Catch2WithMain ${llvm_libs})
enable_testing()
catch_discover_tests(tests)
