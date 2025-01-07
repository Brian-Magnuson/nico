#include <catch2/catch_test_macros.hpp>

#include <algorithm>
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

    SECTION("Grouping characters 1") {
        auto file = make_test_code_file("()");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::LParen,
            Tok::RParen,
            Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
    }

    SECTION("Grouping characters 2") {
        auto file = make_test_code_file("()[]{}");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::LParen,
            Tok::RParen,
            Tok::LSquare,
            Tok::RSquare,
            Tok::LBrace,
            Tok::RBrace,
            Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
    }

    SECTION("Other single character tokens") {
        auto file = make_test_code_file(",;");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Comma,
            Tok::Semicolon,
            Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
    }

    lexer.reset();
    Logger::inst().reset();
}

TEST_CASE("Lexer short tokens", "[lexer]") {
    Lexer lexer;

    SECTION("Arithmetic operators") {
        auto file = make_test_code_file("/+-*%");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Slash,
            Tok::Plus,
            Tok::Minus,
            Tok::Star,
            Tok::Percent,
            Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
    }

    SECTION("Assignment operators") {
        auto file = make_test_code_file("+=-=*=/=%=&=|=^=");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::PlusEq,
            Tok::MinusEq,
            Tok::StarEq,
            Tok::SlashEq,
            Tok::PercentEq,
            Tok::AmpEq,
            Tok::BarEq,
            Tok::CaretEq,
            Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
    }

    SECTION("Comparison operators") {
        auto file = make_test_code_file("==!=>=<=><");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::EqEq,
            Tok::BangEq,
            Tok::GtEq,
            Tok::LtEq,
            Tok::Gt,
            Tok::Lt,
            Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
    }

    lexer.reset();
    Logger::inst().reset();
}

TEST_CASE("Lexer simple indents", "[lexer]") {
    Lexer lexer;

    SECTION("Indents 1") {
        auto file = make_test_code_file(
            R"(
a:
  b
)"
        );
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Identifier,
            Tok::Indent,
            Tok::Identifier,
            Tok::Dedent,
            Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
    }

    SECTION("Indents 2") {
        auto file = make_test_code_file(
            R"(
a:
    b
  c
)"
        );
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Identifier,
            Tok::Indent,
            Tok::Identifier,
            Tok::Identifier,
            Tok::Dedent,
            Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
    }

    SECTION("Indents 3") {
        auto file = make_test_code_file(
            R"(
a:
  b
c
)"
        );
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Identifier,
            Tok::Indent,
            Tok::Identifier,
            Tok::Dedent,
            Tok::Identifier,
            Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
    }

    SECTION("Indents 4") {
        auto file = make_test_code_file(
            R"(
a:
    b:
        c
    d
e
)"
        );
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Identifier,
            Tok::Indent,
            Tok::Identifier,
            Tok::Indent,
            Tok::Identifier,
            Tok::Dedent,
            Tok::Identifier,
            Tok::Dedent,
            Tok::Identifier,
            Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
    }

    SECTION("Indents 5") {
        auto file = make_test_code_file(
            R"(
a:
  b
c
  d
)"
        );
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Identifier,
            Tok::Indent,
            Tok::Identifier,
            Tok::Dedent,
            Tok::Identifier,
            Tok::Identifier,
            Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
    }

    SECTION("Indents 6") {
        auto file = make_test_code_file(
            R"(
a:
  b

  d
)"
        );
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Identifier,
            Tok::Indent,
            Tok::Identifier,
            Tok::Identifier,
            Tok::Dedent,
            Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
    }

    SECTION("Indents 7") {
        auto file = make_test_code_file(
            R"(
a:
    b:
        c
d
)"
        );
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Identifier,
            Tok::Indent,
            Tok::Identifier,
            Tok::Indent,
            Tok::Identifier,
            Tok::Dedent,
            Tok::Dedent,
            Tok::Identifier,
            Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
    }

    lexer.reset();
    Logger::inst().reset();
}
