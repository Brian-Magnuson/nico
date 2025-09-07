#include <memory>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "../src/checker/global_checker.h"
#include "../src/checker/local_checker.h"
#include "../src/common/code_file.h"
#include "../src/lexer/lexer.h"
#include "../src/lexer/token.h"
#include "../src/logger/logger.h"
#include "../src/nodes/ast_node.h"
#include "../src/parser/parser.h"
#include "utils/test_utils.h"

TEST_CASE("Local variable declarations", "[checker]") {
    Lexer lexer;
    Parser parser;
    GlobalChecker global_checker;
    LocalChecker local_checker;
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
    Logger::inst().reset();
}

TEST_CASE("Local unary expressions", "[checker]") {
    Lexer lexer;
    Parser parser;
    GlobalChecker global_checker;
    LocalChecker local_checker;
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
    Logger::inst().reset();
}

TEST_CASE("Local binary expressions", "[checker]") {
    Lexer lexer;
    Parser parser;
    GlobalChecker global_checker;
    LocalChecker local_checker;
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
    Logger::inst().reset();
}

TEST_CASE("Local assignment expressions", "[checker]") {
    Lexer lexer;
    Parser parser;
    GlobalChecker global_checker;
    LocalChecker local_checker;
    Logger::inst().set_printing_enabled(false);

    SECTION("Valid assignment expressions") {
        auto file = make_test_code_file("let var a = 1 a = 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Assignment type mismatch 1") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let var a: i32 = 1 a = true");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::AssignmentTypeMismatch);
    }

    SECTION("Assignment type mismatch 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let var a: i32 = 1 a = 1.0");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::AssignmentTypeMismatch);
    }

    SECTION("Assignment not an lvalue 1") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("1 = 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NotAnLValue);
    }

    SECTION("Assignment not an lvalue 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("(1 + 1) = 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NotAnLValue);
    }

    SECTION("Assignment not an lvalue 3") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let var a = 1 (a = 1) = 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NotAnLValue);
    }

    SECTION("Assignment not an lvalue 4") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let var a = 1; -a = 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NotAnLValue);
    }

    lexer.reset();
    parser.reset();
    Logger::inst().reset();
}

TEST_CASE("Local print statements", "[checker]") {
    Lexer lexer;
    Parser parser;
    GlobalChecker global_checker;
    LocalChecker local_checker;
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
    Logger::inst().reset();
}

TEST_CASE("Local block expressions", "[checker]") {
    Lexer lexer;
    Parser parser;
    GlobalChecker global_checker;
    LocalChecker local_checker;
    Logger::inst().set_printing_enabled(false);

    SECTION("Valid block expression") {
        Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("block { let a = 1 printout a }");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Block expression yield 1") {
        Logger::inst().set_printing_enabled(true);
        auto file =
            make_test_code_file("block { let a = 1 printout a yield a }");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Block expression yield 2") {
        Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let var a = 1 a = block { yield 2 }");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Block expression nested yield") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file(
            "let var a = 1 a = block { yield block { yield 2 } }"
        );
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Yield outside local scope") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("yield 1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::YieldOutsideLocalScope);
    }

    SECTION("Incompatible yield types") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("block { yield 1 yield true }");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::YieldTypeMismatch);
    }

    SECTION("Block without yield") {
        // Logger::inst().set_printing_enabled(true);
        auto file =
            make_test_code_file("let var a = 1 a = block { let b = 1 }");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::AssignmentTypeMismatch);
    }

    lexer.reset();
    parser.reset();
    Logger::inst().reset();
}
