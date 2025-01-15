#include "test_utils.h"

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <utility>

std::shared_ptr<CodeFile> make_test_code_file(const char* src_code) {
    auto file = std::make_shared<CodeFile>(
        std::filesystem::current_path() / "test.nico",
        std::string(src_code)
    );
    return file;
}

std::shared_ptr<CodeFile> make_test_code_file(std::string&& src_code) {
    auto file = std::make_shared<CodeFile>(
        std::filesystem::current_path() / "test.nico",
        std::move(src_code)
    );
    return file;
}

std::vector<Tok> extract_token_types(const std::vector<std::shared_ptr<Token>>& tokens) {
    std::vector<Tok> token_types;
    std::transform(tokens.begin(), tokens.end(), std::back_inserter(token_types), [](const auto& token) {
        return token->tok_type;
    });
    return token_types;
}
