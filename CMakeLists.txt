# Root CMake configuration
cmake_minimum_required(VERSION 3.20)
project(nico)
include(CTest)

# Project version
add_compile_definitions(NICO_VERSION="0.0.1")

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

# Sub-projects
add_subdirectory(nico)
add_subdirectory(nico_core)
add_subdirectory(nico_rt)
add_subdirectory(tests/nico_testlib_example)
add_subdirectory(tests/nico_testlib_interop)
add_subdirectory(tests/nico_tests)
