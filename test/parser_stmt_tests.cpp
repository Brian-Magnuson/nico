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
    auto context = std::make_unique<FrontendContext>();
    auto file = make_test_code_file(src_code);
    Lexer::scan(context, file);
    Parser::parse(context);
    AstPrinter printer;
    CHECK(printer.stmts_to_strings(context->stmts) == expected);

    context->reset();
    Logger::inst().reset();
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
    Logger::inst().set_printing_enabled(print_errors);

    auto context = std::make_unique<FrontendContext>();
    auto file = make_test_code_file(src_code);
    Lexer::scan(context, file);
    Parser::parse(context);

    auto& errors = Logger::inst().get_errors();
    REQUIRE(errors.size() >= 1);
    CHECK(errors.at(0) == expected_error);

    context->reset();
    Logger::inst().reset();
}

// MARK: Parser stmt tests

TEST_CASE("Parser let statements", "[parser]") {
    SECTION("Let statements 1") {
        run_parser_stmt_test(
            "let a = 1",
            {"(stmt:let a (lit 1))", "(stmt:eof)"}
        );
    }

    SECTION("Let statements 2") {
        run_parser_stmt_test(
            "let var a = 1",
            {"(stmt:let var a (lit 1))", "(stmt:eof)"}
        );
    }

    SECTION("Let statements 3") {
        run_parser_stmt_test(
            "let a: i32 = 1",
            {"(stmt:let a i32 (lit 1))", "(stmt:eof)"}
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
            {"(stmt:let a i32)", "(stmt:let b (lit 2))", "(stmt:eof)"}
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
            {"(stmt:print (lit 1))", "(stmt:eof)"}
        );
    }

    SECTION("Print statements 2") {
        run_parser_stmt_test(
            "printout 1, 2",
            {"(stmt:print (lit 1) (lit 2))", "(stmt:eof)"}
        );
    }

    SECTION("Print statements 3") {
        run_parser_stmt_test(
            "printout 1, 2, 3",
            {"(stmt:print (lit 1) (lit 2) (lit 3))", "(stmt:eof)"}
        );
    }

    SECTION("Print statements 4") {
        run_parser_stmt_test(
            "printout 1 / 2",
            {"(stmt:print (binary / (lit 1) (lit 2)))", "(stmt:eof)"}
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
            {"(expr (binary - (lit 1) (lit 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Separated unary statements") {
        run_parser_stmt_test(
            "1; - 2",
            {"(expr (lit 1))", "(expr (unary - (lit 2)))", "(stmt:eof)"}
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

    SECTION("Let improper type") {
        run_parser_stmt_error_test("let a: 1 = 1", Err::NotAType);
    }

    SECTION("Let without type or value") {
        run_parser_stmt_error_test("let a", Err::LetWithoutTypeOrValue);
    }
}
