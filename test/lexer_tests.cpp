#include <memory>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "nico/frontend/lexer.h"
#include "nico/frontend/utils/frontend_context.h"
#include "nico/shared/code_file.h"
#include "nico/shared/logger.h"
#include "nico/shared/token.h"

#include "test_utils.h"

/**
 * @brief Run a lexer test with the given source code and expected token types.
 *
 * @param src_code The source code to test.
 * @param expected The expected token types.
 */
void run_lexer_test(
    std::string_view src_code, const std::vector<Tok>& expected
) {
    auto context = std::make_unique<FrontendContext>();
    auto file = make_test_code_file(src_code);
    Lexer lexer;
    lexer.scan(context, file);
    CHECK(extract_token_types(context->scanned_tokens) == expected);

    lexer.reset();
    context->reset();
    Logger::inst().reset();
}

/**
 * @brief Run a lexer error test with the given source code and expected error.
 *
 * Erroneous input can potentially produce multiple errors.
 * Only the first error is checked by this test.
 *
 * @param src_code The source code to test.
 * @param expected_error The expected error.
 * @param print_errors Whether to print errors. Defaults to false.
 */
void run_lexer_error_test(
    std::string_view src_code, Err expected_error, bool print_errors = false
) {
    Logger::inst().set_printing_enabled(print_errors);

    auto context = std::make_unique<FrontendContext>();
    auto file = make_test_code_file(src_code);
    Lexer lexer;
    lexer.scan(context, file);

    auto& errors = Logger::inst().get_errors();
    REQUIRE(errors.size() >= 1);
    CHECK(errors.at(0) == expected_error);

    lexer.reset();
    context->reset();
    Logger::inst().reset();
}

TEST_CASE("Sanity check", "[sanity]") {
    REQUIRE(1 == 1);
}

// MARK: Lexer tests
TEST_CASE("Lexer single characters (run_lexer_test)", "[lexer]") {

    SECTION("Grouping characters 1") {
        run_lexer_test("()", {Tok::LParen, Tok::RParen, Tok::Eof});
    }

    SECTION("Grouping characters 2") {
        run_lexer_test(
            "()[]{}",
            {Tok::LParen,
             Tok::RParen,
             Tok::LSquare,
             Tok::RSquare,
             Tok::LBrace,
             Tok::RBrace,
             Tok::Eof}
        );
    }

    SECTION("Other single character tokens") {
        run_lexer_test(",;", {Tok::Comma, Tok::Semicolon, Tok::Eof});
    }
}

TEST_CASE("Lexer short tokens (run_lexer_test)", "[lexer]") {
    SECTION("Arithmetic operators") {
        run_lexer_test(
            "/+-*%",
            {Tok::Slash,
             Tok::Plus,
             Tok::Minus,
             Tok::Star,
             Tok::Percent,
             Tok::Eof}
        );
    }

    SECTION("Assignment operators") {
        run_lexer_test(
            "+=-=*=/=%=&=|=^=",
            {Tok::PlusEq,
             Tok::MinusEq,
             Tok::StarEq,
             Tok::SlashEq,
             Tok::PercentEq,
             Tok::AmpEq,
             Tok::BarEq,
             Tok::CaretEq,
             Tok::Eof}
        );
    }

    SECTION("Comparison operators") {
        run_lexer_test(
            "==!=>=<=><",
            {Tok::EqEq,
             Tok::BangEq,
             Tok::GtEq,
             Tok::LtEq,
             Tok::Gt,
             Tok::Lt,
             Tok::Eof}
        );
    }

    SECTION("Colon operators") {
        run_lexer_test(
            ": :: :::",
            {Tok::Colon, Tok::ColonColon, Tok::ColonColon, Tok::Colon, Tok::Eof}
        );
    }
}

TEST_CASE("Lexer simple indents (run_lexer_test)", "[lexer]") {
    SECTION("Indents 1") {
        run_lexer_test(
            R"(
a:
  b
)",
            {Tok::Identifier,
             Tok::Indent,
             Tok::Identifier,
             Tok::Dedent,
             Tok::Eof}
        );
    }

    SECTION("Indents 2") {
        run_lexer_test(
            R"(
a:
    b
  c
)",
            {Tok::Identifier,
             Tok::Indent,
             Tok::Identifier,
             Tok::Identifier,
             Tok::Dedent,
             Tok::Eof}
        );
    }

    SECTION("Indents 3") {
        run_lexer_test(
            R"(
a:
  b
c
)",
            {Tok::Identifier,
             Tok::Indent,
             Tok::Identifier,
             Tok::Dedent,
             Tok::Identifier,
             Tok::Eof}
        );
    }

    SECTION("Indents 4") {
        run_lexer_test(
            R"(
a:
    b:
        c
    d
e
)",
            {Tok::Identifier,
             Tok::Indent,
             Tok::Identifier,
             Tok::Indent,
             Tok::Identifier,
             Tok::Dedent,
             Tok::Identifier,
             Tok::Dedent,
             Tok::Identifier,
             Tok::Eof}
        );
    }

    SECTION("Indents 5") {
        run_lexer_test(
            R"(
a:
  b
c
  d
)",
            {Tok::Identifier,
             Tok::Indent,
             Tok::Identifier,
             Tok::Dedent,
             Tok::Identifier,
             Tok::Identifier,
             Tok::Eof}
        );
    }

    SECTION("Indents 6") {
        run_lexer_test(
            R"(
a:
  b

  d
)",
            {Tok::Identifier,
             Tok::Indent,
             Tok::Identifier,
             Tok::Identifier,
             Tok::Dedent,
             Tok::Eof}
        );
    }

    SECTION("Indents 7") {
        run_lexer_test(
            R"(
a:
    b:
        c
d
)",
            {Tok::Identifier,
             Tok::Indent,
             Tok::Identifier,
             Tok::Indent,
             Tok::Identifier,
             Tok::Dedent,
             Tok::Dedent,
             Tok::Identifier,
             Tok::Eof}
        );
    }

    SECTION("Indents 8") {
        run_lexer_test(
            R"(
    a:
        b:
            c
d
)",
            {Tok::Identifier,
             Tok::Indent,
             Tok::Identifier,
             Tok::Indent,
             Tok::Identifier,
             Tok::Dedent,
             Tok::Dedent,
             Tok::Identifier,
             Tok::Eof}
        );
    }

    SECTION("Indents 8") {
        run_lexer_test(
            "a:   b",
            {Tok::Identifier, Tok::Colon, Tok::Identifier, Tok::Eof}
        );
    }

    SECTION("Indents 9") {
        run_lexer_test(
            R"(
a:
    // comment
    )",
            {Tok::Identifier, Tok::Indent, Tok::Dedent, Tok::Eof}
        );
    }
}

TEST_CASE("Lexer indents and groupings (run_lexer_test)", "[lexer]") {
    SECTION("Indents and groupings 1") {
        run_lexer_test(
            R"(
a: 
    [
        b:
            c
]
)",
            {Tok::Identifier,
             Tok::Indent,
             Tok::LSquare,
             Tok::Identifier,
             Tok::Colon,
             Tok::Identifier,
             Tok::RSquare,
             Tok::Dedent,
             Tok::Eof}
        );
    }

    SECTION("Indents and groupings 2") {
        run_lexer_test(
            R"(
a: 
    [
        b
]
    c
d
)",
            {Tok::Identifier,
             Tok::Indent,
             Tok::LSquare,
             Tok::Identifier,
             Tok::RSquare,
             Tok::Identifier,
             Tok::Dedent,
             Tok::Identifier,
             Tok::Eof}
        );
    }
}

TEST_CASE("Lexer basic keywords (run_lexer_test)", "[lexer]") {
    SECTION("Basic keywords 1") {
        run_lexer_test(
            "let var x",
            {Tok::KwLet, Tok::KwVar, Tok::Identifier, Tok::Eof}
        );
    }

    SECTION("Basic keywords 2") {
        run_lexer_test(
            "not true and true or true",
            {Tok::KwNot,
             Tok::Bool,
             Tok::KwAnd,
             Tok::Bool,
             Tok::KwOr,
             Tok::Bool,
             Tok::Eof}
        );
    }
}

// TODO: Prior to test cleanup, Lexer number tests also checked that the literal
// values match the expected values. Please consider re-adding those checks.

TEST_CASE("Lexer numbers (run_lexer_test)", "[lexer]") {
    SECTION("Numbers 1") {
        run_lexer_test("123 123f", {Tok::Int, Tok::Float, Tok::Eof});
    }

    SECTION("Numbers 2") {
        run_lexer_test(
            "0x1A 0o17 0b101",
            {Tok::Int, Tok::Int, Tok::Int, Tok::Eof}
        );
    }

    SECTION("Numbers 3") {
        run_lexer_test("1.23 1.23f", {Tok::Float, Tok::Float, Tok::Eof});
    }

    SECTION("Numbers 4") {
        run_lexer_test(
            "1.23e10 1.23e-10 1.23E10 1.23E-10 123E+10",
            {Tok::Float,
             Tok::Float,
             Tok::Float,
             Tok::Float,
             Tok::Float,
             Tok::Eof}
        );
    }

    SECTION("Numbers 5") {
        run_lexer_test(
            "0 0.0 0.0 0 0",
            {Tok::Int, Tok::Float, Tok::Float, Tok::Int, Tok::Int, Tok::Eof}
        );
    }

    SECTION("Numbers 6") {
        run_lexer_test(
            "0xAbCdEf 0x0 0x00",
            {Tok::Int, Tok::Int, Tok::Int, Tok::Eof}
        );
    }

    SECTION("Numbers 7") {
        run_lexer_test(
            "0o123 0123 0o0",
            {Tok::Int, Tok::Int, Tok::Int, Tok::Eof}
        );
    }

    SECTION("Numbers with underscores 1") {
        run_lexer_test(
            "1_000 0b1010_1010 0o_755 0xFF_FF",
            {Tok::Int, Tok::Int, Tok::Int, Tok::Int, Tok::Eof}
        );
    }

    SECTION("Numbers with underscores 2") {
        run_lexer_test(
            "1_00_00 1__0 1_0_",
            {Tok::Int, Tok::Int, Tok::Int, Tok::Eof}
        );
    }
}

TEST_CASE("Lexer str literals (run_lexer_test)", "[lexer]") {
    SECTION("String literals 1") {
        run_lexer_test(R"("abc")", {Tok::Str, Tok::Eof});
    }

    SECTION("String literals 2") {
        run_lexer_test(R"("abc" "def")", {Tok::Str, Tok::Str, Tok::Eof});
    }

    SECTION("String literals 3") {
        run_lexer_test(R"("")", {Tok::Str, Tok::Eof});
    }

    SECTION("String literal esc sequences") {
        run_lexer_test(R"("\n\t\r\\\"")", {Tok::Str, Tok::Eof});
    }
}

TEST_CASE("Lexer comments (run_lexer_test)", "[lexer]") {
    SECTION("Single-line comments") {
        run_lexer_test(
            R"(
a
// b
c
)",
            {Tok::Identifier, Tok::Identifier, Tok::Eof}
        );
    }

    SECTION("Multi-line comments") {
        run_lexer_test(
            R"(
a
/* b
c
d */
e
)",
            {Tok::Identifier, Tok::Identifier, Tok::Eof}
        );
    }
}

// MARK: Error tests

TEST_CASE("Lexer character errors", "[lexer]") {
    SECTION("Invalid characters") {
        run_lexer_error_test("\v", Err::UnexpectedChar);
    }

    SECTION("Unclosed grouping 1") {
        run_lexer_error_test("(", Err::UnclosedGrouping);
    }

    SECTION("Unclosed grouping 2") {
        run_lexer_error_test("{)", Err::UnclosedGrouping);
    }
}

TEST_CASE("Lexer spacing errors", "[lexer]") {
    SECTION("Mixed spacing") {
        run_lexer_error_test("\t  abc", Err::MixedLeftSpacing);
    }

    SECTION("Inconsistent left spacing") {
        run_lexer_error_test("\tabc\n  abc", Err::InconsistentLeftSpacing);
    }

    SECTION("Malformed indent 1") {
        run_lexer_error_test("  a:\n  b", Err::MalformedIndent);
    }

    SECTION("Malformed indent 2") {
        run_lexer_error_test("    a:\n", Err::MalformedIndent);
    }
}

TEST_CASE("Lexer number scanning errors", "[lexer]") {
    SECTION("Unexpected dot in number") {
        run_lexer_error_test("1.2.3", Err::UnexpectedDotInNumber);
    }

    SECTION("Unexpected exponent in number") {
        run_lexer_error_test("1.2e", Err::UnexpectedExpInNumber);
    }

    SECTION("Digit in wrong base 1") {
        run_lexer_error_test("123abc", Err::DigitInWrongBase);
    }

    SECTION("Digit in wrong base 2") {
        run_lexer_error_test("0b2", Err::DigitInWrongBase);
    }

    SECTION("Unexpected end of number 1") {
        run_lexer_error_test("0b", Err::UnexpectedEndOfNumber);
    }

    SECTION("Unexpected end of number 2") {
        run_lexer_error_test("0o_", Err::UnexpectedEndOfNumber);
    }

    SECTION("Invalid character after number") {
        run_lexer_error_test("123gg", Err::InvalidCharAfterNumber);
    }

    SECTION("Dot in hex number") {
        run_lexer_error_test("0x1.2", Err::UnexpectedDotInNumber);
    }

    SECTION("Dot in exp part") {
        run_lexer_error_test("1.2e1.2", Err::UnexpectedDotInNumber);
    }

    SECTION("Number out of range") {
        run_lexer_error_test(
            "99999999999999999999999999",
            Err::NumberOutOfRange
        );
    }

    SECTION("Float out of range") {
        run_lexer_error_test("1e9999999", Err::NumberOutOfRange);
    }
}

TEST_CASE("Lexer str scanning errors", "[lexer]") {
    SECTION("Unterminated string") {
        run_lexer_error_test(R"("abc)", Err::UnterminatedStr);
    }

    SECTION("Invalid escape sequence") {
        run_lexer_error_test(R"("\a")", Err::InvalidEscSeq);
    }
}

TEST_CASE("Lexer comment scanning errors", "[lexer]") {
    SECTION("Unclosed comment 1") {
        run_lexer_error_test("/*", Err::UnclosedComment);
    }

    SECTION("Unclosed comment 2") {
        run_lexer_error_test("/*/*", Err::UnclosedComment);
    }

    SECTION("Unclosed comment 3") {
        run_lexer_error_test("/*/*/*\ncomment */", Err::UnclosedComment);
    }

    SECTION("Closing unopened comment") {
        run_lexer_error_test("*/", Err::ClosingUnopenedComment);
    }
}
