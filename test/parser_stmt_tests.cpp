#include <memory>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "nico/frontend/components/lexer.h"
#include "nico/frontend/components/parser.h"
#include "nico/frontend/utils/ast_node.h"
#include "nico/shared/code_file.h"
#include "nico/shared/logger.h"
#include "nico/shared/token.h"

#include "ast_printer.h"
#include "test_utils.h"

using nico::Err;

/**
 * @brief Run a parser statement test with the given source code and expected
 * AST strings.
 *
 * @param src_code The source code to test.
 * @param expected The expected AST strings.
 */
void run_parser_stmt_test(
    std::string_view src_code, const std::vector<std::string>& expected
) {
    auto context = std::make_unique<nico::FrontendContext>();
    auto file = nico::make_test_code_file(src_code);
    nico::Lexer::scan(context, file);
    nico::Parser::parse(context);
    nico::AstPrinter printer;
    CHECK(printer.stmts_to_strings(context->stmts) == expected);

    context->reset();
    nico::Logger::inst().reset();
}

/**
 * @brief Run a parser statement error test with the given source code and
 * expected error.
 *
 * Erroneous input can potentially produce multiple errors.
 * Only the first error is checked by this test.
 *
 * @param src_code The source code to test.
 * @param expected_error The expected error.
 * @param print_errors Whether to print errors. Defaults to false.
 */
void run_parser_stmt_error_test(
    std::string_view src_code, Err expected_error, bool print_errors = false
) {
    nico::Logger::inst().set_printing_enabled(print_errors);

    auto context = std::make_unique<nico::FrontendContext>();
    auto file = nico::make_test_code_file(src_code);
    nico::Lexer::scan(context, file);
    nico::Parser::parse(context);

    auto& errors = nico::Logger::inst().get_errors();
    REQUIRE(errors.size() >= 1);
    CHECK(errors.at(0) == expected_error);

    context->reset();
    nico::Logger::inst().reset();
}

// MARK: Parser stmt tests

TEST_CASE("Parser let statements", "[parser]") {
    SECTION("Let statements 1") {
        run_parser_stmt_test(
            "let a = 1",
            {"(stmt:let a (lit i32 1))", "(stmt:eof)"}
        );
    }

    SECTION("Let statements 2") {
        run_parser_stmt_test(
            "let var a = 1",
            {"(stmt:let var a (lit i32 1))", "(stmt:eof)"}
        );
    }

    SECTION("Let statements 3") {
        run_parser_stmt_test(
            "let a: i32 = 1",
            {"(stmt:let a i32 (lit i32 1))", "(stmt:eof)"}
        );
    }

    SECTION("Let statements 4") {
        run_parser_stmt_test(
            "let a: i32 let b: f64",
            {"(stmt:let a i32)", "(stmt:let b f64)", "(stmt:eof)"}
        );
    }

    SECTION("Let statements 5") {
        run_parser_stmt_test(
            "let a: Vector2D",
            {"(stmt:let a Vector2D)", "(stmt:eof)"}
        );
    }

    SECTION("Let statements 6") {
        run_parser_stmt_test(
            "let a: i32 let b = 2",
            {"(stmt:let a i32)", "(stmt:let b (lit i32 2))", "(stmt:eof)"}
        );
    }

    SECTION("Let statements pointers 1") {
        run_parser_stmt_test(
            "let var a: @i32 = 10",
            {"(stmt:let var a @i32 (lit i32 10))", "(stmt:eof)"}
        );
    }

    SECTION("Let statements pointers 2") {
        run_parser_stmt_test(
            "let var ptr: var@f64",
            {"(stmt:let var ptr var@f64)", "(stmt:eof)"}
        );
    }

    SECTION("Let statements pointers 3") {
        run_parser_stmt_test(
            "let data: nullptr = nullptr",
            {"(stmt:let data nullptr (lit nullptr))", "(stmt:eof)"}
        );
    }

    SECTION("Let statements typeof 1") {
        run_parser_stmt_test(
            "let x: i32 let size: typeof(x)",
            {"(stmt:let x i32)",
             "(stmt:let size typeof(<expr@1:29>))",
             "(stmt:eof)"}
        );
    }

    SECTION("Let statements typeof 2") {
        run_parser_stmt_test(
            "let x: i32 let len: typeof(x + 1)",
            {"(stmt:let x i32)",
             "(stmt:let len typeof(<expr@1:30>))",
             "(stmt:eof)"}
        );
    }

    SECTION("Let statements arrays 1") {
        run_parser_stmt_test(
            "let arr: [i32; 10]",
            {"(stmt:let arr [i32; 10])", "(stmt:eof)"}
        );
    }

    SECTION("Let statements arrays 2") {
        run_parser_stmt_test(
            "let matrix: [[f64; 5]; 10]",
            {"(stmt:let matrix [[f64; 5]; 10])", "(stmt:eof)"}
        );
    }

    SECTION("Let statements arrays 3") {
        run_parser_stmt_test(
            "let buffer: [u8; ?]",
            {"(stmt:let buffer [u8; ?])", "(stmt:eof)"}
        );
    }

    SECTION("Let statements arrays 4") {
        run_parser_stmt_test(
            "let var data: var@[i32; 20]",
            {"(stmt:let var data var@[i32; 20])", "(stmt:eof)"}
        );
    }

    SECTION("Let statements arrays 5") {
        run_parser_stmt_test(
            "let var empty: [f64; 0]",
            {"(stmt:let var empty [f64; 0])", "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser function statements", "[parser]") {
    SECTION("Func statement 1") {
        run_parser_stmt_test(
            "func f1() {}",
            {"(stmt:func f1 () => (block))", "(stmt:eof)"}
        );
    }

    SECTION("Func statement 2") {
        run_parser_stmt_test(
            "func f2():\n    pass",
            {"(stmt:func f2 () => (block (stmt:pass)))", "(stmt:eof)"}
        );
    }

    SECTION("Func statement 3") {
        run_parser_stmt_test(
            "func f3() -> i32 => 10",
            {"(stmt:func f3 i32 () => (block (stmt:yield => (lit i32 10))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Func statement 4") {
        run_parser_stmt_test(
            "func f4(x: i32) -> i32 => x + 1",
            {"(stmt:func f4 i32 (x i32) => (block (stmt:yield => (binary + "
             "(nameref x) (lit i32 1)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Func statement 5") {
        run_parser_stmt_test(
            "func f5(var y: f64) { y += 1.0 }",
            {"(stmt:func f5 (var y f64) => (block (expr (assign (nameref y) "
             "(binary + (nameref y) (lit f64 1.000000))))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Func statement 6") {
        run_parser_stmt_test(
            "func f6(a: i32 = 0) -> i32 => a * 2",
            {"(stmt:func f6 i32 (a i32 (lit i32 0)) => (block (stmt:yield => "
             "(binary * (nameref a) (lit i32 2)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Func statement 7") {
        run_parser_stmt_test(
            "func f7() { pass pass pass }",
            {"(stmt:func f7 () => (block (stmt:pass) (stmt:pass) (stmt:pass)))",
             "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser tuple annotations", "[parser]") {
    SECTION("Tuple annotation 1") {
        run_parser_stmt_test(
            "let a: (i32)",
            {"(stmt:let a (i32))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple annotation 2") {
        run_parser_stmt_test(
            "let a: (i32, f64, String)",
            {"(stmt:let a (i32, f64, String))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple annotation 3") {
        run_parser_stmt_test(
            "let a: (i32,)",
            {"(stmt:let a (i32))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple annotation 4") {
        run_parser_stmt_test(
            "let a: ((i32, f64), String)",
            {"(stmt:let a ((i32, f64), String))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple annotation 5") {
        run_parser_stmt_test("let a: ()", {"(stmt:let a ())", "(stmt:eof)"});
    }
}

TEST_CASE("Parser print statements", "[parser]") {
    SECTION("Print statements 1") {
        run_parser_stmt_test(
            "printout 1",
            {"(stmt:print (lit i32 1))", "(stmt:eof)"}
        );
    }

    SECTION("Print statements 2") {
        run_parser_stmt_test(
            "printout 1, 2",
            {"(stmt:print (lit i32 1) (lit i32 2))", "(stmt:eof)"}
        );
    }

    SECTION("Print statements 3") {
        run_parser_stmt_test(
            "printout 1, 2, 3",
            {"(stmt:print (lit i32 1) (lit i32 2) (lit i32 3))", "(stmt:eof)"}
        );
    }

    SECTION("Print statements 4") {
        run_parser_stmt_test(
            "printout 1 / 2",
            {"(stmt:print (binary / (lit i32 1) (lit i32 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Print statements 5") {
        run_parser_stmt_test(
            "printout \"Hello, World!\" printout \"Goodbye, World!\"",
            {"(stmt:print (lit \"Hello, World!\"))",
             "(stmt:print (lit \"Goodbye, World!\"))",
             "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser statement separation", "[parser]") {
    SECTION("Unseparated binary statement") {
        run_parser_stmt_test(
            "1 - 2",
            {"(expr (binary - (lit i32 1) (lit i32 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Separated unary statements") {
        run_parser_stmt_test(
            "1; - 2",
            {"(expr (lit i32 1))", "(expr (lit i32 -2))", "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser non-declaring statements", "[parser]") {
    SECTION("Yield statement") {
        run_parser_stmt_test(
            "yield 42",
            {"(stmt:yield yield (lit i32 42))", "(stmt:eof)"}
        );
    }

    SECTION("Continue statement") {
        run_parser_stmt_test("continue", {"(stmt:continue)", "(stmt:eof)"});
    }

    SECTION("Break statement") {
        run_parser_stmt_test(
            "break 10",
            {"(stmt:yield break (lit i32 10))", "(stmt:eof)"}
        );
    }
}

// MARK: Error tests

TEST_CASE("Parser let stmt errors", "[parser]") {
    SECTION("Let missing identifier 1") {
        run_parser_stmt_error_test("let", Err::NotAnIdentifier);
    }

    SECTION("Let missing identifier 2") {
        run_parser_stmt_error_test("let var", Err::NotAnIdentifier);
    }

    SECTION("Let without type or value") {
        run_parser_stmt_error_test("let a", Err::LetWithoutTypeOrValue);
    }
}

TEST_CASE("Parser func stmt errors", "[parser]") {
    SECTION("Func missing identifier") {
        run_parser_stmt_error_test("func {}", Err::NotAnIdentifier);
    }

    SECTION("Func missing opening parenthesis") {
        run_parser_stmt_error_test("func f1 {}", Err::FuncWithoutOpeningParen);
    }

    SECTION("Func missing annotation after arrow") {
        run_parser_stmt_error_test("func f() -> {}", Err::NotAType);
    }

    SECTION("Func missing arrow or block") {
        run_parser_stmt_error_test("func f() 10", Err::FuncWithoutArrowOrBlock);
    }

    SECTION("Func missing parameter type") {
        run_parser_stmt_error_test("func f(x) {}", Err::NotAType);
    }

    SECTION("Func missing parameter identifier") {
        run_parser_stmt_error_test("func f(: i32) {}", Err::NotAnIdentifier);
    }

    SECTION("Func parameter missing expr after equals") {
        run_parser_stmt_error_test(
            "func f(x: i32 = ) {}",
            Err::NotAnExpression
        );
    }
}

TEST_CASE("Parser annotation errors", "[parser]") {
    SECTION("Let improper type") {
        run_parser_stmt_error_test("let a: 1 = 1", Err::NotAType);
    }

    SECTION("Unexpected var in annotation") {
        run_parser_stmt_error_test(
            "let a: var i32 = 1",
            Err::UnexpectedVarInAnnotation
        );
    }

    SECTION("Typeof missing opening parenthesis") {
        run_parser_stmt_error_test(
            "let a: typeof x = 1",
            Err::TypeofWithoutOpeningParen
        );
    }
}

TEST_CASE("Parser unexpected token errors", "[parser]") {
    SECTION("Comma in typeof annotation") {
        run_parser_stmt_error_test(
            "let a: typeof(x,) = 1",
            Err::UnexpectedToken
        );
    }

    SECTION("Semicolon in grouping") {
        run_parser_stmt_error_test("(1;)", Err::UnexpectedToken);
    }

    SECTION("Semicolon in func declaration parameters") {
        run_parser_stmt_error_test(
            "func f(a: i32; b: i32) {}",
            Err::UnexpectedToken
        );
    }

    SECTION("Semicolon in func call arguments") {
        run_parser_stmt_error_test("f(1; 2)", Err::UnexpectedToken);
    }

    SECTION("Semicolon in tuple annotation") {
        run_parser_stmt_error_test("let a: (i32; f64)", Err::UnexpectedToken);
    }
}

TEST_CASE("Parser yield without expression", "[parser]") {
    SECTION("Yield missing expression") {
        run_parser_stmt_error_test("yield", Err::NotAnExpression);
    }

    SECTION("Break missing expression") {
        run_parser_stmt_error_test("break", Err::NotAnExpression);
    }
}
