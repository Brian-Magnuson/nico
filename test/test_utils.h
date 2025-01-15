#ifndef NICO_TEST_UTILS_H
#define NICO_TEST_UTILS_H

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../src/compiler/code_file.h"
#include "../src/lexer/token.h"

/**
 * @brief Creates a test code file with the provided source code.
 *
 * The test code file path is set to CWD with the name "test.nico".
 *
 * @param src_code The source code for the code file.
 * @return A shared pointer to the new code file.
 */
std::shared_ptr<CodeFile> make_test_code_file(const char* src_code) {
    auto file = std::make_shared<CodeFile>(
        std::filesystem::current_path() / "test.nico",
        std::string(src_code)
    );
    return file;
}

/**
 * @brief Creates a test code file with the provided source code.
 *
 * The test code file path is set to CWD with the name "test.nico".
 *
 * @param src_code The source code for the code file. Will be moved into the code file.
 * @return A shared pointer to the new code file.
 */
std::shared_ptr<CodeFile> make_test_code_file(std::string&& src_code) {
    auto file = std::make_shared<CodeFile>(
        std::filesystem::current_path() / "test.nico",
        std::move(src_code)
    );
    return file;
}

/**
 * @brief Creates a vector of token types from a vector of tokens.
 *
 * The original vector is not modified.
 *
 * @param tokens The vector of tokens to extract token types from.
 * @return A vector of token types.
 */
std::vector<Tok> extract_token_types(const std::vector<std::shared_ptr<Token>>& tokens) {
    std::vector<Tok> token_types;
    std::transform(tokens.begin(), tokens.end(), std::back_inserter(token_types), [](const auto& token) {
        return token->tok_type;
    });
    return token_types;
}

#endif // NICO_TEST_UTILS_H
