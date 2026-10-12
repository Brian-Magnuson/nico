#ifndef NICO_TESTS_CONFIG_H
#define NICO_TESTS_CONFIG_H

#include <string_view>

namespace nico::test_config {

inline constexpr std::string_view nico_testlib_example_lib_path =
    NICO_TESTLIB_EXAMPLE_LIB_PATH;

inline constexpr std::string_view nico_testlib_interop_lib_path =
    NICO_TESTLIB_INTEROP_LIB_PATH;

} // namespace nico::test_config

#endif // NICO_TESTS_CONFIG_H
