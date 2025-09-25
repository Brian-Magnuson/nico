#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <llvm/Support/Error.h>

#include "../src/checker/global_checker.h"
#include "../src/checker/local_checker.h"
#include "../src/codegen/code_generator.h"
#include "../src/common/code_file.h"
#include "../src/compiler/jit.h"
#include "../src/frontend/context.h"
#include "../src/frontend/frontend.h"
#include "../src/lexer/lexer.h"
#include "../src/lexer/token.h"
#include "../src/logger/logger.h"
#include "../src/parser/ast.h"
#include "../src/parser/parser.h"
#include "utils/test_utils.h"

// void run_compile_test(
//     Lexer& lexer, Parser& parser, GlobalChecker& global_checker,
//     LocalChecker& local_checker, CodeGenerator& codegen,
//     std::unique_ptr<IJit>& jit, std::string_view source,
//     std::optional<std::string_view> expected_output = std::nullopt,
//     std::optional<int> expected_return_code = std::nullopt
// ) {
//     auto file = make_test_code_file(source);

//     auto tokens = lexer.scan(file);
//     auto ast = parser.parse(std::move(tokens));
//     global_checker.check(ast);
//     local_checker.check(ast);
//     REQUIRE(codegen.generate_executable_ir(ast));
//     auto output = codegen.eject();
//     auto error =
//         jit->add_module(std::move(output.module), std::move(output.context));

//     std::optional<llvm::Expected<int>> return_code;

//     auto [out, err] = capture_stdout(
//         [&]() { return_code = jit->run_main(0, nullptr); },
//         4096
//     );

//     if (expected_output) {
//         CHECK(out == *expected_output);
//     }
//     if (expected_return_code) {
//         REQUIRE(return_code.has_value());
//         REQUIRE(*return_code);
//         CHECK(return_code->get() == *expected_return_code);
//     }
// }

void run_jit_test(
    std::string_view source,
    std::optional<std::string_view> expected_output = std::nullopt,
    std::optional<int> expected_return_code = std::nullopt,
    bool print_ir = false
) {
    auto file = make_test_code_file(source);

    Logger::inst().set_printing_enabled(true);

    Frontend frontend;
    if (expected_return_code.has_value() && *expected_return_code != 0) {
        // If we expect a non-zero return code, enable panic recovery to avoid
        // terminating the program.
        frontend.set_panic_recoverable(true);
    }

    frontend.set_ir_printing_enabled(print_ir);

    std::unique_ptr<Context>& context = frontend.compile(file, false);
    REQUIRE(context->status == Context::Status::OK);

    std::unique_ptr<IJit> jit = std::make_unique<SimpleJit>();
    auto jit_err = jit->add_module(
        std::move(context->ir_module),
        std::move(context->llvm_context)
    );
    REQUIRE(!jit_err);

    std::optional<llvm::Expected<int>> return_code;
    auto [out, err] = capture_stdout(
        [&]() { return_code = jit->run_main(0, nullptr); },
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

    SECTION("Divide by zero") {
        run_jit_test(R"(printout 1 / 0)", std::nullopt, 101);
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
}

TEST_CASE("JIT block expressions", "[jit]") {
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
            "let var x = 1 let var y = 2 x = block { let var y = x + 3 yield y "
            "} printout x, \",\", y",
            "4,2"
        );
    }

    SECTION("Block expression 4") {
        run_jit_test(
            "let var x = 1 x = block { yield 2 * block { yield 3 } } printout "
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
}
