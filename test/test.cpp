#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <iterator>
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

TEST_CASE("Sanity check", "[sanity]") {
    REQUIRE(1 == 1);
}

// MARK: Lexer tests

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
        CHECK(extract_token_types(tokens) == expected);
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
        CHECK(extract_token_types(tokens) == expected);
    }

    SECTION("Other single character tokens") {
        auto file = make_test_code_file(",;");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Comma,
            Tok::Semicolon,
            Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
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
        CHECK(extract_token_types(tokens) == expected);
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
        CHECK(extract_token_types(tokens) == expected);
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
        CHECK(extract_token_types(tokens) == expected);
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
        CHECK(extract_token_types(tokens) == expected);
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
        CHECK(extract_token_types(tokens) == expected);
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
        CHECK(extract_token_types(tokens) == expected);
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
        CHECK(extract_token_types(tokens) == expected);
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
        CHECK(extract_token_types(tokens) == expected);
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
        CHECK(extract_token_types(tokens) == expected);
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
        CHECK(extract_token_types(tokens) == expected);
    }

    SECTION("Indents 8") {
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
        CHECK(extract_token_types(tokens) == expected);
    }

    SECTION("Indents 8") {
        auto file = make_test_code_file("a:   b");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Identifier,
            Tok::Colon,
            Tok::Identifier,
            Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
    }

    lexer.reset();
    Logger::inst().reset();
}

TEST_CASE("Lexer indents and groupings", "[lexer]") {
    Lexer lexer;

    SECTION("Indents and groupings 1") {
        auto file = make_test_code_file(
            R"(
a: 
    [
        b:
            c
]
)"
        );
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Identifier,
            Tok::Indent,
            Tok::LSquare,
            Tok::Identifier,
            Tok::Colon,
            Tok::Identifier,
            Tok::RSquare,
            Tok::Dedent,
            Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
    }

    SECTION("Indents and groupings 2") {
        auto file = make_test_code_file(
            R"(
a: 
    [
        b
]
    c
d
)"
        );
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Identifier,
            Tok::Indent,
            Tok::LSquare,
            Tok::Identifier,
            Tok::RSquare,
            Tok::Identifier,
            Tok::Dedent,
            Tok::Identifier,
            Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
    }

    lexer.reset();
    Logger::inst().reset();
}

TEST_CASE("Lexer basic keywords", "[lexer]") {
    Lexer lexer;

    SECTION("Basic keywords 1") {
        auto file = make_test_code_file("let var x");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::KwLet,
            Tok::KwVar,
            Tok::Identifier,
            Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
    }

    lexer.reset();
    Logger::inst().reset();
}

TEST_CASE("Lexer numbers", "[lexer]") {
    Lexer lexer;

    SECTION("Numbers 1") {
        auto file = make_test_code_file("123 123f");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Int,
            Tok::Float,
            Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
    }

    SECTION("Numbers 2") {
        auto file = make_test_code_file("0x1A 0o17 0b101");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Int,
            Tok::Int,
            Tok::Int,
            Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
    }

    SECTION("Numbers 3") {
        auto file = make_test_code_file("1.23 1.23f");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Float,
            Tok::Float,
            Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
    }

    SECTION("Numbers 4") {
        auto file = make_test_code_file("1.23e10 1.23e-10 1.23E10 1.23E-10 123E+10");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Float,
            Tok::Float,
            Tok::Float,
            Tok::Float,
            Tok::Float,
            Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
    }

    lexer.reset();
    Logger::inst().reset();
}

TEST_CASE("Lexer str literals", "[lexer]") {
    Lexer lexer;

    SECTION("String literals 1") {
        auto file = make_test_code_file(R"("abc")");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Str,
            Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
    }

    SECTION("String literals 2") {
        auto file = make_test_code_file(R"("abc" "def")");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Str,
            Tok::Str,
            Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
    }

    SECTION("String literal esc sequences") {
        auto file = make_test_code_file(R"("\n\t\r\\\"")");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Str,
            Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
    }

    lexer.reset();
    Logger::inst().reset();
}

TEST_CASE("Lexer comments", "[lexer]") {
    Lexer lexer;

    SECTION("Single-line comments") {
        auto file = make_test_code_file(
            R"(
a
// b
c
)"
        );

        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Identifier,
            Tok::Identifier,
            Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
        REQUIRE(tokens.size() == 3);
        CHECK(tokens.at(0)->lexeme == "a");
        CHECK(tokens.at(0)->location.line == 2);
        CHECK(tokens.at(1)->lexeme == "c");
        CHECK(tokens.at(1)->location.line == 4);
    }

    lexer.reset();
    Logger::inst().reset();
}

// MARK: Error tests

TEST_CASE("Lexer character errors", "[lexer]") {
    Lexer lexer;
    Logger::inst().set_printing_enabled(false);

    SECTION("Invalid characters") {
        auto file = make_test_code_file("\v");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UnexpectedChar);
    }

    SECTION("Unclosed grouping 1") {
        auto file = make_test_code_file("(");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UnclosedGrouping);
    }

    SECTION("Unclosed grouping 2") {
        auto file = make_test_code_file("{)");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UnclosedGrouping);
    }

    lexer.reset();
    Logger::inst().reset();
}

TEST_CASE("Lexer spacing errors", "[lexer]") {
    Lexer lexer;
    Logger::inst().set_printing_enabled(false);

    SECTION("Mixed spacing") {
        auto file = make_test_code_file("\t  abc");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::MixedLeftSpacing);
    }

    SECTION("Inconsistent left spacing") {
        auto file = make_test_code_file("\tabc\n  abc");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::InconsistentLeftSpacing);
    }

    SECTION("Malformed indent") {
        auto file = make_test_code_file("  a:\n  b");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::MalformedIndent);
    }

    lexer.reset();
    Logger::inst().reset();
}

TEST_CASE("Lexer number scanning errors", "[lexer]") {
    Lexer lexer;
    Logger::inst().set_printing_enabled(false);

    SECTION("Unexpected dot in number") {
        auto file = make_test_code_file("1.2.3");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UnexpectedDotInNumber);
    }

    SECTION("Unexpected exponent in number") {
        auto file = make_test_code_file("1.2e");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UnexpectedExpInNumber);
    }

    SECTION("Invalid character after number") {
        auto file = make_test_code_file("123abc");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::InvalidCharAfterNumber);
    }

    SECTION("Dot in hex number") {
        auto file = make_test_code_file("0x1.2");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UnexpectedDotInNumber);
    }

    SECTION("Dot in exp part") {
        auto file = make_test_code_file("1.2e1.2");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UnexpectedDotInNumber);
    }

    lexer.reset();
    Logger::inst().reset();
}

TEST_CASE("Lexer str scanning errors", "[lexer]") {
    Lexer lexer;
    Logger::inst().set_printing_enabled(false);

    SECTION("Unterminated string") {
        auto file = make_test_code_file(R"("abc)");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UnterminatedStr);
    }

    SECTION("Invalid escape sequence") {
        auto file = make_test_code_file(R"("\a")");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::InvalidEscSeq);
    }

    lexer.reset();
    Logger::inst().reset();
}

TEST_CASE("Lexer comment scanning errors", "[lexer]") {
    Lexer lexer;
    Logger::inst().set_printing_enabled(false);

    SECTION("Unclosed comment 1") {
        auto file = make_test_code_file("/*");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UnclosedComment);
    }

    SECTION("Unclosed comment 2") {
        auto file = make_test_code_file("/*/*");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UnclosedComment);
    }

    SECTION("Unclosed comment 3") {
        Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("/*/*/*\ncomment */");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UnclosedComment);
    }

    SECTION("Closing unopened comment") {
        auto file = make_test_code_file("*/");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::ClosingUnopenedComment);
    }

    lexer.reset();
    Logger::inst().reset();
}
