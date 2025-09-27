#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "frontend/global_checker.h"
#include "frontend/lexer.h"
#include "frontend/local_checker.h"
#include "frontend/parser.h"
#include "frontend/utils/ast_node.h"
#include "shared/code_file.h"
#include "shared/logger.h"
#include "shared/token.h"
#include "test_utils.h"

void run_checker_test(
    std::string_view src_code, std::optional<Err> expected_error = std::nullopt,
    std::optional<bool> print_errors = std::nullopt
) {
    // If there is no expected error, we enable printing to look for unexpected
    // errors.
    Logger::inst().set_printing_enabled(
        print_errors.value_or(!expected_error.has_value())
    );

    auto context = std::make_unique<FrontendContext>();
    auto file = make_test_code_file(src_code);
    Lexer lexer;
    lexer.scan(context, file);
    Parser parser;
    parser.parse(context);
    GlobalChecker global_checker;
    global_checker.check(context);
    LocalChecker local_checker;
    local_checker.check(context);

    if (expected_error.has_value()) {
        auto& errors = Logger::inst().get_errors();
        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == expected_error.value());
    }
    else {
        CHECK(Logger::inst().get_errors().empty());
    }

    lexer.reset();
    parser.reset();
    context->reset();
    Logger::inst().reset();
}

TEST_CASE("Local variable declarations", "[checker]") {
    SECTION("Valid local variable declarations") {
        run_checker_test("let a = 1");
    }

    SECTION("Let type mismatch 1") {
        run_checker_test("let a: i32 = true", Err::LetTypeMismatch);
    }

    SECTION("Let type mismatch 2") {
        run_checker_test("let a: i32 = 1.0", Err::LetTypeMismatch);
    }

    SECTION("Let type mismatch 3") {
        run_checker_test("let a = true let b: i32 = a", Err::LetTypeMismatch);
    }

    SECTION("Undeclared identifier") {
        run_checker_test("let a = b", Err::UndeclaredName);
    }

    SECTION("Immutable variables") {
        run_checker_test("let a = 1 let b = 2 a = b", Err::AssignToImmutable);
    }

    SECTION("Variable name conflict") {
        run_checker_test("let a = 1 let a = 2", Err::NameAlreadyExists);
    }

    SECTION("Primitive type name conflict") {
        run_checker_test("let i32 = 1", Err::NameIsReserved);
    }
}

TEST_CASE("Local unary expressions", "[checker]") {
    SECTION("Valid unary expression 1") {
        run_checker_test("let a = -1");
    }

    SECTION("Valid unary expression 2") {
        run_checker_test("let a = not true");
    }

    SECTION("Unary type mismatch 1") {
        run_checker_test("let a = -true", Err::NoOperatorOverload);
    }

    SECTION("Unary type mismatch 2") {
        run_checker_test("let a = not 1", Err::NoOperatorOverload);
    }
}

TEST_CASE("Local binary expressions", "[checker]") {
    SECTION("Valid binary expressions 1") {
        run_checker_test("let a = 1 + 2");
    }

    SECTION("Valid binary expressions 2") {
        run_checker_test("let a = 1.0 + 2.0");
    }

    SECTION("Binary type mismatch 1") {
        run_checker_test("let a = 1 + true", Err::NoOperatorOverload);
    }

    SECTION("Binary type mismatch 2") {
        run_checker_test("let a = true + 1", Err::NoOperatorOverload);
    }

    SECTION("Binary type mismatch 3") {
        run_checker_test("let a = true + false", Err::NoOperatorOverload);
    }
}

TEST_CASE("Local logical expressions", "[checker]") {
    SECTION("Valid logical expressions 1") {
        run_checker_test("let a = true and false");
    }

    SECTION("Valid logical expressions 2") {
        run_checker_test("let a = true or false and false");
    }

    SECTION("Valid logical expressions 3") {
        run_checker_test("let a = true or not true");
    }

    SECTION("Logical type mismatch 1") {
        run_checker_test("let a = 1 and true", Err::NoOperatorOverload);
    }

    SECTION("Logical type mismatch 2") {
        run_checker_test("let a = true and 1", Err::NoOperatorOverload);
    }

    SECTION("Logical type mismatch 3") {
        run_checker_test("let a: i32 = true and true", Err::LetTypeMismatch);
    }
}

TEST_CASE("Local assignment expressions", "[checker]") {
    SECTION("Valid assignment expressions") {
        run_checker_test("let var a = 1 a = 2");
    }

    SECTION("Assignment type mismatch 1") {
        run_checker_test(
            "let var a: i32 = 1 a = true",
            Err::AssignmentTypeMismatch
        );
    }

    SECTION("Assignment type mismatch 2") {
        run_checker_test(
            "let var a: i32 = 1 a = 1.0",
            Err::AssignmentTypeMismatch
        );
    }

    SECTION("Assignment not an lvalue 1") {
        run_checker_test("1 = 2", Err::NotAPossibleLValue);
    }

    SECTION("Assignment not an lvalue 2") {
        run_checker_test("(1 + 1) = 2", Err::NotAPossibleLValue);
    }

    SECTION("Assignment not an lvalue 3") {
        run_checker_test("let var a = 1 (a = 1) = 2", Err::NotAPossibleLValue);
    }

    SECTION("Assignment not an lvalue 4") {
        run_checker_test("let var a = 1; -a = 2", Err::NotAPossibleLValue);
    }
}

TEST_CASE("Local print statements", "[checker]") {
    SECTION("Print hello world") {
        run_checker_test("printout \"Hello, World!\"");
    }
}

TEST_CASE("Local block expressions", "[checker]") {
    SECTION("Valid block expression") {
        run_checker_test("block { let a = 1 printout a }");
    }

    SECTION("Block expression yield 1") {
        run_checker_test("block { let a = 1 printout a yield a }");
    }

    SECTION("Block expression yield 2") {
        run_checker_test("let var a = 1 a = block { yield 2 }");
    }

    SECTION("Block expression nested yield") {
        run_checker_test("let var a = 1 a = block { yield block { yield 2 } }");
    }

    SECTION("Yield outside local scope") {
        run_checker_test("yield 1", Err::YieldOutsideLocalScope);
    }

    SECTION("Incompatible yield types") {
        run_checker_test(
            "block { yield 1 yield true }",
            Err::YieldTypeMismatch
        );
    }

    SECTION("Block without yield") {
        run_checker_test(
            "let var a = 1 a = block { let b = 1 }",
            Err::AssignmentTypeMismatch
        );
    }
}

TEST_CASE("Local tuple expressions", "[checker]") {
    SECTION("Valid tuple expression 1") {
        run_checker_test("let a = (1, 2.1, true)");
    }

    SECTION("Valid tuple expression 2") {
        run_checker_test("let a: (i32, f64, bool) = (1, 2.1, true)");
    }

    SECTION("Tuple expression type mismatch") {
        run_checker_test(
            "let a: (i32, bool, f64) = (1, 2, true)",
            Err::LetTypeMismatch
        );
    }

    SECTION("Tuple access valid") {
        run_checker_test(
            "let a = (1, 2.1, true) let b: i32 = a.0 let c: f64 = a.1 "
            "let d: bool = a.2"
        );
    }

    SECTION("Tuple access invalid index") {
        run_checker_test(
            "let a = (1, 2.1, true) let b = a.3",
            Err::TupleIndexOutOfBounds
        );
    }

    SECTION("Tuple access invalid right side") {
        run_checker_test(
            "let a = (1, 2.1, true) let b = a.x",
            Err::InvalidTupleAccess
        );
    }

    SECTION("Tuple access as valid lvalue") {
        run_checker_test(
            "let var a = (1, 2.1, true) a.0 = 2 a.1 = 3.5 a.2 = false"
        );
    }

    SECTION("Tuple access assign to immutable") {
        run_checker_test(
            "let a = (1, 2.1, true) a.0 = 2",
            Err::AssignToImmutable
        );
    }

    SECTION("Tuple access out of bounds 1") {
        run_checker_test(
            "let var a = (1, 2.1, true) a.3 = 2",
            Err::TupleIndexOutOfBounds
        );
    }

    SECTION("Tuple access out of bounds 2") {
        run_checker_test("let var a = () a.0 = 2", Err::TupleIndexOutOfBounds);
    }
}

TEST_CASE("Local conditional expressions", "[checker]") {
    SECTION("Valid conditional expression 1") {
        run_checker_test("if true { 1 } else { false }");
    }

    SECTION("Valid conditional expression 2") {
        run_checker_test(R"(
        if true:
            1
        else:
            2
        )");
    }

    SECTION("Valid conditional expression 3") {
        run_checker_test("let a: i32 = if true then 1 else 2");
    }

    SECTION("Valid conditional expression 4") {
        run_checker_test("if true {}");
    }

    SECTION("Valid if else if expression 1") {
        run_checker_test(R"(
        if false:
            1
        else if true:
            2
        else:
            3
        )");
    }

    SECTION("Valid if else if expression 2") {
        run_checker_test(R"(
        if false then 1 else if true then 2 else 3
        )");
    }

    SECTION("Conditional condition not bool") {
        run_checker_test("if 1 { 1 } else { 2 }", Err::ConditionNotBool);
    }

    SECTION("Conditional branch type mismatch 1") {
        run_checker_test(
            "if true { yield 1 } else { yield false }",
            Err::ConditionalBranchTypeMismatch
        );
    }

    SECTION("Conditional branch type mismatch 2") {
        run_checker_test(
            R"(
        if true:
            yield 1
        else:
            yield false
        )",
            Err::ConditionalBranchTypeMismatch
        );
    }

    SECTION("Conditional branch type mismatch 3") {
        run_checker_test(
            "let a: i32 = if true then 1 else false",
            Err::ConditionalBranchTypeMismatch
        );
    }

    SECTION("Conditional branch type mismatch 4") {
        run_checker_test("if true then 1", Err::ConditionalBranchTypeMismatch);
    }

    SECTION("If else if branch type mismatch") {
        run_checker_test(
            R"(
        if false:
            1
        else if true:
            yield 2
        else:
            3
        )",
            Err::ConditionalBranchTypeMismatch
        );
    }

    SECTION("Let type mismatch with conditional") {
        run_checker_test(
            "let a: bool = if true then 1 else 2",
            Err::LetTypeMismatch
        );
    }

    SECTION("Yield type mismatch with conditional") {
        run_checker_test(
            "if true { yield 1 yield 2.0 } else { yield 3 }",
            Err::YieldTypeMismatch
        );
    }

    SECTION("Conditional with many errors") {
        run_checker_test(
            R"(
        if 42:
            let a: bool = 1
            1 + 2.0
            yield a
        else:
            yield (true, a + b)
        )",
            Err::ConditionNotBool
        );
    }
}
