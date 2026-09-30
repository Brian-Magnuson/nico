#ifndef NICO_TESTS_TEST_UTILS_H
#define NICO_TESTS_TEST_UTILS_H

#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "nico_core/shared/code_file.h"
#include "nico_core/shared/token.h"

namespace nico {

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
 * @deprecated We are now using C++ streams instead of C functions for printing,
 * so this function is no longer needed. Use `capture_streams` instead.
 */
std::pair<std::string, std::string>
capture_stdout(std::function<void()> func, int buffer_size = 4096);

/**
 * @brief A utility class to capture output from `std::cout` and `std::cerr`.
 *
 * Use the static `capture` method to execute a function and capture its output.
 * The captured output can be accessed through the returned `Result` struct.
 *
 * This class is used to provide an RAII-style mechanism for capturing output,
 * ensuring that the original stream buffers are restored even if an exception
 * is thrown during the execution of the function.
 */
class StreamCapture {
    // The string stream to capture output from `std::cout`.
    std::ostringstream cout_capture;
    // The string stream to capture output from `std::cerr`.
    std::ostringstream cerr_capture;
    // The previous stream buffer for `std::cout` before redirection.
    std::streambuf* prev_cout_buf;
    // The previous stream buffer for `std::cerr` before redirection.
    std::streambuf* prev_cerr_buf;

    StreamCapture()
        : prev_cout_buf(std::cout.rdbuf(cout_capture.rdbuf())),
          prev_cerr_buf(std::cerr.rdbuf(cerr_capture.rdbuf())) {}

    ~StreamCapture() {
        std::cout.rdbuf(prev_cout_buf);
        std::cerr.rdbuf(prev_cerr_buf);
    }

    StreamCapture(const StreamCapture&) = delete;
    StreamCapture& operator=(const StreamCapture&) = delete;

public:
    /**
     * @brief The result of a stream capture operation, containing the captured
     * output from `std::cout` and `std::cerr`, and a boolean indicating if an
     * exception was thrown during the execution of the function.
     */
    struct Result {
        // The captured output from `std::cout`.
        std::string cout_output;
        // The captured output from `std::cerr`.
        std::string cerr_output;
        // Whether an exception was thrown during the execution of `func`.
        bool was_exception_thrown;
    };

    /**
     * @brief Captures output from `std::cout` and `std::cerr` during the
     * execution of a function.
     *
     * The result of the capture is returned in a `Result` struct, which
     * contains the captured outputs from `std::cout` and `std::cerr`. If an
     * exception is thrown during the execution of `func`, the original stream
     * buffers are restored and `was_exception_thrown` is set to true in the
     * returned `Result`.
     *
     * @param func The function from which to execute and capture output. May be
     * a lambda.
     * @return Result The result of the stream capture operation (see
     * description).
     */
    static Result capture(std::function<void()> func) {
        StreamCapture capture;
        bool was_exception_thrown = false;

        try {
            func();
        }
        catch (...) {
            was_exception_thrown = true;
        }

        return Result{
            capture.cout_capture.str(),
            capture.cerr_capture.str(),
            was_exception_thrown
        };
    }
};

} // namespace nico

#endif // NICO_TESTS_TEST_UTILS_H
