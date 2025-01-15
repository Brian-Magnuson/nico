#ifndef NICO_TEST_UTILS_H
#define NICO_TEST_UTILS_H

#include <memory>
#include <string>
#include <vector>

#include "../compiler/code_file.h"
#include "../lexer/token.h"

/**
 * @brief Creates a test code file with the provided source code.
 *
 * The test code file path is set to CWD with the name "test.nico".
 *
 * @param src_code The source code for the code file.
 * @return A shared pointer to the new code file.
 */
std::shared_ptr<CodeFile> make_test_code_file(const char* src_code);

/**
 * @brief Creates a test code file with the provided source code.
 *
 * The test code file path is set to CWD with the name "test.nico".
 *
 * @param src_code The source code for the code file. Will be moved into the code file.
 * @return A shared pointer to the new code file.
 */
std::shared_ptr<CodeFile> make_test_code_file(std::string&& src_code);

/**
 * @brief Creates a vector of token types from a vector of tokens.
 *
 * The original vector is not modified.
 *
 * @param tokens The vector of tokens to extract token types from.
 * @return A vector of token types.
 */
std::vector<Tok> extract_token_types(const std::vector<std::shared_ptr<Token>>& tokens);

#endif // NICO_TEST_UTILS_H
