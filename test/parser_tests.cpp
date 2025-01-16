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

    lexer.reset();
    parser.reset();
    Logger::inst().reset();
}
