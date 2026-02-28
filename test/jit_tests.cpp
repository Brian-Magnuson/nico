#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <llvm/Support/Error.h>

#include "nico/backend/jit.h"
#include "nico/frontend/frontend.h"
#include "nico/frontend/utils/frontend_context.h"
#include "nico/shared/code_file.h"
#include "nico/shared/logger.h"
#include "nico/shared/status.h"
#include "nico/shared/token.h"

#include "test_utils.h"

/**
 * @brief Run a JIT test with the given source code, expected output, and
 * expected return code.
 *
 * This test will check that the source code compiles successfully.
 *
 * If expected_output is provided, this test will check that the output of the
 * JIT (written to stdout) matches the expected output.
 *
 * If expected_return_code is provided, this test will check that the return
 * code of the JIT matches the expected return code.
 * If expected_return_code is non-zero, the code generator will be set to panic
 * recoverable mode to avoid a signal termination.
 *
 * @param source The source code to test.
 * @param expected_output (Optional) The expected output, if any.
 * @param expected_return_code (Optional) The expected return code, if any.
 * @param print_ir Whether to print the generated IR before verification.
 * Defaults to false.
 */
void run_jit_test(
    std::string_view source,
    std::optional<std::string_view> expected_output = std::nullopt,
    std::optional<int> expected_return_code = std::nullopt,
    bool print_ir = false
) {
    auto file = nico::make_test_code_file(source);

    nico::Logger::inst().set_printing_enabled(true);

    nico::Frontend frontend;
    if (expected_return_code.has_value() && *expected_return_code != 0) {
        // If we expect a non-zero return code, enable panic recovery to avoid
        // terminating the program.
        frontend.set_panic_recoverable(true);
    }

    frontend.set_ir_printing_enabled(print_ir);

    std::unique_ptr<nico::FrontendContext>& context =
        frontend.compile(file, false);
    REQUIRE(IS_VARIANT(context->status, nico::Status::Ok));

    std::unique_ptr<nico::IJit> jit = std::make_unique<nico::SimpleJit>();
    auto jit_err = jit->add_module_and_context(std::move(context->mod_ctx));
    REQUIRE(!jit_err);

    std::optional<llvm::Expected<int>> return_code;
    auto [out, err] = nico::capture_stdout(
        [&]() {
            return_code = jit->run_main_func(0, nullptr, context->main_fn_name);
        },
        4096
    );

    if (expected_output) {
        CHECK(out == *expected_output);
    }
    if (expected_return_code) {
        REQUIRE(return_code.has_value());
        REQUIRE(*return_code);
        CHECK(return_code->get() == *expected_return_code);
    }

    frontend.reset();
    jit->reset();
}

TEST_CASE("JIT print statements", "[jit]") {
    SECTION("Print hello world 1") {
        run_jit_test(R"(printout "Hello, World!")", "Hello, World!");
    }

    SECTION("Print hello world 2") {
        run_jit_test(
            R"(printout "Hello, World!" printout "Goodbye, World!")",
            "Hello, World!Goodbye, World!"
        );
    }

    SECTION("Print hello world 3") {
        run_jit_test(
            R"(printout "Hello, World!\n" printout "Goodbye, World!")",
            "Hello, World!\nGoodbye, World!"
        );
    }

    SECTION("Print hello world 4") {
        run_jit_test(R"(printout "Hello", ", World!")", "Hello, World!");
    }
}

TEST_CASE("JIT let statements", "[jit]") {
    SECTION("Basic variable reference") {
        run_jit_test(R"(let x = 5 printout x)", "5");
    }
}

TEST_CASE("JIT integer operators", "[jit]") {
    SECTION("Unary minus") {
        run_jit_test(R"(let x = -5 printout x, ",", -x)", "-5,5");
    }

    SECTION("Integer addition") {
        run_jit_test(R"(let x = 5 let y = 10 printout x + y)", "15");
    }

    SECTION("Integer subtraction") {
        run_jit_test(R"(let x = 5 let y = 10 printout y - x)", "5");
    }

    SECTION("Integer add and subtract negatives") {
        run_jit_test(
            R"(printout -5 + 10, ",", 10 + -5, ",", -5 - 10, ",", 10 - -5)",
            "5,5,-15,15"
        );
    }

    SECTION("Integer multiplication") {
        run_jit_test(
            R"(printout 5 * 10, ",", 2 * -3, ",", 7 * 0, ",", -1 * 1)",
            "50,-6,0,-1"
        );
    }

    SECTION("Integer division") {
        run_jit_test(
            R"(printout 10 / 5, ",", 7 / 2, ",", -9 / 3, ",", 1 / -1)",
            "2,3,-3,-1"
        );
    }

    SECTION("Integer remainder") {
        run_jit_test(
            R"(printout 17 % 5, ",", 17 % -5, ",", -17 % 5, ",", -17 % -5)",
            "2,2,-2,-2"
        );
    }

    SECTION("Integer comparison LT") {
        run_jit_test(R"(printout 1 < 2, 2 < 1, 1 < 1)", "truefalsefalse");
    }

    SECTION("Integer comparison GT") {
        run_jit_test(R"(printout 2 > 1, 1 > 2, 1 > 1)", "truefalsefalse");
    }

    SECTION("Integer comparison LE") {
        run_jit_test(R"(printout 1 <= 2, 2 <= 1, 1 <= 1)", "truefalsetrue");
    }

    SECTION("Integer comparison GE") {
        run_jit_test(R"(printout 2 >= 1, 1 >= 2, 1 >= 1)", "truefalsetrue");
    }

    SECTION("Integer comparison EQ and NEQ") {
        run_jit_test(
            R"(printout 1 == 1, 1 != 1, 1 == 2, 1 != 2)",
            "truefalsefalsetrue"
        );
    }
}

TEST_CASE("JIT float operators", "[jit]") {
    SECTION("Unary minus") {
        run_jit_test(R"(let x = -5.5 printout x, ",", -x)", "-5.5,5.5");
    }

    SECTION("Float addition") {
        run_jit_test(R"(let x = 5.5 let y = 10.2 printout x + y)", "15.7");
    }

    SECTION("Float subtraction") {
        run_jit_test(R"(let x = 5.5 let y = 10.2 printout y - x)", "4.7");
    }

    SECTION("Float multiplication") {
        run_jit_test(
            R"(printout 2.0 * 4.0, ",", 0.5 * 0.25, ",", -8.0 * 0.125, ",", -1.5 * -2.0)",
            "8,0.125,-1,3"
        );
    }

    SECTION("Float division") {
        run_jit_test(
            R"(printout 8.0 / 4.0, ",", 0.5 / 0.25, ",", -8.0 / 0.125, ",", -1.5 / -2.0)",
            "2,2,-64,0.75"
        );
    }

    SECTION("Float comparison LT") {
        run_jit_test(
            R"(printout 1.0 < 2.0, 2.0 < 1.0, 1.0 < 1.0)",
            "truefalsefalse"
        );
    }

    SECTION("Float comparison GT") {
        run_jit_test(
            R"(printout 2.0 > 1.0, 1.0 > 2.0, 1.0 > 1.0)",
            "truefalsefalse"
        );
    }

    SECTION("Float comparison LE") {
        run_jit_test(
            R"(printout 1.0 <= 2.0, 2.0 <= 1.0, 1.0 <= 1.0)",
            "truefalsetrue"
        );
    }

    SECTION("Float comparison GE") {
        run_jit_test(
            R"(printout 2.0 >= 1.0, 1.0 >= 2.0, 1.0 >= 1.0)",
            "truefalsetrue"
        );
    }

    SECTION("Float comparison EQ and NEQ") {
        run_jit_test(
            R"(printout 1.0 == 1.0, 1.0 != 1.0, 1.0 == 2.0, 1.0 != 2.0)",
            "truefalsefalsetrue"
        );
    }
}

TEST_CASE("JIT grouped expressions", "[jit]") {
    SECTION("Order of operations 1") {
        run_jit_test(R"(printout (2 + 3) * (4 - 1))", "15");
    }

    SECTION("Order of operations 2") {
        run_jit_test(R"(printout 2 + 3 * 4)", "14");
    }

    SECTION("Order of operations 3") {
        run_jit_test(R"(printout 4 * (2 + 3))", "20");
    }
}

TEST_CASE("JIT assign expressions", "[jit]") {
    SECTION("Assignment 1") {
        run_jit_test(R"(let var x = 1 x = 2 printout x)", "2");
    }

    SECTION("Assignment 2") {
        run_jit_test(
            R"(let var x = 5 printout x, ",", (x = 10), ",", x)",
            "5,10,10"
        );
    }

    SECTION("Assignment chain") {
        run_jit_test(
            R"(let var x = 1 let var y = 2 let var z = 3 x = y = z printout x, ",", y, ",", z)",
            "3,3,3"
        );
    }

    SECTION("Compound assignment") {
        run_jit_test(
            R"(let var x = 5 
            x += 10 printout x 
            x -= 3 printout ",", x 
            x *= 2 printout ",", x 
            x /= 4 printout ",", x
            x %= 3 printout ",", x
            )",
            "15,12,24,6,0"
        );
    }
}

TEST_CASE("JIT sizeof expressions", "[jit]") {
    SECTION("Size of int") {
        run_jit_test(R"(printout sizeof i32)", "4");
    }

    SECTION("Size of float") {
        run_jit_test(R"(printout sizeof f64)", "8");
    }

    SECTION("Size of bool") {
        run_jit_test(R"(printout sizeof bool)", "1");
    }

    SECTION("Size of type of") {
        run_jit_test(
            R"(
        let var x = 42_i16
        printout sizeof typeof(x)
        )",
            "2"
        );
    }
}

TEST_CASE("JIT cast expressions", "[jit]") {
    SECTION("Cast NoOp") {
        run_jit_test("let a: i32 = 1 let b: i32 = a as i32 printout b", "1");
    }

    SECTION("Cast IntToBool 1") {
        run_jit_test(
            "let a: i32 = 1 let b: bool = a as bool printout b",
            "true"
        );
    }

    SECTION("Cast IntToBool 2") {
        run_jit_test(
            "let a: i32 = 0 let b: bool = a as bool printout b",
            "false"
        );
    }

    SECTION("Cast IntToBool 3") {
        run_jit_test(
            "let a: i32 = -42 let b: bool = a as bool printout b",
            "true"
        );
    }

    SECTION("Cast IntToBool 4") {
        run_jit_test(
            "let a: u32 = 1_u32 let b: bool = a as bool printout b",
            "true"
        );
    }

    SECTION("Cast FPToBool 1") {
        run_jit_test(
            "let a: f64 = 3.14 let b: bool = a as bool printout b",
            "true"
        );
    }

    SECTION("Cast FPToBool 2") {
        run_jit_test(
            "let a: f64 = 0.0 let b: bool = a as bool printout b",
            "false"
        );
    }

    SECTION("Cast FPToBool 3") {
        run_jit_test(
            "let a: f32 = -2.71_f32 let b: bool = a as bool printout b",
            "true"
        );
    }

    SECTION("Cast FPToBool 4") {
        run_jit_test(
            "let a: f64 = nan let b: bool = a as bool printout b",
            "true"
        );
    }

    SECTION("Cast SignExt 1") {
        run_jit_test(
            "let a: i8 = -1_i8 let b: i32 = a as i32 printout b",
            "-1"
        );
    }

    SECTION("Cast SignExt 2") {
        run_jit_test(
            "let a: i8 = 42_i8 let b: i32 = a as i32 printout b",
            "42"
        );
    }

    SECTION("Cast ZeroExt 1") {
        run_jit_test(
            "let a: u8 = 255_u8 let b: u32 = a as u32 printout b",
            "255"
        );
    }

    SECTION("Cast ZeroExt 2") {
        run_jit_test(
            "let a: u8 = 255_u8 let b: i32 = a as i32 printout b",
            "255"
        );
    }

    SECTION("Cast ZeroExt 3") {
        run_jit_test(
            "let a: i8 = -1_i8 let b: u32 = a as u32 printout b",
            "255"
        );
    }

    SECTION("Cast FPExt") {
        run_jit_test(
            "let a: f32 = 3.14_f32 let b: f64 = a as f64 printout b",
            "3.14"
        );
    }

    SECTION("Cast IntTrunc 1") {
        run_jit_test("let a: i32 = 300 let b: i8 = a as i8 printout b", "44");
    }

    SECTION("Cast IntTrunc 2") {
        run_jit_test(
            "let a: i32 = -2147483648 let b: i8 = a as i8 printout b",
            "0"
        );
    }

    SECTION("Cast FPTrunc") {
        run_jit_test(
            "let a: f64 = 3.1415926535 let b: f32 = a as f32 printout b",
            "3.14159"
        );
    }

    SECTION("Cast FPToSInt 1") {
        run_jit_test(
            "let a: f64 = 42.99 let b: i32 = a as i32 printout b",
            "42"
        );
    }

    SECTION("Cast FPToSInt 2") {
        run_jit_test(
            "let a: f64 = -42.99 let b: i32 = a as i32 printout b",
            "-42"
        );
    }

    SECTION("Cast FPToSInt 3") {
        run_jit_test(
            "let a: f64 = 1e20 let b: i32 = a as i32 printout b",
            "2147483647"
        );
    }

    SECTION("Cast FPToSInt 4") {
        run_jit_test(
            "let a: f64 = -1e20 let b: i32 = a as i32 printout b",
            "-2147483648"
        );
    }

    SECTION("Cast FPToUInt 1") {
        run_jit_test(
            "let a: f64 = 42.99 let b: u32 = a as u32 printout b",
            "42"
        );
    }

    SECTION("Cast FPToUInt 2") {
        run_jit_test(
            "let a: f64 = -42.99 let b: u32 = a as u32 printout b",
            "0"
        );
    }

    SECTION("Cast FPToUInt 3") {
        run_jit_test(
            "let a: f64 = 1e20 let b: u32 = a as u32 printout b",
            "4294967295"
        );
    }

    SECTION("Cast SIntToFP 1") {
        run_jit_test("let a: i32 = 42 let b: f64 = a as f64 printout b", "42");
    }

    SECTION("Cast SIntToFP 2") {
        run_jit_test(
            "let a: i32 = -42 let b: f64 = a as f64 printout b",
            "-42"
        );
    }

    SECTION("Cast UIntToFP 1") {
        run_jit_test(
            "let a: u32 = 42_u32 let b: f64 = a as f64 printout b",
            "42"
        );
    }

    SECTION("Cast UIntToFP 2") {
        run_jit_test(
            "let a: u32 = 4294967295_u32 let b: f64 = a as f64 printout b",
            "4.29497e+09"
        );
    }
}

TEST_CASE("JIT block expressions", "[jit]") {
    SECTION("Simple block expression") {
        run_jit_test(R"(block { yield 5 })");
    }

    SECTION("Block expression 1") {
        run_jit_test("let var x = 1 x = block { yield 2 } printout x", "2");
    }

    SECTION("Block expression 2") {
        run_jit_test(
            "let var x = 1 x = block { yield x * 2 + 1 } printout x",
            "3"
        );
    }

    SECTION("Block expression 3") {
        run_jit_test(
            "let var x = 1 let var y = 2 x = block { let var y = x + 3 "
            "yield y "
            "} printout x, \",\", y",
            "4,2"
        );
    }

    SECTION("Block expression 4") {
        run_jit_test(
            "let var x = 1 x = block { yield 2 * block { yield 3 } } "
            "printout "
            "x",
            "6"
        );
    }
}

TEST_CASE("JIT tuple expressions", "[jit]") {
    SECTION("Tuple expression 1") {
        run_jit_test(
            R"(let x = (1, 2, 3) printout x.0, ",", x.1, ",", x.2)",
            "1,2,3"
        );
    }

    SECTION("Tuple expression 2") {
        run_jit_test(
            R"(let x = (1, (2, 3), 4) printout x.0, ",", x.1.0, ",", x.1.1, ",", x.2)",
            "1,2,3,4"
        );
    }

    SECTION("Tuple assignment 1") {
        run_jit_test(
            R"(let var x = (1, 2) x.0 = 3 x.1 = 4 printout x.0, ",", x.1)",
            "3,4"
        );
    }

    SECTION("Tuple assignment 2") {
        run_jit_test(
            R"(let var x = (1, (2, 3)) x.1.0 = 4 x.1.1 = 5 printout x.0, ",", x.1.0, ",", x.1.1)",
            "1,4,5"
        );
    }

    SECTION("Tuple printing 1") {
        run_jit_test(R"(printout (1, 2, 3))", "(1, 2, 3)");
    }

    SECTION("Tuple printing 2") {
        run_jit_test(
            R"(printout (1, (2.0, true), "hello"))",
            "(1, (2, true), \"hello\")"
        );
    }

    SECTION("Tuple printing 3") {
        run_jit_test(R"(printout ())", "()");
    }

    SECTION("Tuple printing 4") {
        run_jit_test("let var x = (1, 2) x.0 = 3 printout x", "(3, 2)");
    }
}

TEST_CASE("JIT if expressions", "[jit]") {
    SECTION("If expression 1") {
        run_jit_test(
            R"(if true { printout "true" } else { printout "false" })",
            "true"
        );
    }

    SECTION("If expression 2") {
        run_jit_test(
            R"(if false { printout "true" } else { printout "false" })",
            "false"
        );
    }

    SECTION("If expression 3") {
        run_jit_test(
            R"(let var x = 1 if true { x = 2 } else { x = 3 } printout x)",
            "2"
        );
    }

    SECTION("If expression 4") {
        run_jit_test(
            R"(
            if true:
                printout "true"
            else:
                printout "false"
            )",
            "true"
        );
    }

    SECTION("If expression 5") {
        run_jit_test(
            R"(
            let x = if true then 1 else 2
            printout x
            )",
            "1"
        );
    }

    SECTION("If expression 6") {
        run_jit_test(
            R"(
            let x = if false { yield 2 + 3 } else { yield 2 * 3 }
            printout x
            )",
            "6"
        );
    }

    SECTION("If expression 7") {
        run_jit_test(
            R"(
            if true { printout "1" }
            if false { printout "2" }
            if true { printout "3" }
            )",
            "13"
        );
    }

    SECTION("If else if expression 1") {
        run_jit_test(
            R"(
            let var x = 1
            if false {
                x = 2
            } else if true {
                x = 3
            } else {
                x = 4
            }
            printout x
            )",
            "3"
        );
    }

    SECTION("If else if expression 2") {
        run_jit_test(
            R"(
            let x =
            if false {
                yield 2
            } else if false {
                yield 3
            } else {
                yield 4
            }
            printout x
            )",
            "4"
        );
    }
}

TEST_CASE("JIT logical expressions", "[jit]") {
    SECTION("Logical OR") {
        run_jit_test(R"(let x = false printout true or x)", "true");
    }

    SECTION("Logical AND") {
        run_jit_test(R"(let x = true printout false and x)", "false");
    }

    SECTION("Logical expression with blocks") {
        run_jit_test(
            R"(printout true and if true then true else false)",
            "true"
        );
    }

    SECTION("Unary NOT") {
        run_jit_test(R"(let x = false printout not x, !x)", "truetrue");
    }
}

TEST_CASE("JIT while loop expressions", "[jit]") {
    SECTION("Simple while loop") {
        run_jit_test(
            R"(
            let var x = 0
            while false:
                x = x + 1
            printout x
            )",
            "0"
        );
    }

    SECTION("While loop 1") {
        run_jit_test(
            R"(
            let var x = 0
            while x < 5:
                x = x + 1
            printout x
            )",
            "5"
        );
    }

    SECTION("While loop 2") {
        run_jit_test(
            R"(
            let var x = 10
            while x > 0:
                x = x - 2
            printout x
            )",
            "0"
        );
    }

    SECTION("While loop 3") {
        run_jit_test(
            R"(
            let var x = 1
            while x < 100:
                x = x * 2
            printout x
            )",
            "128"
        );
    }

    SECTION("While loop 4") {
        run_jit_test(
            R"(
            let var x = 5
            while x >= 0:
                x = x - 1
            printout x
            )",
            "-1"
        );
    }

    SECTION("While loop 5") {
        run_jit_test(
            R"(
            let var x = 0
            while x < 5:
                printout x
                x = x + 1
            )",
            "01234"
        );
    }

    SECTION("While loop with inner conditional expr") {
        run_jit_test(
            R"(
            let var x = 0
            while true:
                if true:
                    break ()
            printout x
            )",
            "0"
        );
    }

    SECTION("While loop with break") {
        run_jit_test(
            R"(
            let var x = 0
            while x < 5:
                if x == 3:
                    break ()
                x = x + 1
            printout x
            )",
            "3"
        );
    }

    SECTION("While loop short form") {
        run_jit_test(
            R"(
            let var x = 0
            while x < 5 do x = x + 1
            printout x
            )",
            "5"
        );
    }

    SECTION("While loop with continue") {
        run_jit_test(
            R"(
            let var x = 0
            while x < 10:
                x = x + 1
                if x % 2 == 0:
                    continue
                printout x
            )",
            "13579"
        );
    }

    SECTION("FizzBuzz") {
        run_jit_test(
            R"(
            let var i = 1
            while i <= 15:
                if i % 3 == 0 and i % 5 == 0:
                    printout "FizzBuzz\n"
                else if i % 3 == 0:
                    printout "Fizz\n"
                else if i % 5 == 0:
                    printout "Buzz\n"
                else:
                    printout i, "\n"
                i += 1
            )",
            "1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\nBuzz\n11\nFizz\n13\n14"
            "\n"
            "FizzBuzz\n"
        );
    }
}

TEST_CASE("JIT do-while loop expressions", "[jit]") {
    SECTION("Simple do-while loop") {
        run_jit_test(
            R"(
            let var x = 0
            do:
                x = x + 1
            while false
            printout x
            )",
            "1"
        );
    }

    SECTION("Do-while loop 1") {
        run_jit_test(
            R"(
            let var x = 0
            do:
                x = x + 2
            while x < 10
            printout x
            )",
            "10"
        );
    }

    SECTION("Do-while loop 2") {
        run_jit_test(
            R"(
            let var x = 0
            do:
                x = x + 1
                printout x
            while x < 3
            )",
            "123"
        );
    }

    SECTION("Do-while loop 3") {
        run_jit_test(
            R"(
            let var x = 0
            do:
                x = x - 1
                printout x
            while x > 0
            )",
            "-1"
        );
    }

    SECTION("Do-while short form") {
        run_jit_test(
            R"(
            let var x = 0
            do x = x + 1 while false
            printout x
            )",
            "1"
        );
    }

    SECTION("Do-while loop with continue") {
        run_jit_test(
            R"(
            let var x = 0
            do:
                x = x + 1
                if x % 2 == 0:
                    continue
                printout x
            while x < 10
            )",
            "13579"
        );
    }
}

TEST_CASE("JIT non-conditional loop expressions", "[jit]") {
    SECTION("Non-conditional loop 1") {
        run_jit_test(
            R"(
            let x = loop:
                break 1
            printout x
            )",
            "1"
        );
    }

    SECTION("Non-conditional loop 2") {
        run_jit_test(
            R"(
            let var x = 1
            let var i = 0
            let y =
            loop:
                if i > 3:
                    break x
                x = x * 2
                i = i + 1
            printout x, ",", y
            )",
            "16,16"
        );
    }

    SECTION("Nested loops") {
        run_jit_test(
            R"(
            loop:
                printout 1
                loop:
                    printout 2
                    break ()
                    printout 3
                printout 4
                break ()
                printout 5
            printout 6
            )",
            "1246"
        );
    }

    SECTION("Non-conditional loop break propagation") {
        run_jit_test(
            R"(
            let x = loop:
                let var i = 0
                break loop:
                    if i > 3:
                        break i
                    i = i + 1
            printout x
            )",
            "4"
        );
    }

    SECTION("Non-conditional loop with continue") {
        run_jit_test(
            R"(
            let var i = 0
            loop:
                i = i + 1
                if i % 2 == 0:
                    continue
                printout i
                if i > 5:
                    break i
            )",
            "1357"
        );
    }
}

TEST_CASE("JIT pointers", "[jit]") {
    SECTION("Pointer read") {
        run_jit_test(
            R"(
            let x = 10
            let p = @x
            printout unsafe { yield ^p }
            )",
            "10"
        );
    }

    SECTION("Pointer write") {
        run_jit_test(
            R"(
            let var x = 10
            let p = var@x
            unsafe { ^p = 20 }
            printout x
            )",
            "20"
        );
    }

    SECTION("Pointer reassignment") {
        run_jit_test(
            R"(
            let var x = 10
            let var y = 20
            let var p = var@x
            unsafe { ^p = 30 }
            printout x, ",", y, ","
            p = var@y
            unsafe { ^p = 40 }
            printout x, ",", y
            )",
            "30,20,30,40"
        );
    }

    SECTION("Pointer to pointer") {
        run_jit_test(
            R"(
            let var x = 10
            let p1 = var@x
            let p2 = @p1
            unsafe { ^^p2 = 20 }
            printout x
            )",
            "20"
        );
    }

    SECTION("Pointer comparison") {
        run_jit_test(
            R"(
            let var x = 10
            let var y = 20
            let p1 = var@x
            let p2 = var@y
            printout p1 == p2, ",", p1 != p2, ",", p1 == @x, ",", p2 == @y
            )",
            "false,true,true,true"
        );
    }

    SECTION("Pointer comparison different types") {
        run_jit_test(
            R"(
            let var x: i32 = 10
            let var y: f64 = 20.0
            let p1 = var@x
            let p2 = var@y
            printout p1 == p2, ",", p1 != p2
            )",
            "false,true"
        );
    }
}

TEST_CASE("JIT static vars", "[jit]") {
    SECTION("Static variable assigned after declaration") {
        run_jit_test(
            R"(
            static var counter: i32
            counter = 42
            printout counter
            )",
            "42"
        );
    }

    SECTION("Static variable assigned before declaration") {
        run_jit_test(
            R"(
            counter = 37
            static var counter: i32
            printout counter
            )",
            "37"
        );
    }

    SECTION("Static variable with initializer 1") {
        run_jit_test(
            R"(
            static var counter: i32 = 100
            printout counter
            )",
            "100"
        );
    }

    SECTION("Static variable with initializer 2") {
        run_jit_test(
            R"(
            static var counter: i32 = 100
            counter = 200
            printout counter
            )",
            "200"
        );
    }

    SECTION("Static variable with array initializer") {
        run_jit_test(
            R"(
            static var arr = [1, 2, 3]
            printout arr
            )",
            "[1, 2, 3]"
        );
    }

    SECTION("Static variable printed before declaration") {
        run_jit_test(
            R"(
            printout counter
            static var counter: i32 = 50
            )",
            "50"
        );
    }
}

TEST_CASE("JIT functions", "[jit]") {
    SECTION("Simple function") {
        run_jit_test(
            R"(
            func answer() -> i32 => 42
            )"
        );
    }

    SECTION("Function call") {
        run_jit_test(
            R"(
            func answer() -> i32 => 42
            printout answer()
            )",
            "42"
        );
    }

    SECTION("Function braced form") {
        run_jit_test(
            R"(
            func answer() -> i32 {
                return 42
            }
            printout answer()
            )",
            "42"
        );
    }

    SECTION("Function braced form with yield") {
        run_jit_test(
            R"(
            func answer() -> i32 {
                yield 42
            }
            printout answer()
            )",
            "42"
        );
    }

    SECTION("Function indented form") {
        run_jit_test(
            R"(
            func answer() -> i32:
                return 42
            printout answer()
            )",
            "42"
        );
    }

    SECTION("Function with multiple yields") {
        run_jit_test(
            R"(
            func answer() -> i32 {
                yield 17
                yield 42
                yield 37
            }
            printout answer()
            )",
            "37"
        );
    }

    SECTION("Function with no return value") {
        run_jit_test(
            R"(
            func greet() {
                printout "Hello!"
            }
            greet()
            )",
            "Hello!"
        );
    }

    SECTION("Function with parameters 1") {
        run_jit_test(
            R"(
            func increment(x: i32) -> i32 {
                return x + 1
            }
            printout increment(5)
            )",
            "6"
        );
    }

    SECTION("Function with parameters 2") {
        run_jit_test(
            R"(
            func add(x: i32, y: i32) -> i32 {
                return x + y
            }
            printout add(5, 10)
            )",
            "15"
        );
    }

    SECTION("Function overloading 1") {
        run_jit_test(
            R"(
            func greet() {
                printout "Hello, World!"
            }
            func greet(name: str) {
                printout "Hello, ", name, "!"
            }
            greet()
            printout "\n"
            greet("Alice")
            )",
            "Hello, World!\nHello, Alice!"
        );
    }

    SECTION("Function overloading 2") {
        run_jit_test(
            R"(
            func increment(x: i32) -> i32 {
                return x + 1
            }
            func increment(x: f64) -> f64 {
                return x + 1.0
            }
            printout increment(5), ",", increment(5.5)
            )",
            "6,6.5"
        );
    }

    SECTION("Function with pointer parameter") {
        run_jit_test(
            R"(
            func set_to_ten(x: var@i32) {
                unsafe { ^x = 10 }
            }
            let var y = 0
            set_to_ten(var@y)
            printout y
            )",
            "10"
        );
    }

    SECTION("Function pointer call") {
        run_jit_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let func_ptr = add
        let result: i32 = func_ptr(1, 2)
        printout result
        )",
            "3"
        );
    }

    SECTION("Overloaded function pointer call") {
        run_jit_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        func add(a: f64, b: f64) -> f64 => a + b
        let func_ptr = add
        printout func_ptr(1, 3), ", ", func_ptr(2.5, 2.5)
        )",
            "4, 5"
        );
    }
}

TEST_CASE("JIT arrays", "[jit]") {
    SECTION("Array creation and access") {
        run_jit_test(
            R"(
            let arr = [1, 2, 3, 4, 5]
            printout arr[0], ",", arr[2], ",", arr[4]
            )",
            "1,3,5"
        );
    }

    SECTION("Array assignment") {
        run_jit_test(
            R"(
            let var arr = [10, 20, 30]
            arr[1] = 99
            printout arr[0], ",", arr[1], ",", arr[2]
            )",
            "10,99,30"
        );
    }

    SECTION("Array size") {
        run_jit_test(
            R"(
            let arr = [1, 2, 3, 4, 5, 6]
            printout "sizeof arr: ", sizeof typeof(arr)
            printout "\nsizeof arr[0]: ", sizeof typeof(arr[0])
            printout "\nlength arr: ", sizeof typeof(arr) / sizeof typeof(arr[0])
            )",
            "sizeof arr: 24\nsizeof arr[0]: 4\nlength arr: 6"
        );
    }

    SECTION("Multi-dimensional arrays") {
        run_jit_test(
            R"(
            let matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]
            printout matrix[0][0], ",", matrix[1][1], ",", matrix[2][2]
            )",
            "1,5,9"
        );
    }

    SECTION("Array runtime bounds check (>= length)") {
        run_jit_test(
            R"(
            let arr = [1, 2, 3]
            printout arr[3]
            )",
            std::nullopt,
            101
        );
    }

    SECTION("Array runtime bounds check (< 0)") {
        run_jit_test(
            R"(
            let arr = [1, 2, 3]
            printout arr[-1]
            )",
            std::nullopt,
            101
        );
    }

    SECTION("Array shallow copy") {
        run_jit_test(
            R"(
            let var a = [1, 1]
            let var b = a
            a[0] = 2
            b[1] = 3
            printout a[0], a[1], b[0], b[1]
            )",
            "2113"
        );
    }

    SECTION("Print array") {
        run_jit_test(
            R"(
            let arr = [8, 16, 32, 64]
            printout arr
            )",
            "[8, 16, 32, 64]"
        );
    }

    SECTION("Print empty array") {
        run_jit_test(
            R"(
            let arr = []
            printout arr
            )",
            "[]"
        );
    }

    SECTION("Print nested arrays") {
        run_jit_test(
            R"(
            let arr = [[1, 2], [3, 4], [5, 6]]
            printout arr
            )",
            "[[1, 2], [3, 4], [5, 6]]"
        );
    }
}

TEST_CASE("JIT alloc and dealloc", "[jit]") {
    SECTION("Alloc and dealloc integer") {
        run_jit_test(
            R"(
            let p = alloc i32
            printout unsafe { 
                ^p = 123
                yield ^p 
                dealloc p
            }
            )",
            "123"
        );
    }

    SECTION("Alloc and dealloc float") {
        run_jit_test(
            R"(
            let p = alloc f64 with 3.14
            printout unsafe { yield ^p }
            unsafe { dealloc p }
            )",
            "3.14"
        );
    }

    SECTION("Alloc and dealloc array") {
        run_jit_test(
            R"(
            let p = alloc with [1, 2, 3, 4, 5]
            let arr = unsafe { yield ^p }
            printout arr[0], ",", arr[4]
            unsafe { dealloc p }
            )",
            "1,5"
        );
    }

    SECTION("Alloc dynamic array") {
        run_jit_test(
            R"(
            let size = 4
            let p = alloc for size of i32
            unsafe {
                ^p = [10, 20, 30, 40]
                printout p[0], ",", p[3]
                dealloc p
            }
            )",
            "10,40"
        );
    }

    SECTION("Alloc negative size") {
        run_jit_test(
            R"(
            let size = -5
            let p = alloc for size of i32
            )",
            std::nullopt,
            101
        );
    }

    SECTION("Alloc in block") {
        run_jit_test(
            R"(
            block {
                let p = alloc i32 with 42
                printout unsafe { yield ^p }
                unsafe { dealloc p }
            }
            printout " done"
            )",
            "42 done"
        );
    }
}

TEST_CASE("JIT namespaces", "[jit]") {
    SECTION("Namespace declaration and access") {
        run_jit_test(
            R"(
            namespace math {
                func add(a: i32, b: i32) -> i32 => a + b
            }
            printout math::add(3, 4)
            )",
            "7"
        );
    }

    SECTION("Nested namespaces") {
        run_jit_test(
            R"(
            namespace outer {
                namespace inner {
                    func greet() -> str => "Hello from inner!"
                }
            }
            printout outer::inner::greet()
            )",
            "Hello from inner!"
        );
    }

    SECTION("Namespace with variables") {
        run_jit_test(
            R"(
            namespace config {
                static var version: str
            }
            config::version = "1.0.0"
            printout config::version
            )",
            "1.0.0"
        );
    }

    SECTION("Namespace with functions and variables") {
        run_jit_test(
            R"(
            namespace utils {
                static var count: i32
                func increment() {
                    count += 1
                }
                func get_count() -> i32 => count
            }
            utils::count = 0
            utils::increment()
            utils::increment()
            printout utils::get_count()
            )",
            "2"
        );
    }

    SECTION("Namespace access before declaration") {
        run_jit_test(
            R"(
            printout math::add(1, 2)
            namespace math {
                func add(a: i32, b: i32) -> i32 => a + b
            }
            )",
            "3"
        );
    }

    SECTION("Namespace with overloaded functions") {
        run_jit_test(
            R"(
            namespace math {
                func add(a: i32, b: i32) -> i32 => a + b
                func add(a: f64, b: f64) -> f64 => a + b
            }
            printout math::add(1, 2), ", ", math::add(1.5, 2.5)
            )",
            "3, 4"
        );
    }
}
