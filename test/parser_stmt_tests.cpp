#include <memory>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "../src/common/code_file.h"
#include "../src/debug/ast_printer.h"
#include "../src/debug/test_utils.h"
#include "../src/lexer/lexer.h"
#include "../src/lexer/token.h"
#include "../src/logger/logger.h"
#include "../src/parser/ast.h"
#include "../src/parser/parser.h"

// MARK: Stmt tests

TEST_CASE("Parser let statements", "[parser]") {
    Lexer lexer;
    Parser parser;
    AstPrinter printer;

    SECTION("Let statements 1") {
        auto file = make_test_code_file("let a = 1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(stmt:let a (lit 1))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Let statements 2") {
        auto file = make_test_code_file("let var a = 1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(stmt:let var a (lit 1))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Let statements 3") {
        auto file = make_test_code_file("let a: i32 = 1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(stmt:let a i32 (lit 1))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);

        REQUIRE(ast.size() == 2);
        auto let_stmt = std::dynamic_pointer_cast<Stmt::Let>(ast[0]);
        REQUIRE(let_stmt != nullptr);
        REQUIRE(let_stmt->annotation.has_value());
        REQUIRE(let_stmt->annotation.value()->to_string() == "i32");
    }

    SECTION("Let statements 4") {
        auto file = make_test_code_file("let a: i32 let b: f64");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(stmt:let a i32)",
            "(stmt:let b f64)",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);

        REQUIRE(ast.size() == 3);
        auto let_stmt = std::dynamic_pointer_cast<Stmt::Let>(ast[1]);
        REQUIRE(let_stmt != nullptr);
        REQUIRE(let_stmt->annotation.has_value());
        REQUIRE(let_stmt->annotation.value()->to_string() == "f64");
    }

    SECTION("Let statements 5") {
        auto file = make_test_code_file("let a: Vector2D");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(stmt:let a Vector2D)",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);

        REQUIRE(ast.size() == 2);
        auto let_stmt = std::dynamic_pointer_cast<Stmt::Let>(ast[0]);
        REQUIRE(let_stmt != nullptr);
        REQUIRE(let_stmt->annotation.has_value());
        REQUIRE(let_stmt->annotation.value()->to_string() == "Vector2D");
    }

    SECTION("Let statements 6") {
        auto file = make_test_code_file("let a: i32 let b = 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(stmt:let a i32)",
            "(stmt:let b (lit 2))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    lexer.reset();
    parser.reset();
    Logger::inst().reset();
}

TEST_CASE("Parser print statements", "[parser]") {
    Lexer lexer;
    Parser parser;
    AstPrinter printer;

    SECTION("Print statements 1") {
        auto file = make_test_code_file("printout 1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(stmt:print (lit 1))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Print statements 2") {
        auto file = make_test_code_file("printout 1, 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(stmt:print (lit 1) (lit 2))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Print statements 3") {
        auto file = make_test_code_file("printout 1, 2, 3");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(stmt:print (lit 1) (lit 2) (lit 3))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Print statements 4") {
        auto file = make_test_code_file("printout \"Hello, World!\"");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(stmt:print (lit \"Hello, World!\"))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    lexer.reset();
    parser.reset();
    Logger::inst().reset();
}

TEST_CASE("Parser statement separation", "[parser]") {
    Lexer lexer;
    Parser parser;
    AstPrinter printer;

    SECTION("Unseparated binary statement") {
        auto file = make_test_code_file("1 - 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (binary - (lit 1) (lit 2)))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    SECTION("Separated unary statements") {
        auto file = make_test_code_file("1; - 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        std::vector<std::string> expected = {
            "(expr (lit 1))",
            "(expr (unary - (lit 2)))",
            "(stmt:eof)"
        };
        CHECK(printer.stmts_to_strings(ast) == expected);
    }

    lexer.reset();
    parser.reset();
    Logger::inst().reset();
}

// MARK: Error tests

TEST_CASE("Parser let stmt errors", "[parser]") {
    Lexer lexer;
    Parser parser;
    Logger::inst().set_printing_enabled(false);

    SECTION("Let missing identifier 1") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let");
        auto tokens = lexer.scan(file);
        parser.parse(std::move(tokens));
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NotAnIdentifier);
    }

    SECTION("Let missing identifier 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let var");
        auto tokens = lexer.scan(file);
        parser.parse(std::move(tokens));
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NotAnIdentifier);
    }

    SECTION("Let improper type") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a: 1 = 1");
        auto tokens = lexer.scan(file);
        parser.parse(std::move(tokens));
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NotAType);
    }

    SECTION("Let without type or value") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a");
        auto tokens = lexer.scan(file);
        parser.parse(std::move(tokens));
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::LetWithoutTypeOrValue);
    }

    lexer.reset();
    parser.reset();
    Logger::inst().reset();
}
