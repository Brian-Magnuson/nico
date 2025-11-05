#ifndef NICO_UTILS_H
#define NICO_UTILS_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#elif defined(__unix__) || defined(__unix) ||                                  \
    (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif

namespace nico {

// Checks if the provided std::shared_ptr points to an instance of the specified
// type.
#define PTR_INSTANCEOF(ptr, type)                                              \
    (std::dynamic_pointer_cast<type>(ptr) != nullptr)

/**
 * @brief Prints out a message to stderr, then aborts the program.
 *
 * We try to avoid using exceptions in this project, so we instead use explicit
 * values for recoverable errors and functions like this for unrecoverable
 * errors.
 *
 * @param message The message to print out before aborting.
 * @return This function never returns; it calls std::abort().
 */
[[noreturn]] inline void panic(std::string_view message) {
    std::cerr << "Panic: " << message << std::endl;
    std::abort();
}

/**
 * @brief Checks if the standard output is a terminal.
 *
 * This function uses `isatty` on Unix-like systems and `_isatty` on Windows.
 * If neither function is available, this function always returns false.
 *
 * Useful for determining if colored output should be used.
 *
 * @return True if the standard output is known to be a terminal, false
 * otherwise.
 */
inline bool is_stdout_terminal() {
#if defined(_WIN32) || defined(_WIN64)
    return _isatty(_fileno(stdout));
#elif defined(__unix__) || defined(__unix) ||                                  \
    (defined(__APPLE__) && defined(__MACH__))
    return isatty(fileno(stdout));
#else
    return false;
#endif
}

/**
 * @brief Returns the current project version.
 *
 * The version is formatted like this: "X.Y.Z" where X is the major version,
 * Y is the minor version, and Z is the patch version.
 *
 * The project version is defined by the NICO_VERSION macro, which should be set
 * by the build system. If this macro is not defined, the function returns
 * "<unknown version>".
 *
 * @return The project version string.
 */
inline std::string project_version() {
#ifdef NICO_VERSION
    return NICO_VERSION;
#else
    return "<unknown version>";
#endif
}

/**
 * @brief Breaks a string view into multiple string views based on a maximum
 * length.
 *
 * This function is useful for breaking long error messages into multiple
 * lines.
 * The message will be split at whitespace characters whenever possible.
 * If a single word exceeds the maximum length, the word will be broken.
 *
 * For safety, the minimum value for `max_length` is 10. Values less than 10
 * are set to 10.
 *
 * @param message The message to break.
 * @param max_length The max length of each line. Default is 72.
 * @return A vector of string views representing the broken lines.
 */
std::vector<std::string_view>
break_message(std::string_view message, size_t max_length = 72);

} // namespace nico

#endif // NICO_UTILS_H
