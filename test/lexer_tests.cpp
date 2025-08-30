#include <memory>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "../src/common/code_file.h"
#include "../src/debug/test_utils.h"
#include "../src/lexer/lexer.h"
#include "../src/lexer/token.h"
#include "../src/logger/logger.h"

TEST_CASE("Sanity check", "[sanity]") {
    REQUIRE(1 == 1);
}

// MARK: Lexer tests

TEST_CASE("Lexer single characters", "[lexer]") {
    Lexer lexer;

    SECTION("Grouping characters 1") {
        auto file = make_test_code_file("()");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {Tok::LParen, Tok::RParen, Tok::Eof};
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
        std::vector<Tok> expected = {Tok::Comma, Tok::Semicolon, Tok::Eof};
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
            Tok::Slash, Tok::Plus, Tok::Minus, Tok::Star, Tok::Percent, Tok::Eof
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

    SECTION("Colon operators") {
        auto file = make_test_code_file(": :: :::");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Colon, Tok::ColonColon, Tok::ColonColon, Tok::Colon, Tok::Eof
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
            Tok::Identifier, Tok::Indent, Tok::Identifier, Tok::Dedent, Tok::Eof
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
            Tok::Identifier, Tok::Colon, Tok::Identifier, Tok::Eof
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
            Tok::KwLet, Tok::KwVar, Tok::Identifier, Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
    }

    SECTION("Basic keywords 2") {
        auto file = make_test_code_file("not true and true or true");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::KwNot,
            Tok::Bool,
            Tok::KwAnd,
            Tok::Bool,
            Tok::KwOr,
            Tok::Bool,
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
        std::vector<Tok> expected = {Tok::Int, Tok::Float, Tok::Eof};
        REQUIRE(extract_token_types(tokens) == expected);
        CHECK(std::any_cast<int32_t>(tokens[0]->literal) == 123);
        CHECK(std::any_cast<double>(tokens[1]->literal) == 123.0);
    }

    SECTION("Numbers 2") {
        auto file = make_test_code_file("0x1A 0o17 0b101");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {Tok::Int, Tok::Int, Tok::Int, Tok::Eof};
        REQUIRE(extract_token_types(tokens) == expected);
        CHECK(std::any_cast<int32_t>(tokens[0]->literal) == 0x1A);
        CHECK(std::any_cast<int32_t>(tokens[1]->literal) == 017);
        CHECK(std::any_cast<int32_t>(tokens[2]->literal) == 0b101);
    }

    SECTION("Numbers 3") {
        auto file = make_test_code_file("1.23 1.23f");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {Tok::Float, Tok::Float, Tok::Eof};
        REQUIRE(extract_token_types(tokens) == expected);
        CHECK(std::any_cast<double>(tokens[0]->literal) == 1.23);
        CHECK(std::any_cast<double>(tokens[1]->literal) == 1.23);
    }

    SECTION("Numbers 4") {
        auto file =
            make_test_code_file("1.23e10 1.23e-10 1.23E10 1.23E-10 123E+10");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Float, Tok::Float, Tok::Float, Tok::Float, Tok::Float, Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
        CHECK(std::any_cast<double>(tokens[0]->literal) == 1.23e10);
        CHECK(std::any_cast<double>(tokens[1]->literal) == 1.23e-10);
        CHECK(std::any_cast<double>(tokens[2]->literal) == 1.23E10);
        CHECK(std::any_cast<double>(tokens[3]->literal) == 1.23E-10);
        CHECK(std::any_cast<double>(tokens[4]->literal) == 123E+10);
    }

    SECTION("Numbers 5") {
        auto file = make_test_code_file("0 0.0 0.0 0 0");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Int, Tok::Float, Tok::Float, Tok::Int, Tok::Int, Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
        CHECK(std::any_cast<int32_t>(tokens[0]->literal) == 0);
        CHECK(std::any_cast<double>(tokens[1]->literal) == 0.0);
        CHECK(std::any_cast<double>(tokens[2]->literal) == 0.0);
        CHECK(std::any_cast<int32_t>(tokens[3]->literal) == 0);
        CHECK(std::any_cast<int32_t>(tokens[4]->literal) == 0);
    }

    SECTION("Numbers 6") {
        auto file = make_test_code_file("0xAbCdEf 0x0 0x00");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {Tok::Int, Tok::Int, Tok::Int, Tok::Eof};
        REQUIRE(extract_token_types(tokens) == expected);
        CHECK(std::any_cast<int32_t>(tokens[0]->literal) == 0xabcdef);
        CHECK(std::any_cast<int32_t>(tokens[1]->literal) == 0);
        CHECK(std::any_cast<int32_t>(tokens[2]->literal) == 0);
    }

    SECTION("Numbers 7") {
        auto file = make_test_code_file("0o123 0123 0o0");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {Tok::Int, Tok::Int, Tok::Int, Tok::Eof};
        REQUIRE(extract_token_types(tokens) == expected);
        CHECK(std::any_cast<int32_t>(tokens[0]->literal) == 0123);
        CHECK(std::any_cast<int32_t>(tokens[1]->literal) == 123);
        CHECK(std::any_cast<int32_t>(tokens[2]->literal) == 0);
    }

    SECTION("Numbers with underscores 1") {
        auto file = make_test_code_file("1_000 0b1010_1010 0o_755 0xFF_FF");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Int, Tok::Int, Tok::Int, Tok::Int, Tok::Eof
        };
        REQUIRE(extract_token_types(tokens) == expected);
        CHECK(std::any_cast<int32_t>(tokens[0]->literal) == 1000);
        CHECK(std::any_cast<int32_t>(tokens[1]->literal) == 0b10101010);
        CHECK(std::any_cast<int32_t>(tokens[2]->literal) == 0755);
        CHECK(std::any_cast<int32_t>(tokens[3]->literal) == 0xFFFF);
    }

    SECTION("Numbers with underscores 2") {
        auto file = make_test_code_file("1_00_00 1__0 1_0_");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {Tok::Int, Tok::Int, Tok::Int, Tok::Eof};
        REQUIRE(extract_token_types(tokens) == expected);
        CHECK(std::any_cast<int32_t>(tokens[0]->literal) == 10000);
        CHECK(std::any_cast<int32_t>(tokens[1]->literal) == 10);
        CHECK(std::any_cast<int32_t>(tokens[2]->literal) == 10);
    }

    lexer.reset();
    Logger::inst().reset();
}

TEST_CASE("Lexer str literals", "[lexer]") {
    Lexer lexer;

    SECTION("String literals 1") {
        auto file = make_test_code_file(R"("abc")");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {Tok::Str, Tok::Eof};
        REQUIRE(extract_token_types(tokens) == expected);
        CHECK(std::any_cast<std::string>(tokens[0]->literal) == "abc");
    }

    SECTION("String literals 2") {
        auto file = make_test_code_file(R"("abc" "def")");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {Tok::Str, Tok::Str, Tok::Eof};
        REQUIRE(extract_token_types(tokens) == expected);
        CHECK(std::any_cast<std::string>(tokens[0]->literal) == "abc");
        CHECK(std::any_cast<std::string>(tokens[1]->literal) == "def");
    }

    SECTION("String literals 3") {
        auto file = make_test_code_file(R"("")");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {Tok::Str, Tok::Eof};
        REQUIRE(extract_token_types(tokens) == expected);
        CHECK(std::any_cast<std::string>(tokens[0]->literal) == "");
    }

    SECTION("String literal esc sequences") {
        auto file = make_test_code_file(R"("\n\t\r\\\"")");
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {Tok::Str, Tok::Eof};
        REQUIRE(extract_token_types(tokens) == expected);
        CHECK(std::any_cast<std::string>(tokens[0]->literal) == "\n\t\r\\\"");
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
            Tok::Identifier, Tok::Identifier, Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
        REQUIRE(tokens.size() == 3);
        CHECK(tokens.at(0)->lexeme == "a");
        CHECK(tokens.at(0)->location.line == 2);
        CHECK(tokens.at(1)->lexeme == "c");
        CHECK(tokens.at(1)->location.line == 4);
    }

    SECTION("Multi-line comments") {
        auto file = make_test_code_file(
            R"(
a
/* b
c
d */
e
)"
        );
        auto tokens = lexer.scan(file);
        std::vector<Tok> expected = {
            Tok::Identifier, Tok::Identifier, Tok::Eof
        };
        CHECK(extract_token_types(tokens) == expected);
        REQUIRE(tokens.size() == 3);
        CHECK(tokens.at(0)->lexeme == "a");
        CHECK(tokens.at(0)->location.line == 2);
        CHECK(tokens.at(1)->lexeme == "e");
        CHECK(tokens.at(1)->location.line == 6);
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

    SECTION("Digit in wrong base 1") {
        auto file = make_test_code_file("123abc");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::DigitInWrongBase);
    }

    SECTION("Digit in wrong base 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("0b2");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::DigitInWrongBase);
    }

    SECTION("Unexpected end of number 1") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("0b");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UnexpectedEndOfNumber);
    }

    SECTION("Unexpected end of number 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("0o_");
        auto tokens = lexer.scan(file);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UnexpectedEndOfNumber);
    }

    SECTION("Invalid character after number") {
        auto file = make_test_code_file("123gg");
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
