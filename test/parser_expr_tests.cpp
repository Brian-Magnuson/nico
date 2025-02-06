#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <utility>
#include <vector>

#include "../src/compiler/code_file.h"
#include "../src/debug/ast_printer.h"
#include "../src/debug/test_utils.h"
#include "../src/lexer/lexer.h"
#include "../src/lexer/token.h"
#include "../src/logger/logger.h"
#include "../src/parser/parser.h"
#include "../src/parser/stmt.h"

// MARK: Expr tests

TEST_CASE("Parser basic", "[parser]") {
    Lexer lexer;
    Parser parser;
    AstPrinter printer;

    SECTION("Basic 1") {
        auto file = make_test_code_file("basic");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (ident basic))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Basic 2") {
        auto file = make_test_code_file("123");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (lit 123))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    lexer.reset();
    parser.reset();
    Logger::inst().reset();
}

TEST_CASE("Parser expressions", "[parser]") {
    Lexer lexer;
    Parser parser;
    AstPrinter printer;

    SECTION("Unary 1") {
        auto file = make_test_code_file("-123");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (unary - (lit 123)))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Binary 1") {
        auto file = make_test_code_file("1 + 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (binary + (lit 1) (lit 2)))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Binary 2") {
        auto file = make_test_code_file("1 + 2 * 3");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (binary + (lit 1) (binary * (lit 2) (lit 3))))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Binary 3") {
        auto file = make_test_code_file("1 * 2 1 + 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (binary * (lit 1) (lit 2)))",
            "(expr (binary + (lit 1) (lit 2)))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Binary 4") {
        auto file = make_test_code_file("1 * -2 + -3");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (binary + (binary * (lit 1) (unary - (lit 2))) (unary - (lit 3))))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Binary 5") {
        auto file = make_test_code_file("true and false");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (binary and (lit true) (lit false)))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Binary 6") {
        auto file = make_test_code_file("true or false and false");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (binary or (lit true) (binary and (lit false) (lit false))))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Binary 7") {
        auto file = make_test_code_file("true or not true");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (binary or (lit true) (unary not (lit true))))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Assignment 1") {
        auto file = make_test_code_file("a = 1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (assign (ident a) (lit 1)))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Assignment 2") {
        auto file = make_test_code_file("a = b = c");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (assign (ident a) (assign (ident b) (ident c))))",
            "(stmt:eof)"
        };
    }

    lexer.reset();
    parser.reset();
    Logger::inst().reset();
}
