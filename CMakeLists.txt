# CMake configuration
cmake_minimum_required(VERSION 3.20)
project(nico)
include(FetchContent)
include(CTest)

# Project version
add_compile_definitions(NICO_VERSION="0.0.1")

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

# Front end utility files
set(FRONT_END_UTILS_SRC
    src/frontend/utils/symbol_node.cpp
    src/frontend/utils/type_node.cpp
    src/frontend/utils/symbol_tree.cpp
    src/frontend/utils/control_stack.cpp
    src/frontend/utils/expression_checker.cpp
    src/frontend/utils/mir.cpp
)

# Front end component files
set(FRONT_END_COMPONENTS_SRC
    src/frontend/components/code_generator.cpp
    src/frontend/components/global_checker.cpp
    src/frontend/components/lexer.cpp
    src/frontend/components/local_checker.cpp
    src/frontend/components/parser.cpp
)

# Front end files
set(FRONT_END_SRC
    src/frontend/frontend.cpp
    ${FRONT_END_COMPONENTS_SRC}
    ${FRONT_END_UTILS_SRC}
)

# Back end files
set(BACK_END_SRC
    src/backend/emitter.cpp
    src/backend/jit.cpp
    src/backend/optimizer.cpp
)

set(DRIVER_SRC
    src/driver/repl.cpp
    src/driver/jit_runner.cpp
)

# Shared files
set(SHARED_SRC
    src/shared/logger.cpp
    src/shared/utils.cpp
)

# Core source files
set(CORE_SRC
    ${FRONT_END_SRC}
    ${BACK_END_SRC}
    ${SHARED_SRC}
    ${DRIVER_SRC}
)

# Main file
set(MAIN_SRC
    src/main.cpp
)

# Test utility files
set(TEST_UTILS_SRC
    test/utils/ast_printer.cpp
    test/utils/test_utils.cpp
)

# Test files
set(TEST_SRC
    ${TEST_UTILS_SRC}
    test/lexer_tests.cpp
    test/parser_expr_tests.cpp
    test/parser_stmt_tests.cpp
    test/type_checker_tests.cpp
    test/jit_tests.cpp
    test/utils_tests.cpp
)

# Main executable
add_executable(nico ${MAIN_SRC} ${CORE_SRC})
target_include_directories(nico PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(nico ${llvm_libs})

# Test executable
add_executable(tests ${TEST_SRC} ${CORE_SRC})
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/test/include)
target_link_libraries(tests Catch2::Catch2WithMain ${llvm_libs})
enable_testing()
catch_discover_tests(tests)
