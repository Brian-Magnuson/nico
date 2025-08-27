#include <memory>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "../src/checker/global_checker.h"
#include "../src/checker/local_checker.h"
#include "../src/checker/symbol_tree.h"
#include "../src/common/code_file.h"
#include "../src/debug/test_utils.h"
#include "../src/lexer/lexer.h"
#include "../src/lexer/token.h"
#include "../src/logger/logger.h"
#include "../src/parser/ast.h"
#include "../src/parser/parser.h"

TEST_CASE("Local variable declarations", "[checker]") {
    Lexer lexer;
    Parser parser;
    std::shared_ptr<SymbolTree> symbol_tree = std::make_shared<SymbolTree>();
    GlobalChecker global_checker(symbol_tree);
    LocalChecker local_checker(symbol_tree);
    Logger::inst().set_printing_enabled(false);

    SECTION("Valid local variable declarations") {
        auto file = make_test_code_file("let a = 1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Let type mismatch 1") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a: i32 = true");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::LetTypeMismatch);
    }

    SECTION("Let type mismatch 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a: i32 = 1.0");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::LetTypeMismatch);
    }

    SECTION("Let type mismatch 3") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = true let b: i32 = a");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::LetTypeMismatch);
    }

    SECTION("Undeclared identifier") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = b");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::UndeclaredName);
    }

    SECTION("Immutable variables") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = 1 let b = 2 a = b");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::AssignToImmutable);
    }

    SECTION("Variable name conflict") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = 1 let a = 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NameAlreadyExists);
    }

    SECTION("Primitive type name conflict") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let i32 = 1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NameIsReserved);
    }

    lexer.reset();
    parser.reset();
    symbol_tree->reset();
    Logger::inst().reset();
}

TEST_CASE("Local unary expressions", "[checker]") {
    Lexer lexer;
    Parser parser;
    std::shared_ptr<SymbolTree> symbol_tree = std::make_shared<SymbolTree>();
    GlobalChecker global_checker(symbol_tree);
    LocalChecker local_checker(symbol_tree);
    Logger::inst().set_printing_enabled(false);

    SECTION("Valid unary expressions") {
        auto file = make_test_code_file("let a = -1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Unary type mismatch 1") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = -true");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NoOperatorOverload);
    }

    lexer.reset();
    parser.reset();
    symbol_tree->reset();
    Logger::inst().reset();
}

TEST_CASE("Local binary expressions", "[checker]") {
    Lexer lexer;
    Parser parser;
    std::shared_ptr<SymbolTree> symbol_tree = std::make_shared<SymbolTree>();
    GlobalChecker global_checker(symbol_tree);
    LocalChecker local_checker(symbol_tree);
    Logger::inst().set_printing_enabled(false);

    SECTION("Valid binary expressions 1") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = 1 + 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Valid binary expressions 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = 1.0 + 2.0");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Binary type mismatch 1") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = 1 + true");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NoOperatorOverload);
    }

    SECTION("Binary type mismatch 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = true + 1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NoOperatorOverload);
    }

    SECTION("Binary type mismatch 3") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = true + false");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NoOperatorOverload);
    }

    lexer.reset();
    parser.reset();
    symbol_tree->reset();
    Logger::inst().reset();
}

TEST_CASE("Local print statements", "[checker]") {
    Lexer lexer;
    Parser parser;
    std::shared_ptr<SymbolTree> symbol_tree = std::make_shared<SymbolTree>();
    GlobalChecker global_checker(symbol_tree);
    LocalChecker local_checker(symbol_tree);
    Logger::inst().set_printing_enabled(false);

    SECTION("Print hello world") {
        auto file = make_test_code_file("printout \"Hello, World!\"");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    lexer.reset();
    parser.reset();
    symbol_tree->reset();
    Logger::inst().reset();
}
