# Root CMake configuration
cmake_minimum_required(VERSION 3.20)
project(nico)
include(CTest)

# Project version
set(NICO_VERSION "0.0.1")

# Compiler configuration
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall)

# LLVM
find_package(LLVM 20.1.0 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
llvm_map_components_to_libnames(llvm_libs core support target native mc asmparser asmprinter targetparser orcjit)
message(STATUS "LLVM libraries: ${llvm_libs}")

# C++ compiler runtime library
set(NICO_CXX_COMPILER_RT_LIB_PATH "" CACHE FILEPATH
    "Optional path to the C++ compiler runtime library.")

if(NICO_CXX_COMPILER_RT_LIB_PATH)
    if(NOT EXISTS "${NICO_CXX_COMPILER_RT_LIB_PATH}")
        message(FATAL_ERROR
            "NICO_CXX_COMPILER_RT_LIB_PATH does not exist: ${NICO_CXX_COMPILER_RT_LIB_PATH}")
    endif()

    file(TO_CMAKE_PATH "${NICO_CXX_COMPILER_RT_LIB_PATH}" NICO_CXX_COMPILER_RT_LIB_PATH)
endif()

# Project configuration file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/project_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/project_config.h"
    @ONLY
)
set(NICO_CONFIG_INCLUDE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

# Sub-projects
add_subdirectory(nico)
add_subdirectory(nico_core)
add_subdirectory(nico_rt)
add_subdirectory(tests/nico_testlib_example)
add_subdirectory(tests/nico_testlib_interop)
add_subdirectory(tests/nico_tests)
