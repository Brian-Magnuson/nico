#ifndef NICO_CORE_CONFIG_H
#define NICO_CORE_CONFIG_H

#include <cstddef>

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
 * @brief The path to the runtime library used by the Nico compiler.
 *
 * This should be set automatically by the build system.
 */
inline constexpr auto nico_rt_lib_path = NICO_RT_LIB_PATH;

/**
 * @brief The current project version.
 *
 * The version is formatted like this: "X.Y.Z" where X is the major version,
 * Y is the minor version, and Z is the patch version.
 *
 * This should be set in the project's build files.
 */
inline constexpr auto nico_version = NICO_VERSION;

} // namespace nico::config

#endif // NICO_CORE_CONFIG_H
