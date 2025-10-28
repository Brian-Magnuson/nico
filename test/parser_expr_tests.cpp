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
 * @brief Run a parser expression test with the given source code and expected
 * AST string.
 *
 * The AST string is generated using the AstPrinter visitor.
 *
 * @param src_code The source code to test.
 * @param expected The expected AST string.
 */
void run_parser_expr_test(
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
 * @brief Run a parser expression error test with the given source code and
 * expected error.
 *
 * Erroneous input can potentially produce multiple errors.
 * Only the first error is checked by this test.
 *
 * @param src_code The source code to test.
 * @param expected_error The expected error.
 * @param print_errors Whether to print errors. Defaults to false.
 */
void run_parser_expr_error_test(
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

// MARK: Parser expr tests

TEST_CASE("Parser basic", "[parser]") {
    SECTION("Basic 1") {
        run_parser_expr_test("basic", {"(expr (nameref basic))", "(stmt:eof)"});
    }

    SECTION("Basic 2") {
        run_parser_expr_test("123", {"(expr (lit 123))", "(stmt:eof)"});
    }

    SECTION("Nullptr") {
        run_parser_expr_test("nullptr", {"(expr (lit nullptr))", "(stmt:eof)"});
    }
}

TEST_CASE("Parser expressions", "[parser]") {
    SECTION("Unary 1") {
        run_parser_expr_test(
            "-123",
            {"(expr (unary - (lit 123)))", "(stmt:eof)"}
        );
    }

    SECTION("Unary 2") {
        run_parser_expr_test(
            "not true !false",
            {"(expr (unary not (lit true)))",
             "(expr (unary ! (lit false)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Address and Deref") {
        run_parser_expr_test(
            "@x; var@x; &x; var&x; ^p",
            {"(expr (address @ (nameref x)))",
             "(expr (address var@ (nameref x)))",
             "(expr (address & (nameref x)))",
             "(expr (address var& (nameref x)))",
             "(expr (deref (nameref p)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Binary 1") {
        run_parser_expr_test(
            "1 + 2",
            {"(expr (binary + (lit 1) (lit 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Binary 2") {
        run_parser_expr_test(
            "1 + 2 * 3",
            {"(expr (binary + (lit 1) (binary * (lit 2) (lit 3))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Binary 3") {
        run_parser_expr_test(
            "1 * 2 1 + 2",
            {"(expr (binary * (lit 1) (lit 2)))",
             "(expr (binary + (lit 1) (lit 2)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Binary 4") {
        run_parser_expr_test(
            "1 * -2 + -3",
            {"(expr (binary + (binary * (lit 1) (unary - (lit 2))) (unary - "
             "(lit 3))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Comparison 1") {
        run_parser_expr_test(
            "1 < 2",
            {"(expr (binary < (lit 1) (lit 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Comparison 2") {
        run_parser_expr_test(
            "1 + 2 >= 3 * 4",
            {"(expr (binary >= (binary + (lit 1) (lit 2)) (binary * (lit 3) "
             "(lit 4))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Equality 1") {
        run_parser_expr_test(
            "a == b",
            {"(expr (binary == (nameref a) (nameref b)))", "(stmt:eof)"}
        );
    }

    SECTION("Equality 2") {
        run_parser_expr_test(
            "a != b + c",
            {"(expr (binary != (nameref a) (binary + (nameref b) (nameref "
             "c))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Equality 3") {
        run_parser_expr_test(
            "a < b == c >= d",
            {"(expr (binary == (binary < (nameref a) (nameref b)) (binary >= "
             "(nameref c) (nameref d))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Logical 1") {
        run_parser_expr_test(
            "true and false",
            {"(expr (logical and (lit true) (lit false)))", "(stmt:eof)"}
        );
    }

    SECTION("Logical 2") {
        run_parser_expr_test(
            "true or false and false",
            {"(expr (logical or (lit true) (logical and (lit false) (lit "
             "false))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Logical 3") {
        run_parser_expr_test(
            "true or not true",
            {"(expr (logical or (lit true) (unary not (lit true))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Assignment 1") {
        run_parser_expr_test(
            "a = 1",
            {"(expr (assign (nameref a) (lit 1)))", "(stmt:eof)"}
        );
    }

    SECTION("Assignment 2") {
        run_parser_expr_test(
            "a = b = c",
            {"(expr (assign (nameref a) (assign (nameref b) (nameref c))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Compound assignment 1") {
        run_parser_expr_test(
            "a += b",
            {"(expr (assign (nameref a) (binary + (nameref a) (nameref b))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Compound assignment 2") {
        run_parser_expr_test(
            "a *= b + c",
            {"(expr (assign (nameref a) (binary * (nameref a) (binary + "
             "(nameref b) (nameref c)))))",
             "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser groupings and tuples", "[parser]") {
    SECTION("Grouping 1") {
        run_parser_expr_test("(1)", {"(expr (lit 1))", "(stmt:eof)"});
    }

    SECTION("Grouping 2") {
        run_parser_expr_test(
            "(1 + 2)",
            {"(expr (binary + (lit 1) (lit 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Grouping 3") {
        run_parser_expr_test(
            "(1 + 2) * 3",
            {"(expr (binary * (binary + (lit 1) (lit 2)) (lit 3)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Tuple unit") {
        run_parser_expr_test("()", {"(expr (tuple))", "(stmt:eof)"});
    }

    SECTION("Tuple 1") {
        run_parser_expr_test("(1,)", {"(expr (tuple (lit 1)))", "(stmt:eof)"});
    }

    SECTION("Tuple 2") {
        run_parser_expr_test(
            "(1, 2)",
            {"(expr (tuple (lit 1) (lit 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple 3") {
        run_parser_expr_test(
            "(1, 2, 3,)",
            {"(expr (tuple (lit 1) (lit 2) (lit 3)))", "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser blocks", "[parser]") {
    SECTION("Braced block 1") {
        run_parser_expr_test(
            "block { 123 }",
            {"(expr (block (expr (lit 123))))", "(stmt:eof)"}
        );
    }

    SECTION("Braced block 2") {
        run_parser_expr_test(
            "block { let a = 1 printout a }",
            {"(expr (block (stmt:let a (lit 1)) (stmt:print (nameref a))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Braced block 3") {
        run_parser_expr_test(
            "block { 123 } 456",
            {"(expr (block (expr (lit 123))))",
             "(expr (lit 456))",
             "(stmt:eof)"}
        );
    }

    SECTION("Indented block 1") {
        run_parser_expr_test(
            "block:\n    123\n",
            {"(expr (block (expr (lit 123))))", "(stmt:eof)"}
        );
    }

    SECTION("Indented block 2") {
        run_parser_expr_test(
            "block:\n    let a = 1\n    printout a\n",
            {"(expr (block (stmt:let a (lit 1)) (stmt:print (nameref a))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Indented block 3") {
        run_parser_expr_test(
            "block:\n    123\n456\n",
            {"(expr (block (expr (lit 123))))",
             "(expr (lit 456))",
             "(stmt:eof)"}
        );
    }

    SECTION("Block with pass") {
        run_parser_expr_test(
            "block:\n    pass\n",
            {"(expr (block (stmt:pass)))", "(stmt:eof)"}
        );
    }

    SECTION("Block with comments") {
        run_parser_expr_test(
            "    block:\n        // comment\n",
            {"(expr (block))", "(stmt:eof)"}
        );
    }

    SECTION("Empty block") {
        run_parser_expr_test("block { }", {"(expr (block))", "(stmt:eof)"});
    }

    SECTION("Braced unsafe block") {
        run_parser_expr_test(
            "unsafe { 123 }",
            {"(expr (block unsafe (expr (lit 123))))", "(stmt:eof)"}
        );
    }

    SECTION("Indented unsafe block") {
        run_parser_expr_test(
            "unsafe:\n    123\n",
            {"(expr (block unsafe (expr (lit 123))))", "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser tuples", "[parser]") {
    SECTION("Tuple 1") {
        run_parser_expr_test("(1,)", {"(expr (tuple (lit 1)))", "(stmt:eof)"});
    }

    SECTION("Tuple 2") {
        run_parser_expr_test(
            "(1, 2)",
            {"(expr (tuple (lit 1) (lit 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple 3") {
        run_parser_expr_test(
            "(1, 2, 3,)",
            {"(expr (tuple (lit 1) (lit 2) (lit 3)))", "(stmt:eof)"}
        );
    }

    SECTION("Not a tuple") {
        run_parser_expr_test("(1)", {"(expr (lit 1))", "(stmt:eof)"});
    }

    SECTION("Empty tuple") {
        run_parser_expr_test("()", {"(expr (tuple))", "(stmt:eof)"});
    }
}

TEST_CASE("Parser dot access", "[parser]") {
    SECTION("Dot access 1") {
        run_parser_expr_test(
            "a.b",
            {"(expr (access . (nameref a) b))", "(stmt:eof)"}
        );
    }

    SECTION("Dot access 2") {
        run_parser_expr_test(
            "a.b.c",
            {"(expr (access . (access . (nameref a) b) c))", "(stmt:eof)"}
        );
    }

    SECTION("Dot access 3") {
        run_parser_expr_test(
            "a.b.c.d",
            {"(expr (access . (access . (access . (nameref a) b) c) d))",
             "(stmt:eof)"}
        );
    }

    SECTION("Dot access with integer") {
        run_parser_expr_test(
            "a.b.0",
            {"(expr (access . (access . (nameref a) b) 0))", "(stmt:eof)"}
        );
    }

    SECTION("Dot access with multiple integers") {
        run_parser_expr_test(
            "a.0.1",
            {"(expr (access . (access . (nameref a) 0) 1))", "(stmt:eof)"}
        );
    }

    SECTION("Dot access with many integers") {
        run_parser_expr_test(
            "a.0.1.2",
            {"(expr (access . (access . (access . (nameref a) 0) 1) 2))",
             "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser if expressions", "[parser]") {
    SECTION("If expression 1") {
        run_parser_expr_test(
            "if true then 123\n",
            {"(expr (if (lit true) then (lit 123) else (tuple)))", "(stmt:eof)"}
        );
    }

    SECTION("If expression 2") {
        run_parser_expr_test(
            "if true:\n    123\n",
            {"(expr (if (lit true) then (block (expr (lit 123))) else "
             "(tuple)))",
             "(stmt:eof)"}
        );
    }

    SECTION("If expression 3") {
        run_parser_expr_test(
            "if true { 123 }\n",
            {"(expr (if (lit true) then (block (expr (lit 123))) else "
             "(tuple)))",
             "(stmt:eof)"}
        );
    }

    SECTION("If else expression 1") {
        run_parser_expr_test(
            "if true then 123 else 456\n",
            {"(expr (if (lit true) then (lit 123) else (lit 456)))",
             "(stmt:eof)"}
        );
    }

    SECTION("If else expression 2") {
        run_parser_expr_test(
            "if true:\n    123\nelse:\n    456\n",
            {"(expr (if (lit true) then (block (expr (lit 123))) else (block "
             "(expr (lit 456)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("If else expression 3") {
        run_parser_expr_test(
            "if true { 123 } else { 456 }\n",
            {"(expr (if (lit true) then (block (expr (lit 123))) else (block "
             "(expr (lit 456)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("If else if expression 1") {
        run_parser_expr_test(
            "if true then 123 else if false then 456 else 789\n",
            {"(expr (if (lit true) then (lit 123) else (if (lit false) then "
             "(lit 456) else (lit 789))))",
             "(stmt:eof)"}
        );
    }

    SECTION("If else if expression 2") {
        run_parser_expr_test(
            "if true:\n    123\nelse if false:\n    456\nelse:\n    789\n",
            {"(expr (if (lit true) then (block (expr (lit 123))) else (if (lit "
             "false) then (block (expr (lit 456))) else (block (expr (lit "
             "789))))))",
             "(stmt:eof)"}
        );
    }

    SECTION("If else if expression 3") {
        run_parser_expr_test(
            "if true { 123 } else if false { 456 } else { 789 }\n",
            {"(expr (if (lit true) then (block (expr (lit 123))) else (if (lit "
             "false) then (block (expr (lit 456))) else (block (expr (lit "
             "789))))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Weird if expression") {
        run_parser_expr_test(
            "if if true then true else false then 123 else 456\n",
            {"(expr (if (if (lit true) then (lit true) else (lit false)) then "
             "(lit 123) else (lit 456)))",
             "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser loop expressions", "[parser]") {
    SECTION("Infinite loop 1") {
        run_parser_expr_test(
            "loop 123\n",
            {"(expr (loop (lit 123)))", "(stmt:eof)"}
        );
    }

    SECTION("Infinite loop 2") {
        run_parser_expr_test(
            "loop:\n    123\n",
            {"(expr (loop (block (expr (lit 123)))))", "(stmt:eof)"}
        );
    }
    SECTION("Infinite loop 3") {
        run_parser_expr_test(
            "loop { 123 }\n",
            {"(expr (loop (block (expr (lit 123)))))", "(stmt:eof)"}
        );
    }

    SECTION("While loop 1") {
        run_parser_expr_test(
            "while condition do 123\n",
            {"(expr (loop while (nameref condition) (lit 123)))", "(stmt:eof)"}
        );
    }

    SECTION("While loop 2") {
        run_parser_expr_test(
            "while condition:\n    123\n",
            {"(expr (loop while (nameref condition) (block (expr (lit 123)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("While loop 3") {
        run_parser_expr_test(
            "while condition { 123 }\n",
            {"(expr (loop while (nameref condition) (block (expr (lit 123)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("While true loop") {
        run_parser_expr_test(
            "while true do 123\n",
            {"(expr (loop (lit 123)))", "(stmt:eof)"}
        );
    }

    SECTION("Do-while loop 1") {
        run_parser_expr_test(
            "do 123 while condition\n",
            {"(expr (loop do while (nameref condition) (lit 123)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Do-while loop 2") {
        run_parser_expr_test(
            "do:\n    123\nwhile condition\n",
            {"(expr (loop do while (nameref condition) (block (expr (lit "
             "123)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Do-while loop 3") {
        run_parser_expr_test(
            "do { 123 } while condition\n",
            {"(expr (loop do while (nameref condition) (block (expr (lit "
             "123)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Do-while true loop") {
        run_parser_expr_test(
            "do 123 while true\n",
            {"(expr (loop (lit 123)))", "(stmt:eof)"}
        );
    }
}

// MARK: Error tests

TEST_CASE("Parser block errors", "[parser]") {
    SECTION("Not a block") {
        run_parser_expr_error_test("block 123", Err::NotABlock);
    }

    SECTION("Conditional without then or block") {
        run_parser_expr_error_test(
            "if true 123",
            Err::ConditionalWithoutThenOrBlock
        );
    }
}

TEST_CASE("Parser access errors", "[parser]") {
    SECTION("Unexpected token after dot 1") {
        run_parser_expr_error_test("a.+b", Err::UnexpectedTokenAfterDot);
    }

    SECTION("Unexpected token after dot 2") {
        run_parser_expr_error_test("a.(b)", Err::UnexpectedTokenAfterDot);
    }
}
