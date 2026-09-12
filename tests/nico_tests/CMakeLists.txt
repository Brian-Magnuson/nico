# nico_tests CMake configuration

include(FetchContent)

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

# Test utility files
set(TEST_UTILS_SRC
    src/utils/ast_printer.cpp
    src/utils/test_utils.cpp
)

# Test files
set(TEST_SRC
    ${TEST_UTILS_SRC}
    src/lexer_tests.cpp
    src/parser_expr_tests.cpp
    src/parser_stmt_tests.cpp
    src/type_checker_expr_tests.cpp
    src/type_checker_stmt_tests.cpp
    src/mir_tests.cpp
    src/jit_tests.cpp
    src/utils_tests.cpp
)

# Test executable
add_executable(nico_tests ${TEST_SRC})
target_include_directories(nico_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nico_tests PRIVATE nico_core Catch2::Catch2WithMain)
target_compile_definitions(nico_tests
    PRIVATE NICO_TESTLIB_EXAMPLE_LIB_PATH="$<TARGET_FILE:nico_testlib_example>"
    PRIVATE NICO_TESTLIB_INTEROP_LIB_PATH="$<TARGET_FILE:nico_testlib_interop>"
)
enable_testing()
catch_discover_tests(nico_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
