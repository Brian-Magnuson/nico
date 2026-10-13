#ifndef NICO_CORE_COMPILE_MODE_H
#define NICO_CORE_COMPILE_MODE_H

namespace nico {

/**
 * @brief Compile modes for the Nico compiler.
 *
 * Primarily used by the frontend to determine how to compile code and what
 * features to enable.
 */
enum class CompileMode {
    // Unspecified compile mode. This value is used when a complete frontend is
    // not required (e.g. for testing purposes).
    None,
    // Ahead-of-Time compilation mode.
    AOT,
    // Just-In-Time compilation mode.
    JIT,
    // Read-Eval-Print Loop mode.
    REPL
};

namespace compile_modes {

/**
 * @brief Indicates if the given compile mode uses JIT compilation.
 *
 * This is the case when the compile mode is either JIT or REPL, and false when
 * the compile mode is AOT.
 *
 * @param mode The compile mode to check.
 * @return bool True if the compile mode uses JIT compilation, false otherwise.
 */
inline bool uses_jit(CompileMode mode) {
    return mode == CompileMode::JIT || mode == CompileMode::REPL;
}

} // namespace compile_modes

} // namespace nico

#endif // NICO_CORE_COMPILE_MODE_H
