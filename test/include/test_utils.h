#ifndef NICO_TEST_UTILS_H
#define NICO_TEST_UTILS_H

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "shared/code_file.h"
#include "shared/token.h"

/**
 * @brief Creates a test code file with the provided source code.
 *
 * The test code file path is set to CWD with the name "test.nico".
 *
 * @param src_code The source code for the code file.
 * @return A shared pointer to the new code file.
 */
std::shared_ptr<CodeFile> make_test_code_file(std::string_view src_code);

/**
 * @brief Creates a vector of token types from a vector of tokens.
 *
 * The original vector is not modified.
 *
 * @param tokens The vector of tokens to extract token types from.
 * @return A vector of token types.
 */
std::vector<Tok>
extract_token_types(const std::vector<std::shared_ptr<Token>>& tokens);

/**
 * @brief Captures output from C functions that normally print to stdout and
 * stderr.
 *
 * This function uses platform-specific APIs to redirect stdout and stderr to
 * pipes, allowing it to capture the output of the specified function. If
 * unistd.h (POSIX) and io.h (Windows) are not available, `func` will still be
 * called, but no output will be captured and an empty string will be returned.
 *
 * This function does not capture output from C++'s print mechanisms like
 * `std::cout`.
 *
 * If `func` throws a C++ exception, the pipe will be closed and the exception
 * will be rethrown.
 *
 * @param func The function from which to execute and capture output. May be a
 * lambda.
 * @param buffer_size The size of the buffer to use when reading from the pipe.
 * Default is 4096.
 *
 * @return std::pair<std::string, std::string> A pair of strings containing the
 * captured output from stdout and stderr.
 *
 * @warning This function is not thread-safe and should not be called from
 * multiple threads simultaneously.
 *
 */
std::pair<std::string, std::string>
capture_stdout(std::function<void()> func, int buffer_size = 4096);

#endif // NICO_TEST_UTILS_H
