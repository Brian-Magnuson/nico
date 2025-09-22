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

    SECTION("Valid unary expression 1") {
        auto file = make_test_code_file("let a = -1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Valid unary expression 2") {
        auto file = make_test_code_file("let a = not true");
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

    SECTION("Unary type mismatch 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = not 1");
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

TEST_CASE("Local logical expressions", "[checker]") {
    Lexer lexer;
    Parser parser;
    GlobalChecker global_checker;
    LocalChecker local_checker;
    Logger::inst().set_printing_enabled(false);

    SECTION("Valid logical expressions 1") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = true and false");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Valid logical expressions 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = true or false and false");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Valid logical expressions 3") {
        Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = true or not true");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Logical type mismatch 1") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = 1 and true");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NoOperatorOverload);
    }

    SECTION("Logical type mismatch 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = true and 1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::NoOperatorOverload);
    }

    SECTION("Logical type mismatch 3") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a: i32 = true and true");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::LetTypeMismatch);
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
        CHECK(errors.at(0) == Err::NotAPossibleLValue);
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
        CHECK(errors.at(0) == Err::NotAPossibleLValue);
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
        CHECK(errors.at(0) == Err::NotAPossibleLValue);
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
        CHECK(errors.at(0) == Err::NotAPossibleLValue);
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

TEST_CASE("Local tuple expressions", "[checker]") {
    Lexer lexer;
    Parser parser;
    GlobalChecker global_checker;
    LocalChecker local_checker;
    Logger::inst().set_printing_enabled(false);

    SECTION("Valid tuple expression 1") {
        auto file = make_test_code_file("let a = (1, 2.1, true)");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Valid tuple expression 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file =
            make_test_code_file("let a: (i32, f64, bool) = (1, 2.1, true)");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Tuple expression type mismatch") {
        // Logger::inst().set_printing_enabled(true);
        auto file =
            make_test_code_file("let a: (i32, bool, f64) = (1, 2, true)");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::LetTypeMismatch);
    }

    SECTION("Tuple access valid") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file(
            "let a = (1, 2.1, true) let b: i32 = a.0 let c: f64 = a.1 "
            "let d: bool = a.2"
        );
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Tuple access invalid index") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = (1, 2.1, true) let b = a.3");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::TupleIndexOutOfBounds);
    }

    SECTION("Tuple access invalid right side") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = (1, 2.1, true) let b = a.x");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::InvalidTupleAccess);
    }

    SECTION("Tuple access as valid lvalue") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file(
            "let var a = (1, 2.1, true) a.0 = 2 a.1 = 3.5 a.2 = false"
        );
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Tuple access assign to immutable") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a = (1, 2.1, true) a.0 = 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::AssignToImmutable);
    }

    SECTION("Tuple access out of bounds 1") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let var a = (1, 2.1, true) a.3 = 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::TupleIndexOutOfBounds);
    }

    SECTION("Tuple access out of bounds 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let var a = () a.0 = 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::TupleIndexOutOfBounds);
    }

    lexer.reset();
    parser.reset();
    Logger::inst().reset();
}

TEST_CASE("Local conditional expressions", "[checker]") {
    Lexer lexer;
    Parser parser;
    GlobalChecker global_checker;
    LocalChecker local_checker;
    Logger::inst().set_printing_enabled(false);

    SECTION("Valid conditional expression 1") {
        Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("if true { 1 } else { false }");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Valid conditional expression 2") {
        Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file(R"(
        if true:
            1
        else:
            2
        )");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Valid conditional expression 3") {
        Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a: i32 = if true then 1 else 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Valid conditional expression 4") {
        Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("if true {}");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Valid if else if expression 1") {
        Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file(R"(
        if false:
            1
        else if true:
            2
        else:
            3
        )");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Valid if else if expression 2") {
        Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file(R"(
        if false then 1 else if true then 2 else 3
        )");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Conditional condition not bool") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("if 1 { 1 } else { 2 }");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::ConditionNotBool);
    }

    SECTION("Conditional branch type mismatch 1") {
        // Logger::inst().set_printing_enabled(true);
        auto file =
            make_test_code_file("if true { yield 1 } else { yield false }");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::ConditionalBranchTypeMismatch);
    }

    SECTION("Conditional branch type mismatch 2") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file(R"(
        if true:
            yield 1
        else:
            yield false
        )");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::ConditionalBranchTypeMismatch);
    }

    SECTION("Conditional branch type mismatch 3") {
        // Logger::inst().set_printing_enabled(true);
        auto file =
            make_test_code_file("let a: i32 = if true then 1 else false");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::ConditionalBranchTypeMismatch);
    }

    SECTION("Conditional branch type mismatch 4") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("if true then 1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::ConditionalBranchTypeMismatch);
    }

    SECTION("If else if branch type mismatch") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file(R"(
        if false:
            1
        else if true:
            yield 2
        else:
            3
        )");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::ConditionalBranchTypeMismatch);
    }

    SECTION("Let type mismatch with conditional") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a: bool = if true then 1 else 2");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::LetTypeMismatch);
    }

    SECTION("Yield type mismatch with conditional") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file(
            "if true { yield 1 yield 2.0 } else { yield 3 }"
        );
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::YieldTypeMismatch);
    }

    SECTION("Conditional with many errors") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file(R"(
        if 42:
            let a: bool = 1
            1 + 2.0
            yield a
        else:
            yield (true, a + b)
        )");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(!Logger::inst().get_errors().empty());
    }

    lexer.reset();
    parser.reset();
    Logger::inst().reset();
}
