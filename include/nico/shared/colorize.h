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

inline std::ostream& red(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[31m";
}

inline std::ostream& green(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[32m";
}

inline std::ostream& yellow(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[33m";
}

inline std::ostream& blue(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[34m";
}

inline std::ostream& magenta(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[35m";
}

inline std::ostream& cyan(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[36m";
}

inline std::ostream& white(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[37m";
}

inline std::ostream& gray(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[90m";
}

inline std::ostream& reset(std::ostream& o) {
    if (!is_colorable(o)) {
        return o;
    }
    return o << "\033[0m";
}

} // namespace colorize

#endif // NICO_COLORIZE_H
