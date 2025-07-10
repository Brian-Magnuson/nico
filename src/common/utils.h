#ifndef NICO_UTILS_H
#define NICO_UTILS_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

// Checks if the provided std::shared_ptr points to an instance of the specified type.
#define PTR_INSTANCEOF(ptr, type) \
    (std::dynamic_pointer_cast<type>(ptr) != nullptr)

/**
 * @brief Prints out a message to stderr, then aborts the program.
 *
 * We try to avoid using exceptions in this project, so we instead use explicit
 * values for recoverable errors and functions like this for unrecoverable errors.
 *
 * @param message The message to print out before aborting.
 * @return This function never returns; it calls std::abort().
 */
[[noreturn]] inline void panic(const std::string& message) {
    std::cerr << "Panic: " << message << std::endl;
    std::abort();
}

#endif // NICO_UTILS_H
