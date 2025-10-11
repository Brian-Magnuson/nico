#ifndef NICO_COLORIZE_H
#define NICO_COLORIZE_H

#include <iostream>

#include "nico/shared/utils.h"

namespace colorize {

/**
 * @brief Check if the output stream supports color.
 * @param o The output stream to check.
 * @return True if the stream supports color, false otherwise.
 */
inline bool is_colorable(std::ostream& o) {
    return is_stdout_terminal() && (&o == &std::cout || &o == &std::cerr);
}

/**
 * @brief Stream manipulator to set text color to red.
 *
 * If the stream does not support color, it returns the stream unchanged.
 *
 * @param o The output stream to modify.
 * @return The modified output stream.
 */
inline std::ostream& red(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[31m";
}

/**
 * @brief Stream manipulator to set text color to green.
 *
 * If the stream does not support color, it returns the stream unchanged.
 *
 * @param o The output stream to modify.
 * @return The modified output stream.
 */
inline std::ostream& green(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[32m";
}

/**
 * @brief Stream manipulator to set text color to yellow.
 *
 * If the stream does not support color, it returns the stream unchanged.
 *
 * @param o The output stream to modify.
 * @return The modified output stream.
 */
inline std::ostream& yellow(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[33m";
}

/**
 * @brief Stream manipulator to set text color to blue.
 *
 * If the stream does not support color, it returns the stream unchanged.
 *
 * @param o The output stream to modify.
 * @return The modified output stream.
 */
inline std::ostream& blue(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[34m";
}

/**
 * @brief Stream manipulator to set text color to magenta.
 *
 * If the stream does not support color, it returns the stream unchanged.
 *
 * @param o The output stream to modify.
 * @return The modified output stream.
 */
inline std::ostream& magenta(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[35m";
}

/**
 * @brief Stream manipulator to set text color to cyan.
 *
 * If the stream does not support color, it returns the stream unchanged.
 *
 * @param o The output stream to modify.
 * @return The modified output stream.
 */
inline std::ostream& cyan(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[36m";
}

/**
 * @brief Stream manipulator to set text color to white.
 *
 * If the stream does not support color, it returns the stream unchanged.
 *
 * @param o The output stream to modify.
 * @return The modified output stream.
 */
inline std::ostream& white(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[37m";
}

/**
 * @brief Stream manipulator to set text color to gray.
 *
 * If the stream does not support color, it returns the stream unchanged.
 *
 * Note: This function actually uses the ANSI escape code for "bright black".
 *
 * @param o The output stream to modify.
 * @return The modified output stream.
 */
inline std::ostream& gray(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[90m";
}

/**
 * @brief Stream manipulator to reset text formatting to default.
 *
 * If the stream does not support color, it returns the stream unchanged.
 *
 * @param o The output stream to modify.
 * @return The modified output stream.
 */
inline std::ostream& reset(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[0m";
}

} // namespace colorize

#endif // NICO_COLORIZE_H
