#ifndef EXAMPLELIB_LIB_H
#define EXAMPLELIB_LIB_H

/**
 * @brief Returns the 32-bit integer 42.
 *
 * @return The 32-bit integer 42, always.
 */
int examplelib_get_constant();

/**
 * @brief Multiplies the given integer by itself and returns the result.
 *
 * @param x The integer to square.
 * @return The square of the given integer.
 */
int examplelib_square(int x);
/**
 * @brief Adds two integers and returns the result.
 *
 * @param a The first integer to add.
 * @param b The second integer to add.
 * @return The sum of the two integers.
 */
int examplelib_add(int a, int b);

/**
 * @brief Adds a variable number of integers and returns the result.
 * @param count The number of integers to add.
 * @param ... The integers to add.
 * @return The sum of the integers.
 */
int examplelib_sum(int count, ...);

#endif // EXAMPLELIB_LIB_H
