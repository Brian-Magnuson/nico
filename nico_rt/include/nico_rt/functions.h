#ifndef NICO_RT_FUNCTIONS_H
#define NICO_RT_FUNCTIONS_H

#include <csetjmp>
#include <cstddef>

inline std::jmp_buf nico_rt_jmp_buf;

extern "C" {

/**
 * @brief Print a formatted string using C++'s std::cout. This function is
 * similar to printf, but it uses C++ streams instead of C-style output.
 *
 * @param format The format string, similar to printf.
 * @param ... The arguments to be formatted.
 * @return int The number of characters printed, or a negative value if an error
 * occurs.
 */
int nico_rt_printf(const char* format, ...);

/**
 * @brief Print a formatted string using C++'s std::cerr. This function is
 * similar to fprintf(stderr, ...), but it uses C++ streams instead of C-style
 * output.
 *
 * @param format The format string, similar to printf.
 * @param ... The arguments to be formatted.
 * @return int The number of characters printed, or a negative value if an error
 * occurs.
 */
int nico_rt_printerrf(const char* format, ...);

/**
 * @brief Abort the program.
 *
 * This function should have no other side effects and should terminate the
 * program immediately.
 */
void nico_rt_abort();

/**
 * @brief Exit the program with the given exit code.
 *
 * @param exit_code The exit code to use when exiting the program.
 */
void nico_rt_exit(int exit_code);

/**
 * @brief Allocate memory.
 *
 * This function is essentially a wrapper around the standard malloc function,
 * but it is provided for use in the Nico runtime environment. It allocates a
 * block of memory of the specified size
 *
 * @param size The number of bytes to allocate.
 * @return void* A pointer to the allocated memory, or nullptr if allocation
 * fails.
 */
void* nico_rt_malloc(size_t size);

/**
 * @brief Free allocated memory.
 *
 * This function is essentially a wrapper around the standard free function, but
 * it is provided for use in the Nico runtime environment. It frees a block of
 * memory that was previously allocated with nico_rt_malloc.
 *
 * @param ptr A pointer to the memory block to free. If ptr is nullptr, no
 * operation is performed.
 */
void nico_rt_free(void* ptr);

/**
 * @brief Set a jump point for a non-local goto.
 *
 * This function saves the current execution context in a jmp_buf, allowing for
 * a non-local jump to be made later using nico_rt_longjmp.
 *
 * This function is primarily used for testing purposes to simulate a
 * sub-process crash without actually calling std::abort() or std::exit(). It
 * allows the program to recover from a simulated crash and continue execution.
 */
void nico_rt_setjmp();

/**
 * @brief Perform a non-local jump to the point saved by nico_rt_setjmp.
 *
 * This function restores the execution context saved by nico_rt_setjmp and
 * transfers control to the point where nico_rt_setjmp was called.
 *
 * This function is primarily used for testing purposes to simulate a
 * sub-process crash without actually calling std::abort() or std::exit(). It
 * allows the program to recover from a simulated crash and continue execution.
 */
void nico_rt_longjmp();
}

#endif // NICO_RT_FUNCTIONS_H
