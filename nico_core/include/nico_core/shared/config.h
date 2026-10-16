#ifndef NICO_CORE_CONFIG_H
#define NICO_CORE_CONFIG_H

#include <cstddef>
#include <string_view>

namespace nico::config {

/**
 * @brief The maximum recursion depth for functions that possibly recurse
 * infinitely.
 *
 * This value can be adjusted in the config header.
 */
inline constexpr std::size_t max_recursion_depth = 255;

/**
 * @brief The maximum number of implicit dereferences that we allow to be
 * performed on an expression.
 *
 * This value can be adjusted in the config header.
 */
inline constexpr std::size_t max_allowed_implicit_dereferences = 16;

/**
 * @brief The exit code used when the compiled Nico program panics in
 * panic-recoverable mode.
 *
 * Should not be 0.
 *
 * This value can be adjusted in the config header.
 */
inline constexpr int nico_panic_exit_code = 101;

/**
 * @brief The default name of the main function generated in the module.
 *
 * This should usually be "main" since "main" is the function called when
 * running an executable compiled by the AOT compiler.
 *
 * This value can be adjusted in the config header.
 */
inline constexpr std::string_view default_main_fn_name = "main";

/**
 * @brief The default name of the script function generated in the module.
 *
 * Prefer using names that start with a `$` to avoid name collisions with
 * user-defined functions.
 *
 * This value can be adjusted in the config header.
 */
inline constexpr std::string_view default_script_fn_name = "$script";

/**
 * @brief The path to the runtime library used by the Nico compiler.
 *
 * This should be set automatically by the build system.
 */
inline constexpr auto nico_rt_lib_path = NICO_RT_LIB_PATH;

} // namespace nico::config

#endif // NICO_CORE_CONFIG_H
