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
        run_parser_expr_test("123", {"(expr (lit i32 123))", "(stmt:eof)"});
    }

    SECTION("Nullptr") {
        run_parser_expr_test("nullptr", {"(expr (lit nullptr))", "(stmt:eof)"});
    }
}

TEST_CASE("Parser numbers", "[parser]") {
    SECTION("Integers 1") {
        run_parser_expr_test(
            "42; -7; 0",
            {"(expr (lit i32 42))",
             "(expr (lit i32 -7))",
             "(expr (lit i32 0))",
             "(stmt:eof)"}
        );
    }

    SECTION("Integers 2") {
        run_parser_expr_test(
            "0b1010 0o52 0x2A 0037",
            {"(expr (lit i32 10))",
             "(expr (lit i32 42))",
             "(expr (lit i32 42))",
             "(expr (lit i32 37))",
             "(stmt:eof)"}
        );
    }

    SECTION("Integers 3") {
        run_parser_expr_test(
            "-0b1111; -0o17; -0xF",
            {"(expr (lit i32 -15))",
             "(expr (lit i32 -15))",
             "(expr (lit i32 -15))",
             "(stmt:eof)"}
        );
    }

    SECTION("Integers 4") {
        run_parser_expr_test(
            "1_000_000; 0b0110_1010; 0xB_AD_C0_DE",
            {"(expr (lit i32 1000000))",
             "(expr (lit i32 106))",
             "(expr (lit i32 195936478))",
             "(stmt:eof)"}
        );
    }

    SECTION("Integers 5") {
        run_parser_expr_test(
            "10; _10; 10_; 1__0; 10__",
            {"(expr (lit i32 10))",
             "(expr (nameref _10))",
             "(expr (lit i32 10))",
             "(expr (lit i32 10))",
             "(expr (lit i32 10))",
             "(stmt:eof)"}
        );
    }

    SECTION("Int 32 Max") {
        run_parser_expr_test(
            "2147483647",
            {"(expr (lit i32 2147483647))", "(stmt:eof)"}
        );
    }

    SECTION("Int 32 Min") {
        run_parser_expr_test(
            "-2147483648",
            {"(expr (lit i32 -2147483648))", "(stmt:eof)"}
        );
    }

    SECTION("Int 8") {
        run_parser_expr_test(
            "37_i8; -128_i8; 127_i8; 0b0111_1111_i8; 0x7F_i8",
            {"(expr (lit i8 37))",
             "(expr (lit i8 -128))",
             "(expr (lit i8 127))",
             "(expr (lit i8 127))",
             "(expr (lit i8 127))",
             "(stmt:eof)"}
        );
    }

    SECTION("Int 16") {
        run_parser_expr_test(
            "32000_i16; -32768_i16; 32767_i16; 0x7FFF_i16",
            {"(expr (lit i16 32000))",
             "(expr (lit i16 -32768))",
             "(expr (lit i16 32767))",
             "(expr (lit i16 32767))",
             "(stmt:eof)"}
        );
    }

    SECTION("Int 64") {
        run_parser_expr_test(
            "9223372036854775807_i64; -9223372036854775808_i64; "
            "0x7FFFFFFFFFFFFFFF_i64",
            {"(expr (lit i64 9223372036854775807))",
             "(expr (lit i64 -9223372036854775808))",
             "(expr (lit i64 9223372036854775807))",
             "(stmt:eof)"}
        );
    }

    SECTION("UInt 8") {
        run_parser_expr_test(
            "255_u8; 0b1111_1111_u8; 0xFF_u8",
            {"(expr (lit u8 255))",
             "(expr (lit u8 255))",
             "(expr (lit u8 255))",
             "(stmt:eof)"}
        );
    }

    SECTION("UInt 16") {
        run_parser_expr_test(
            "65535_u16; 0xFFFF_u16",
            {"(expr (lit u16 65535))", "(expr (lit u16 65535))", "(stmt:eof)"}
        );
    }

    SECTION("UInt 32") {
        run_parser_expr_test(
            "4294967295_u32; 0xFFFFFFFF_u32",
            {"(expr (lit u32 4294967295))",
             "(expr (lit u32 4294967295))",
             "(stmt:eof)"}
        );
    }

    SECTION("UInt 64") {
        run_parser_expr_test(
            "18446744073709551615_u64; 0xFFFFFFFFFFFFFFFF_u64",
            {"(expr (lit u64 18446744073709551615))",
             "(expr (lit u64 18446744073709551615))",
             "(stmt:eof)"}
        );
    }

    SECTION("Floats 1") {
        run_parser_expr_test(
            "3.14; -0.001; 2e10",
            {"(expr (lit f64 3.140000))",
             "(expr (lit f64 -0.001000))",
             "(expr (lit f64 20000000000.000000))",
             "(stmt:eof)"}
        );
    }

    SECTION("Floats 2") {
        run_parser_expr_test(
            "0.1e-5; 1.5E+10",
            {"(expr (lit f64 0.000001))",
             "(expr (lit f64 15000000000.000000))",
             "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser expressions", "[parser]") {
    SECTION("Unary 1") {
        run_parser_expr_test(
            "-123; -(123)",
            {"(expr (lit i32 -123))",
             "(expr (unary - (lit i32 123)))",
             "(stmt:eof)"}
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
            {"(expr (binary + (lit i32 1) (lit i32 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Binary 2") {
        run_parser_expr_test(
            "1 + 2 * 3",
            {"(expr (binary + (lit i32 1) (binary * (lit i32 2) (lit i32 3))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Binary 3") {
        run_parser_expr_test(
            "1 * 2 1 + 2",
            {"(expr (binary * (lit i32 1) (lit i32 2)))",
             "(expr (binary + (lit i32 1) (lit i32 2)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Binary 4") {
        run_parser_expr_test(
            "1 * -2 + -3",
            {"(expr (binary + (binary * (lit i32 1) (lit i32 -2)) (lit i32 "
             "-3)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Comparison 1") {
        run_parser_expr_test(
            "1 < 2",
            {"(expr (binary < (lit i32 1) (lit i32 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Comparison 2") {
        run_parser_expr_test(
            "1 + 2 >= 3 * 4",
            {"(expr (binary >= (binary + (lit i32 1) (lit i32 2)) (binary * "
             "(lit i32 3) "
             "(lit i32 4))))",
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
            {"(expr (assign (nameref a) (lit i32 1)))", "(stmt:eof)"}
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

TEST_CASE("Parser sizeof", "[parser]") {
    SECTION("Sizeof type") {
        run_parser_expr_test(
            "sizeof i32 sizeof u8 sizeof f64",
            {"(expr (sizeof i32))",
             "(expr (sizeof u8))",
             "(expr (sizeof f64))",
             "(stmt:eof)"}
        );
    }

    SECTION("Sizeof typeof") {
        run_parser_expr_test(
            "sizeof typeof(123_i8)",
            {"(expr (sizeof typeof(<expr@1:15>)))", "(stmt:eof)"}
        );
    }

    SECTION("Sizeof tuple") {
        run_parser_expr_test(
            "sizeof i32 sizeof (i32)",
            {"(expr (sizeof i32))", "(expr (sizeof (i32)))", "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser cast expressions", "[parser]") {
    SECTION("Cast 1") {
        run_parser_expr_test(
            "123 as i8; 3.14 as f32; true as u8",
            {"(expr (cast (lit i32 123) as i8))",
             "(expr (cast (lit f64 3.140000) as f32))",
             "(expr (cast (lit true) as u8))",
             "(stmt:eof)"}
        );
    }

    SECTION("Cast 2") {
        run_parser_expr_test(
            "(1 + 2) as i16; -(3.14 as f32) as f64",
            {"(expr (cast (binary + (lit i32 1) (lit i32 2)) as i16))",
             "(expr (cast (unary - (cast (lit f64 3.140000) as f32)) as f64))",
             "(stmt:eof)"}
        );
    }

    SECTION("Cast chain") {
        run_parser_expr_test(
            "123 as i8 as i16 as i32",
            {"(expr (cast (cast (cast (lit i32 123) as i8) as i16) as i32))",
             "(stmt:eof)"}
        );
    }

    SECTION("Casts with nullptr") {
        run_parser_expr_test(
            "nullptr as @i32",
            {"(expr (cast (lit nullptr) as @i32))", "(stmt:eof)"}
        );
    }

    SECTION("Casts with tuples") {
        run_parser_expr_test(
            "(1, 2) as (i32, i32)",
            {"(expr (cast (tuple (lit i32 1) (lit i32 2)) as (i32, i32)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Casts with parentheses") {
        run_parser_expr_test(
            "(123 as i8) + (456 as i16)",
            {"(expr (binary + (cast (lit i32 123) as i8) (cast (lit i32 456) "
             "as i16)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Casts with function calls") {
        run_parser_expr_test(
            "f() as i32",
            {"(expr (cast (call (nameref f)) as i32))", "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser groupings and tuples", "[parser]") {
    SECTION("Grouping 1") {
        run_parser_expr_test("(1)", {"(expr (lit i32 1))", "(stmt:eof)"});
    }

    SECTION("Grouping 2") {
        run_parser_expr_test(
            "(1 + 2)",
            {"(expr (binary + (lit i32 1) (lit i32 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Grouping 3") {
        run_parser_expr_test(
            "(1 + 2) * 3",
            {"(expr (binary * (binary + (lit i32 1) (lit i32 2)) (lit i32 3)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Grouping 4") {
        run_parser_expr_test(
            "(1); (2)",
            {"(expr (lit i32 1))", "(expr (lit i32 2))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple unit") {
        run_parser_expr_test("()", {"(expr (tuple))", "(stmt:eof)"});
    }

    SECTION("Tuple 1") {
        run_parser_expr_test(
            "(1,)",
            {"(expr (tuple (lit i32 1)))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple 2") {
        run_parser_expr_test(
            "(1, 2)",
            {"(expr (tuple (lit i32 1) (lit i32 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple 3") {
        run_parser_expr_test(
            "(1, 2, 3,)",
            {"(expr (tuple (lit i32 1) (lit i32 2) (lit i32 3)))", "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser blocks", "[parser]") {
    SECTION("Braced block 1") {
        run_parser_expr_test(
            "block { 123 }",
            {"(expr (block (expr (lit i32 123))))", "(stmt:eof)"}
        );
    }

    SECTION("Braced block 2") {
        run_parser_expr_test(
            "block { let a = 1 printout a }",
            {"(expr (block (stmt:let a (lit i32 1)) (stmt:print (nameref a))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Braced block 3") {
        run_parser_expr_test(
            "block { 123 } 456",
            {"(expr (block (expr (lit i32 123))))",
             "(expr (lit i32 456))",
             "(stmt:eof)"}
        );
    }

    SECTION("Indented block 1") {
        run_parser_expr_test(
            "block:\n    123\n",
            {"(expr (block (expr (lit i32 123))))", "(stmt:eof)"}
        );
    }

    SECTION("Indented block 2") {
        run_parser_expr_test(
            "block:\n    let a = 1\n    printout a\n",
            {"(expr (block (stmt:let a (lit i32 1)) (stmt:print (nameref a))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Indented block 3") {
        run_parser_expr_test(
            "block:\n    123\n456\n",
            {"(expr (block (expr (lit i32 123))))",
             "(expr (lit i32 456))",
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
            {"(expr (block unsafe (expr (lit i32 123))))", "(stmt:eof)"}
        );
    }

    SECTION("Indented unsafe block") {
        run_parser_expr_test(
            "unsafe:\n    123\n",
            {"(expr (block unsafe (expr (lit i32 123))))", "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser tuples", "[parser]") {
    SECTION("Tuple 1") {
        run_parser_expr_test(
            "(1,)",
            {"(expr (tuple (lit i32 1)))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple 2") {
        run_parser_expr_test(
            "(1, 2)",
            {"(expr (tuple (lit i32 1) (lit i32 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple 3") {
        run_parser_expr_test(
            "(1, 2, 3,)",
            {"(expr (tuple (lit i32 1) (lit i32 2) (lit i32 3)))", "(stmt:eof)"}
        );
    }

    SECTION("Not a tuple") {
        run_parser_expr_test("(1)", {"(expr (lit i32 1))", "(stmt:eof)"});
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
            {"(expr (if (lit true) then (lit i32 123) else (tuple)))",
             "(stmt:eof)"}
        );
    }

    SECTION("If expression 2") {
        run_parser_expr_test(
            "if true:\n    123\n",
            {"(expr (if (lit true) then (block (expr (lit i32 123))) else "
             "(tuple)))",
             "(stmt:eof)"}
        );
    }

    SECTION("If expression 3") {
        run_parser_expr_test(
            "if true { 123 }\n",
            {"(expr (if (lit true) then (block (expr (lit i32 123))) else "
             "(tuple)))",
             "(stmt:eof)"}
        );
    }

    SECTION("If else expression 1") {
        run_parser_expr_test(
            "if true then 123 else 456\n",
            {"(expr (if (lit true) then (lit i32 123) else (lit i32 456)))",
             "(stmt:eof)"}
        );
    }

    SECTION("If else expression 2") {
        run_parser_expr_test(
            "if true:\n    123\nelse:\n    456\n",
            {"(expr (if (lit true) then (block (expr (lit i32 123))) else "
             "(block "
             "(expr (lit i32 456)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("If else expression 3") {
        run_parser_expr_test(
            "if true { 123 } else { 456 }\n",
            {"(expr (if (lit true) then (block (expr (lit i32 123))) else "
             "(block "
             "(expr (lit i32 456)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("If else if expression 1") {
        run_parser_expr_test(
            "if true then 123 else if false then 456 else 789\n",
            {"(expr (if (lit true) then (lit i32 123) else (if (lit false) "
             "then "
             "(lit i32 456) else (lit i32 789))))",
             "(stmt:eof)"}
        );
    }

    SECTION("If else if expression 2") {
        run_parser_expr_test(
            "if true:\n    123\nelse if false:\n    456\nelse:\n    789\n",
            {"(expr (if (lit true) then (block (expr (lit i32 123))) else (if "
             "(lit "
             "false) then (block (expr (lit i32 456))) else (block (expr (lit "
             "i32 "
             "789))))))",
             "(stmt:eof)"}
        );
    }

    SECTION("If else if expression 3") {
        run_parser_expr_test(
            "if true { 123 } else if false { 456 } else { 789 }\n",
            {"(expr (if (lit true) then (block (expr (lit i32 123))) else (if "
             "(lit "
             "false) then (block (expr (lit i32 456))) else (block (expr (lit "
             "i32 "
             "789))))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Weird if expression") {
        run_parser_expr_test(
            "if if true then true else false then 123 else 456\n",
            {"(expr (if (if (lit true) then (lit true) else (lit false)) then "
             "(lit i32 123) else (lit i32 456)))",
             "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser loop expressions", "[parser]") {
    SECTION("Infinite loop 1") {
        run_parser_expr_test(
            "loop 123\n",
            {"(expr (loop (block (expr (lit i32 123)))))", "(stmt:eof)"}
        );
    }

    SECTION("Infinite loop 2") {
        run_parser_expr_test(
            "loop:\n    123\n",
            {"(expr (loop (block (expr (lit i32 123)))))", "(stmt:eof)"}
        );
    }
    SECTION("Infinite loop 3") {
        run_parser_expr_test(
            "loop { 123 }\n",
            {"(expr (loop (block (expr (lit i32 123)))))", "(stmt:eof)"}
        );
    }

    SECTION("While loop 1") {
        run_parser_expr_test(
            "while condition do 123\n",
            {"(expr (loop while (nameref condition) (block (expr (lit i32 "
             "123)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("While loop 2") {
        run_parser_expr_test(
            "while condition:\n    123\n",
            {"(expr (loop while (nameref condition) (block (expr (lit i32 "
             "123)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("While loop 3") {
        run_parser_expr_test(
            "while condition { 123 }\n",
            {"(expr (loop while (nameref condition) (block (expr (lit i32 "
             "123)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("While true loop") {
        run_parser_expr_test(
            "while true do 123\n",
            {"(expr (loop (block (expr (lit i32 123)))))", "(stmt:eof)"}
        );
    }

    SECTION("Do-while loop 1") {
        run_parser_expr_test(
            "do 123 while condition\n",
            {"(expr (loop do while (nameref condition) (block (expr (lit i32 "
             "123)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Do-while loop 2") {
        run_parser_expr_test(
            "do:\n    123\nwhile condition\n",
            {"(expr (loop do while (nameref condition) (block (expr (lit i32 "
             "123)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Do-while loop 3") {
        run_parser_expr_test(
            "do { 123 } while condition\n",
            {"(expr (loop do while (nameref condition) (block (expr (lit i32 "
             "123)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Do-while true loop") {
        run_parser_expr_test(
            "do 123 while true\n",
            {"(expr (loop (block (expr (lit i32 123)))))", "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser call expressions", "[parser]") {
    SECTION("Call with no arguments") {
        run_parser_expr_test(
            "f()",
            {"(expr (call (nameref f)))", "(stmt:eof)"}
        );
    }

    SECTION("Call with one argument") {
        run_parser_expr_test(
            "f(123)",
            {"(expr (call (nameref f) (lit i32 123)))", "(stmt:eof)"}
        );
    }

    SECTION("Call with trailing comma") {
        run_parser_expr_test(
            "f(123,)",
            {"(expr (call (nameref f) (lit i32 123)))", "(stmt:eof)"}
        );
    }

    SECTION("Call with multiple arguments") {
        run_parser_expr_test(
            "f(1, 2, 3)",
            {"(expr (call (nameref f) (lit i32 1) (lit i32 2) (lit i32 3)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Call with complex arguments") {
        run_parser_expr_test(
            "f(a + b, c * d)",
            {"(expr (call (nameref f) (binary + (nameref a) (nameref b)) "
             "(binary * (nameref c) (nameref d))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Call with named arguments") {
        run_parser_expr_test(
            "f(x: 1, y: 2)",
            {"(expr (call (nameref f) (x: (lit i32 1)) (y: (lit i32 2))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Call with identifier argument") {
        run_parser_expr_test(
            "f(y)",
            {"(expr (call (nameref f) (nameref y)))", "(stmt:eof)"}
        );
    }

    SECTION("Call with mixed arguments") {
        run_parser_expr_test(
            "f(1, y: 2)",
            {"(expr (call (nameref f) (lit i32 1) (y: (lit i32 2))))",
             "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser subscript expressions", "[parser]") {
    SECTION("Subscript with integer") {
        run_parser_expr_test(
            "a[0]",
            {"(expr (subscript (nameref a) (lit i32 0)))", "(stmt:eof)"}
        );
    }

    SECTION("Subscript with expression") {
        run_parser_expr_test(
            "a[i + 1]",
            {"(expr (subscript (nameref a) (binary + (nameref i) (lit i32 "
             "1))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Multiple subscripts") {
        run_parser_expr_test(
            "a[i][j + 1]",
            {"(expr (subscript (subscript (nameref a) (nameref i)) "
             "(binary + (nameref j) (lit i32 1))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Subscript with call") {
        run_parser_expr_test(
            "a[f(i)]",
            {"(expr (subscript (nameref a) (call (nameref f) (nameref i))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Subscript with nested subscript") {
        run_parser_expr_test(
            "a[b[i]]",
            {"(expr (subscript (nameref a) (subscript (nameref b) (nameref "
             "i))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Subscript with tuple index") {
        run_parser_expr_test(
            "a[(i, j)]",
            {"(expr (subscript (nameref a) (tuple (nameref i) (nameref j))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Subscript with really big expression") {
        run_parser_expr_test(
            "a[(i + 1) * (j - 2) / f(k)]",
            {"(expr (subscript (nameref a) (binary / (binary * (binary + "
             "(nameref i) (lit i32 1)) (binary - (nameref j) (lit i32 2))) "
             "(call (nameref f) (nameref k)))))",
             "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser postfix expressions", "[parser]") {
    SECTION("Postfix 1") {
        run_parser_expr_test(
            "a.b()",
            {"(expr (call (access . (nameref a) b)))", "(stmt:eof)"}
        );
    }

    SECTION("Postfix 2") {
        run_parser_expr_test(
            "a().b",
            {"(expr (access . (call (nameref a)) b))", "(stmt:eof)"}
        );
    }

    SECTION("Postfix 3") {
        run_parser_expr_test(
            "a.b().c()",
            {"(expr (call (access . (call (access . (nameref a) b)) c)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Postfix 4") {
        run_parser_expr_test(
            "a()().b.c",
            {"(expr (access . (access . (call (call (nameref a))) b) c))",
             "(stmt:eof)"}
        );
    }

    SECTION("Postfix 5") {
        run_parser_expr_test(
            "a.b()[c]",
            {"(expr (subscript (call (access . (nameref a) b)) (nameref c)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Postfix 6") {
        run_parser_expr_test(
            "a[b].c()",
            {"(expr (call (access . (subscript (nameref a) (nameref b)) c)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Postfix 7") {
        run_parser_expr_test(
            "a()[b]()[c]",
            {"(expr (subscript (call (subscript (call (nameref a)) (nameref "
             "b))) (nameref c)))",
             "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser array expressions", "[parser]") {
    SECTION("Array literal with integers") {
        run_parser_expr_test(
            "[1, 2, 3]",
            {"(expr (array (lit i32 1) (lit i32 2) (lit i32 3)))", "(stmt:eof)"}
        );
    }

    SECTION("Array trailing comma") {
        run_parser_expr_test(
            "[1, 2, 3,]",
            {"(expr (array (lit i32 1) (lit i32 2) (lit i32 3)))", "(stmt:eof)"}
        );
    }

    SECTION("Array literal with mixed expressions") {
        run_parser_expr_test(
            "[a, b + c, d * e]",
            {"(expr (array (nameref a) (binary + (nameref b) (nameref c)) "
             "(binary * (nameref d) (nameref e))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Empty array literal") {
        run_parser_expr_test("[]", {"(expr (array))", "(stmt:eof)"});
    }
}

TEST_CASE("Parser alloc expressions", "[parser]") {
    SECTION("Alloc type 1") {
        run_parser_expr_test("alloc i32", {"(expr (alloc i32))", "(stmt:eof)"});
    }

    SECTION("Alloc type 2") {
        run_parser_expr_test("alloc f64", {"(expr (alloc f64))", "(stmt:eof)"});
    }

    SECTION("Alloc type 3") {
        run_parser_expr_test("alloc @i8", {"(expr (alloc @i8))", "(stmt:eof)"});
    }

    SECTION("Alloc type 4") {
        run_parser_expr_test(
            "alloc (i32, f64)",
            {"(expr (alloc (i32, f64)))", "(stmt:eof)"}
        );
    }

    SECTION("Alloc type 5") {
        run_parser_expr_test(
            "alloc [i32; 3]",
            {"(expr (alloc [i32; 3]))", "(stmt:eof)"}
        );
    }

    SECTION("Alloc type with 1") {
        run_parser_expr_test(
            "alloc i32 with 1",
            {"(expr (alloc i32 with (lit i32 1)))", "(stmt:eof)"}
        );
    }

    SECTION("Alloc type with 2") {
        run_parser_expr_test(
            "alloc i32 with a + b",
            {"(expr (alloc i32 with (binary + (nameref a) (nameref b))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Alloc type with 3") {
        run_parser_expr_test(
            "alloc [i32; 3] with [1, 2, 3]",
            {"(expr (alloc [i32; 3] with (array (lit i32 1) (lit i32 2) "
             "(lit i32 3))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Alloc type with 4") {
        run_parser_expr_test(
            "alloc (i32, f64) with (1, 3.14)",
            {"(expr (alloc (i32, f64) with (tuple (lit i32 1) (lit f64 "
             "3.140000))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Alloc type with 5") {
        run_parser_expr_test(
            "alloc @i32 with alloc i32 with 10",
            {"(expr (alloc @i32 with (alloc i32 with (lit i32 10))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Alloc with 1") {
        run_parser_expr_test(
            "alloc with 10",
            {"(expr (alloc with (lit i32 10)))", "(stmt:eof)"}
        );
    }

    SECTION("Alloc with 2") {
        run_parser_expr_test(
            "alloc with f()",
            {"(expr (alloc with (call (nameref f))))", "(stmt:eof)"}
        );
    }

    SECTION("Alloc with 3") {
        run_parser_expr_test(
            "alloc with @x",
            {"(expr (alloc with (address @ (nameref x))))", "(stmt:eof)"}
        );
    }

    SECTION("Alloc with 4") {
        run_parser_expr_test(
            "alloc with alloc with 10",
            {"(expr (alloc with (alloc with (lit i32 10))))", "(stmt:eof)"}
        );
    }

    SECTION("Alloc with 5") {
        run_parser_expr_test(
            "alloc with block { yield 42 }",
            {"(expr (alloc with (block (stmt:yield yield (lit i32 42)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Alloc with 6") {
        run_parser_expr_test(
            "alloc with 1 as i64",
            {"(expr (alloc with (cast (lit i32 1) as i64)))", "(stmt:eof)"}
        );
    }

    SECTION("Alloc for 1") {
        run_parser_expr_test(
            "alloc for 5 of i32",
            {"(expr (alloc for (lit i32 5) of i32))", "(stmt:eof)"}
        );
    }

    SECTION("Alloc for 2") {
        run_parser_expr_test(
            "alloc for n of f64",
            {"(expr (alloc for (nameref n) of f64))", "(stmt:eof)"}
        );
    }

    SECTION("Alloc for 3") {
        run_parser_expr_test(
            "alloc for 10 + m of @i8",
            {"(expr (alloc for (binary + (lit i32 10) (nameref m)) of @i8))",
             "(stmt:eof)"}
        );
    }
}

// MARK: Error tests

TEST_CASE("Parser number errors", "[parser]") {
    SECTION("Int 32 Max Plus One") {
        run_parser_expr_error_test(
            "2147483648", // One more than max int32_t
            Err::NumberOutOfRange
        );
    }

    SECTION("Int 32 Min Minus One") {
        run_parser_expr_error_test(
            "-2147483649", // One less than min int32_t
            Err::NumberOutOfRange
        );
    }

    SECTION("Int 32 Min With Parentheses") {
        run_parser_expr_error_test(
            "-(2147483648)", // Min int32_t inside parentheses
            Err::NumberOutOfRange
        );
    }

    SECTION("Negative UInt") {
        run_parser_expr_error_test("-1_u32", Err::NegativeOnUnsignedLiteral);
    }
}

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

TEST_CASE("Parser call errors", "[parser]") {
    SECTION("Positional argument after named argument") {
        run_parser_expr_error_test(
            "f(x: 1, 2)",
            Err::PosArgumentAfterNamedArgument
        );
    }
}

TEST_CASE("Parser alloc errors", "[parser]") {
    SECTION("Alloc without expression") {
        run_parser_expr_error_test("alloc", Err::NotAType);
    }

    SECTION("Alloc with without expr") {
        run_parser_expr_error_test("alloc with", Err::NotAnExpression);
    }

    SECTION("Alloc for without expr") {
        run_parser_expr_error_test("alloc for", Err::NotAnExpression);
    }

    SECTION("Alloc for without of") {
        run_parser_expr_error_test("alloc for 10", Err::AllocForWithoutOf);
    }
}
