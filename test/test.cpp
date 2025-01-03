#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../src/compiler/code_file.h"
#include "../src/lexer/lexer.h"
#include "../src/logger/logger.h"

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

TEST_CASE("Sanity check", "[sanity]") {
    REQUIRE(1 == 1);
}

TEST_CASE("Lexer single characters", "[lexer]") {
    Lexer lexer;
    Logger::inst().set_printing_enabled(false);

    auto file = make_test_code_file("(){}[]");
    auto tokens = lexer.scan(file);
    REQUIRE(tokens.size() == 7);

    lexer.reset();
    Logger::inst().reset();
}
