#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "nico/frontend/components/global_checker.h"
#include "nico/frontend/components/lexer.h"
#include "nico/frontend/components/local_checker.h"
#include "nico/frontend/components/parser.h"
#include "nico/shared/diagnostics.h"

#include "test_utils.h"

using nico::Err;

/**
 * @brief Run a checker test with the given source code and expected error.
 *
 * Pass an error code to assert that that the checker produces that error first.
 * If no error code is given, this test asserts that no errors are produced.
 *
 * Use print_errors to control whether errors are printed.
 * If print_errors is not given, printing will be enabled when there are no
 * expected errors and will be disabled when there are expected errors.
 *
 * @param src_code The source code to test.
 * @param expected_error (Optional) The expected error, if any.
 * @param print_errors (Optional) Whether to print errors. If not provided, will
 * be based on the presence of expected_error (see description).
 */
void run_checker_stmt_test(
    std::string_view src_code,
    std::optional<Err> expected_error = std::nullopt,
    std::optional<bool> print_errors = std::nullopt,
    bool print_tree = false
) {
    // If there is no expected error, we enable printing to look for unexpected
    // errors.
    bool printing_enabled = print_errors.value_or(!expected_error.has_value());
    nico::Diagnostics::inst().set_printing_enabled(printing_enabled);

    auto& errors = nico::Diagnostics::inst().get_errors();
    auto context = std::make_unique<nico::FrontendContext>();
    auto file = nico::make_test_code_file(src_code);
    nico::Lexer::scan(context, file);
    nico::Parser::parse(context);
    nico::GlobalChecker::check(context);
    if (errors.empty())
        nico::LocalChecker::check(context);

    if (print_tree) {
        std::cout << context->symbol_tree->to_tree_string() << "\n";
    }

    if (expected_error.has_value()) {
        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == expected_error.value());
    }
    else {
        CHECK(errors.empty());
    }

    context->initialize();
    nico::Diagnostics::inst().reset();
}

TEST_CASE("Check variable declarations", "[checker]") {
    SECTION("Valid local variable declarations 1") {
        run_checker_stmt_test("let a = 1");
    }

    SECTION("Valid local variable declarations 2") {
        run_checker_stmt_test("let a: i32 = 1");
    }

    SECTION("Typeof annotation") {
        run_checker_stmt_test("let a = 1 let b: typeof(a) = 2");
    }

    SECTION("Let type mismatch i32 and bool") {
        run_checker_stmt_test("let a: i32 = true", Err::LetTypeMismatch);
    }

    SECTION("Let type mismatch i32 and bool 2") {
        run_checker_stmt_test(
            "let a = true let b: i32 = a",
            Err::LetTypeMismatch
        );
    }

    SECTION("Let type mismatch i32 and f64") {
        run_checker_stmt_test("let a: i32 = 1.0", Err::LetTypeMismatch);
    }

    SECTION("Let type mismatch i32 and i8") {
        run_checker_stmt_test("let var a: i32 = 1_i8", Err::LetTypeMismatch);
    }

    SECTION("Let type mismatch arrays of different sizes") {
        run_checker_stmt_test("let a: [i32; 3] = [1, 2]", Err::LetTypeMismatch);
    }

    SECTION("Undeclared identifier") {
        run_checker_stmt_test("let a = b", Err::NameNotFound);
    }

    SECTION("Immutable variables") {
        run_checker_stmt_test(
            "let a = 1 let b = 2 a = b",
            Err::AssignToImmutable
        );
    }

    SECTION("Variable name conflict") {
        run_checker_stmt_test("let a = 1 let a = 2", Err::NameAlreadyExists);
    }

    SECTION("Primitive type name conflict") {
        run_checker_stmt_test("let i32 = 1", Err::NameIsReserved);
    }

    SECTION("Variable immutable without initializer") {
        run_checker_stmt_test("let a: i32", Err::ImmutableWithoutInitializer);
    }
}

TEST_CASE("Check variable nullptr declarations", "[checker]") {
    SECTION("Nullptr assignment single pointer") {
        run_checker_stmt_test("let var a: @i32 = nullptr");
    }

    SECTION("Nullptr assignment double pointer") {
        run_checker_stmt_test("let var a: @@i32 = nullptr");
    }

    SECTION("Nullptr assignment var pointer") {
        run_checker_stmt_test("let var a: var@i32 = nullptr");
    }

    SECTION("Nullptr assignment deep pointer") {
        run_checker_stmt_test("let var a: var@var@var@var@var@i32 = nullptr");
    }

    SECTION("Let type mismatch i32 and nullptr") {
        run_checker_stmt_test("let a: i32 = nullptr", Err::LetTypeMismatch);
    }
}

TEST_CASE("Check variable anyptr declarations", "[checker]") {
    SECTION("Anyptr assignment with nullptr") {
        run_checker_stmt_test("let var a: anyptr = nullptr");
    }

    SECTION("Anyptr assignment with raw typed pointer") {
        run_checker_stmt_test("let var a = 1 let p = var@a let q: anyptr = p");
    }

    SECTION("Anyptr assignment with alloc") {
        run_checker_stmt_test("let q: anyptr = alloc i32");
    }

    SECTION("Anyptr assignment with anyptr") {
        run_checker_stmt_test("let var a: anyptr = nullptr let b: anyptr = a");
    }

    SECTION("Let type mismatch anyptr and i32") {
        run_checker_stmt_test("let a: anyptr = 1", Err::LetTypeMismatch);
    }

    SECTION("Let type mismatch i32 and anyptr") {
        run_checker_stmt_test(
            "let q: anyptr = alloc i32 let a: i32 = q",
            Err::LetTypeMismatch
        );
    }

    SECTION("Let type mismatch anyptr and non-var ptr") {
        run_checker_stmt_test(
            "let a = 1 let p = @a let q: anyptr = p",
            Err::LetTypeMismatch
        );
    }
}

TEST_CASE("Check variable void declarations", "[checker]") {
    SECTION("Void assignment") {
        run_checker_stmt_test("let var a: void = void");
    }

    SECTION("Void compatible with unit") {
        run_checker_stmt_test("let var a: void = ()");
    }

    SECTION("Unit compatible with void") {
        run_checker_stmt_test("let var a: () = void");
    }

    SECTION("Let type mismatch void and i32") {
        run_checker_stmt_test("let a: void = 1", Err::LetTypeMismatch);
    }

    SECTION("Let type mismatch void and non-unit tuple") {
        run_checker_stmt_test("let a: void = (1, 2)", Err::LetTypeMismatch);
    }

    SECTION("Let type mismatch non-unit tuple and void") {
        run_checker_stmt_test("let a: (i32) = void", Err::LetTypeMismatch);
    }
}

TEST_CASE("Check static variable declarations", "[checker]") {
    SECTION("Static variable assigned after declaration") {
        run_checker_stmt_test("static var a: i32 a = 37");
    }

    SECTION("Static variable assigned before declaration") {
        run_checker_stmt_test("a = 37 static var a: i32");
    }

    SECTION("Immutable static variable 1") {
        run_checker_stmt_test(
            "static a: i32 = 0 a = 73",
            Err::AssignToImmutable
        );
    }

    SECTION("Immutable static variable 2") {
        run_checker_stmt_test(
            "a = 73 static a: i32 = 0",
            Err::AssignToImmutable
        );
    }

    SECTION("Static variable name conflict 1") {
        run_checker_stmt_test(
            "static var a: i32 static var a: i32",
            Err::NameAlreadyExists
        );
    }

    SECTION("Static variable name conflict 1") {
        run_checker_stmt_test(
            "let var a: i32 static var a: i32",
            Err::NameAlreadyExists
        );
    }

    SECTION("Static variable with initializer") {
        run_checker_stmt_test("static var a: i32 = 42");
    }

    SECTION("Static variable immutable without initializer") {
        run_checker_stmt_test(
            "static s: i32",
            Err::ImmutableWithoutInitializer
        );
    }
}

TEST_CASE("Check dealloc statements", "[checker]") {
    SECTION("Valid dealloc statement 1") {
        run_checker_stmt_test("let a: @i32 = alloc i32 unsafe { dealloc a }");
    }

    SECTION("Valid dealloc statement 2") {
        run_checker_stmt_test(
            "let var a: var@i32 = alloc i32 unsafe { dealloc a }"
        );
    }

    SECTION("Valid dealloc statement 3") {
        run_checker_stmt_test(
            "let a: @i32 = alloc i32 let b = a unsafe { dealloc b }"
        );
    }

    SECTION("Valid dealloc statement 4") {
        run_checker_stmt_test(
            "let a: @[i32; 5] = alloc [i32; 5] with [1,2,3,4,5] "
            "unsafe { dealloc a }"
        );
    }

    SECTION("Valid dealloc statement 5") {
        run_checker_stmt_test(
            "let a: @[i32; ?] = alloc for 10 of i32 unsafe { dealloc a }"
        );
    }

    SECTION("Dealloc non-pointer") {
        run_checker_stmt_test(
            "let a = 1 unsafe { dealloc a }",
            Err::DeallocNonRawPointer
        );
    }

    SECTION("Dealloc nullptr") {
        run_checker_stmt_test(
            "let a = nullptr unsafe { dealloc a }",
            Err::DeallocNullptr
        );
    }

    SECTION("Dealloc outside unsafe") {
        run_checker_stmt_test(
            "let a: @i32 = alloc i32 dealloc a",
            Err::DeallocOutsideUnsafeBlock
        );
    }
}

TEST_CASE("Check print statements", "[checker]") {
    SECTION("Print hello world") {
        run_checker_stmt_test("printout \"Hello, World!\"");
    }
}

TEST_CASE("Check function declarations", "[checker]") {
    SECTION("Valid function declaration braced form") {
        run_checker_stmt_test(R"(
        func add(a: i32, b: i32) -> i32 {
            return a + b
        }
        )");
    }

    SECTION("Valid function declaration indented form") {
        run_checker_stmt_test(R"(
        func add(a: i32, b: i32) -> i32:
            return a + b
        )");
    }

    SECTION("Valid function declaration short form") {
        run_checker_stmt_test("func add(a: i32, b: i32) -> i32 => a + b");
    }

    SECTION("Function with reserved name") {
        run_checker_stmt_test(
            "func bool(a: i32) -> i32 { return a }",
            Err::NameIsReserved
        );
    }

    SECTION("Variable name already exists") {
        run_checker_stmt_test(
            "let add = 1 "
            "func add(a: i32, b: i32) -> i32 { return a + b }",
            Err::NameAlreadyExists
        );
    }

    SECTION("Variable name OK") {
        run_checker_stmt_test(
            "block { let add = 1 } "
            "func add(a: i32, b: i32) -> i32 => a + b "
            "block { let add = 1 }"
        );
    }

    SECTION("Function name already exists") {
        run_checker_stmt_test(
            "func add(a: i32, b: i32) -> i32 { return a + b } "
            "let add = 1",
            Err::NameAlreadyExists
        );
    }

    SECTION("Duplicate function parameter name") {
        run_checker_stmt_test(
            "func add(a: i32, a: i32) -> i32 { return a + a }",
            Err::DuplicateFunctionParameterName
        );
    }

    SECTION("Function parameter default argument type mismatch") {
        run_checker_stmt_test(
            "func add(a: i32 = true, b: i32) -> i32 { return a + b }",
            Err::DefaultArgTypeMismatch
        );
    }

    SECTION("Function immutable parameter assignment") {
        run_checker_stmt_test(
            "func add(a: i32, b: i32) -> i32 { a = 2 return a + b }",
            Err::AssignToImmutable
        );
    }

    SECTION("Function return type mismatch") {
        run_checker_stmt_test(
            "func add(a: i32, b: i32) -> i32 { return true }",
            Err::FunctionReturnTypeMismatch
        );
    }

    SECTION("Extern variadic function") {
        run_checker_stmt_test("extern ex1 { func add(...) -> i32 }");
    }

    SECTION("Non-extern variadic function") {
        run_checker_stmt_test(
            "func add(...) -> i32 { return 0 }",
            Err::NonExternVariadicFunc
        );
    }
}

TEST_CASE("Check function overload declarations", "[checker]") {
    SECTION("Valid overloads 1") {
        run_checker_stmt_test(R"(
        func add(a: i32, b: i32) -> i32 => a + b
        func add(a: f64, b: f64) -> f64 => a + b
        )");
    }

    SECTION("Valid overloads 2") {
        run_checker_stmt_test(R"(
        func add(a: i32, b: i32, c: i32) -> i32 => a + b + c
        func add(a: i32, b: i32) -> i32 => a + b
        )");
    }

    SECTION("Valid overloads 3") {
        run_checker_stmt_test(R"(
        func add(b: i32) -> i32 => b + 1
        func add(a: i32) -> i32 => a + 1
        )");
    }

    SECTION("Valid overloads 4") {
        run_checker_stmt_test(R"(
        func add(a: i32, b: i32, c: i32 = 0) -> i32 => a + b + c
        func add(a: i32) -> i32 => a + 1
        )");
    }

    SECTION("Valid overloads 5") {
        run_checker_stmt_test(R"(
            func add(a: i32, b: i32, c: i32 = 0) -> i32 => a + b + c
            func add(a: i32, b: f64) -> i32 => 0
        )");
    }

    SECTION("Valid overloads 6") {
        run_checker_stmt_test(
            R"(
        func add(a: i32, b: i32 = 0, c: i32 = 0) -> i32 => a + b + c
        func add(a: i32, d: i32 = 0) -> i32 => a + d
        )"
        );
    }

    SECTION("Many valid overloads") {
        run_checker_stmt_test(
            R"(
        func add() -> i32 => 0
        func add(a: i32) -> i32 => a + 1
        func add(a: i32, b: i32) -> i32 => a + b
        func add(a: i32, b: i32, c: i32) -> i32 => a + b + c
        func add(a: i32, b: i32, c: i32, d: i32) -> i32 => a + b + c + d
        func add(a: f64, b: f64) -> f64 => a + b
        func add(a: f64, b: f64, c: f64) -> f64 => a + b + c
        func add(a: f64, b: f64, c: f64, d: f64) -> f64 => a + b + c + d
        )"
        );
    }

    SECTION("Overload conflicts 1") {
        run_checker_stmt_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        func add(a: i32, b: i32) -> i32 => a - b
        )",
            Err::FunctionOverloadConflict
        );
    }

    SECTION("Overload conflicts 2") {
        run_checker_stmt_test(
            R"(
        func add(a: i32, b: i32, c: i32 = 0) -> i32 => a + b + c
        func add(a: i32, b: i32) -> i32 => a + b
        )",
            Err::FunctionOverloadConflict
        );
    }

    SECTION("Overload conflicts 3") {
        run_checker_stmt_test(
            R"(
        func add(a: i32, b: f64) -> i32 => 0
        func add(b: f64, a: i32) -> i32 => 0
        )",
            Err::FunctionOverloadConflict
        );
    }

    SECTION("Overload conflicts 4") {
        run_checker_stmt_test(
            R"(
        func add(a: i32) -> i32 => a + 1
        func add(a: i32, b: i32) -> i32 => a + b
        func add(a: i32, b: i32 = 0) -> i32 => a + b
        )",
            Err::FunctionOverloadConflict
        );
    }

    SECTION("Overload conflicts 5") {
        run_checker_stmt_test(
            R"(
        func add() -> i32 => 0
        func add(a: i32 = 0) -> i32 => a + 1
        )",
            Err::FunctionOverloadConflict
        );
    }
}

TEST_CASE("Check function call", "[checker]") {
    SECTION("Valid function call") {
        run_checker_stmt_test(R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let result: i32 = add(1, 2)
        )");
    }

    SECTION("Function call no arguments") {
        run_checker_stmt_test(R"(
        func get_five() -> i32 => 5
        let result: i32 = get_five()
        )");
    }

    SECTION("Function call undeclared name") {
        run_checker_stmt_test("let result = add(1, 2)", Err::NameNotFound);
    }

    SECTION("Function call wrong number of arguments") {
        run_checker_stmt_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let result: i32 = add(1)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("Function call too many arguments") {
        run_checker_stmt_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let result: i32 = add(1, 2, 3)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("Not a callable") {
        run_checker_stmt_test(
            R"(
        let add = 1
        let result = add(1, 2)
        )",
            Err::NotACallable
        );
    }

    SECTION("Function parameter type mismatch") {
        run_checker_stmt_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let result: i32 = add(1, true)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("Function call error in argument") {
        run_checker_stmt_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let result: i32 = add(1, undeclared_var)
        )",
            Err::NameNotFound
        );
    }

    SECTION("Function call with default arguments") {
        run_checker_stmt_test(R"(
        func add(a: i32, b: i32 = 2) -> i32 => a + b
        let result1: i32 = add(3)
        let result2: i32 = add(3, 4)
        )");
    }

    SECTION("Function call with named arguments") {
        run_checker_stmt_test(R"(
        func add(a: i32, b: i32, c: i32) -> i32 => a + b + c
        let result1: i32 = add(a: 1, b: 2, c: 3)
        let result2: i32 = add(c: 3, a: 1, b: 2)
        )");
    }

    SECTION("Function call with named arguments and defaults") {
        run_checker_stmt_test(R"(
        func add(a: i32, b: i32 = 2, c: i32) -> i32 => a + b + c
        let result: i32 = add(1, c: 3)
        )");
    }

    SECTION("Function call with invalid named argument") {
        run_checker_stmt_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let result: i32 = add(a: 1, c: 2)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("Function pointer call") {
        run_checker_stmt_test(R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let func_ptr = add
        let result: i32 = func_ptr(1, 2)
        )");
    }

    SECTION("Function call before declaration") {
        run_checker_stmt_test(
            R"(
        let result: i32 = add(1, 2)
        func add(a: i32, b: i32) -> i32 => a + b
        )"
        );
    }

    SECTION("Non extern function without body") {
        run_checker_stmt_test(
            "func add(a: i32, b: i32) -> i32",
            Err::FuncWithoutBody
        );
    }

    SECTION("Calling variadic function") {
        run_checker_stmt_test(
            R"(
        extern ex1 {
            func add(a: i32, ...) -> i32
        }
        let result1 = ex1::add(1)
        let result2 = ex1::add(1, 2)
        let result3 = ex1::add(1, 2, 3)
        )"
        );
    }

    SECTION("Variadic function call with wrong first argument") {
        run_checker_stmt_test(
            R"(
        extern ex1 {
            func add(a: i32, ...) -> i32
        }
        let result = ex1::add(true)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("Variadic function not enough required arguments") {
        run_checker_stmt_test(
            R"(
        extern ex1 {
            func add(a: i32, b: i32, ...) -> i32
        }
        let result = ex1::add(1)
        )",
            Err::NoMatchingFunctionOverload
        );
    }
}

TEST_CASE("Check function overload calls", "[checker]") {
    SECTION("Valid overload call 1") {
        run_checker_stmt_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        func add(a: f64, b: f64) -> f64 => a + b
        let result1: i32 = add(1, 2)
        let result2: f64 = add(1.0, 2.0)
        )"
        );
    }

    SECTION("Valid overload call 2") {
        run_checker_stmt_test(R"(
            func f() -> bool => true
            func f(p1: i32) -> i32 => 1
            func f(p1: i32, p2: i32) -> f64 => 2.0
            let a: bool = f()
            let b: i32 = f(10)
            let c: f64 = f(10, 20)
        )");
    }

    SECTION("Valid overload call 3") {
        run_checker_stmt_test(R"(
        func f(a: i32) -> i32 => 0
        func f(b: i32) -> f64 => 0.0
        let a: i32 = f(a: 10)
        let b: f64 = f(b: 20)
        )");
    }

    SECTION("Valid overload call 4") {
        run_checker_stmt_test(R"(
        func f(a: i32, b: i32) -> i32 => 0
        func f(a: i32, c: i32) -> f64 => 0.0
        let a: i32 = f(a: 10, b: 20)
        let b: f64 = f(a: 30, c: 40)
        )");
    }

    SECTION("Valid overload call 5") {
        run_checker_stmt_test(R"(
        func f(a: i32, b: i32) -> i32 => 0
        func f(a: i32) -> f64 => 0.0
        let a: i32 = f(a: 10, b: 20)
        let b: f64 = f(a: 30)
        )");
    }

    SECTION("Ambiguous call 1") {
        run_checker_stmt_test(
            R"(
        func add(a: i32, b: i32 = 2) -> i32 => a + b
        func add(a: i32, c: i32 = 3) -> i32 => a + c
        let result: i32 = add(1)
        )",
            Err::MultipleMatchingFunctionOverloads
        );
    }

    SECTION("Ambiguous call 2") {
        run_checker_stmt_test(
            R"(
        func f(a: i32, b: i32) -> i32 => 0
        func f(a: i32, c: i32) -> f64 => 0.0
        let result: i32 = f(1, 2)
        )",
            Err::MultipleMatchingFunctionOverloads
        );
    }

    SECTION("Many matching overloads") {
        run_checker_stmt_test(
            R"(
        func f(a: i32) -> i32 => 0
        func f(b: i32) -> i32 => 0
        func f(c: i32) -> i32 => 0
        func f(d: i32) -> i32 => 0
        func f(e: i32) -> i32 => 0
        func f(g: i32) -> i32 => 0
        let result: i32 = f(0)
        )",
            Err::MultipleMatchingFunctionOverloads
        );
    }

    SECTION("No matching overload 1") {
        run_checker_stmt_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        func add(a: f64, b: f64) -> f64 => a + b
        let result: i32 = add(1, 2.0)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("No matching overload 2") {
        run_checker_stmt_test(
            R"(
        func f() -> i32 => 0
        func f(a: i32) -> i32 => 0
        let result: i32 = f(1, 2)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("No matching overload 3") {
        run_checker_stmt_test(
            R"(
        func f(a: i32) -> i32 => 0
        func f(b: i32) -> i32 => 0
        let result: i32 = f(c: 1)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("Many non-matching overloads") {
        run_checker_stmt_test(
            R"(
        func f(a: i32) -> i32 => 0
        func f(b: i32) -> i32 => 0
        func f(c: i32) -> i32 => 0
        func f(d: i32) -> i32 => 0
        func f(e: i32) -> i32 => 0
        func f(g: i32) -> i32 => 0
        let result: i32 = f(n: 1)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("Function pointer overload call 1") {
        run_checker_stmt_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        func add(a: f64, b: f64) -> f64 => a + b
        let func_ptr = add
        let result1: i32 = func_ptr(1, 2)
        let result2: f64 = func_ptr(1.0, 2.0)
        )"
        );
    }

    SECTION("Function pointer overload call 2") {
        run_checker_stmt_test(
            R"(
        func f(a: i32) -> i32 {
            let b = a + 1
            return b
        }
        func f(a: f64) -> f64 {
            let b = a + 1.0
            return b
        }
        let func_ptr = f
        let result1: i32 = func_ptr(10)
        let result2: f64 = func_ptr(10.0)
        )"
        );
    }
}

TEST_CASE("Check namespace declarations", "[checker]") {
    SECTION("Valid namespace declaration") {
        run_checker_stmt_test(R"(
        namespace ns {
            static var x: i32
            func add(a: i32, b: i32) -> i32 => a + b
        }
        )");
    }

    SECTION("Namespace name already exists") {
        run_checker_stmt_test(
            R"(
        namespace ns {
            static var x: i32
        }
        namespace ns {
            func add(a: i32, b: i32) -> i32 => a + b
        }
        )",
            Err::NameAlreadyExists
        );
    }

    SECTION("Namespace name conflicts with variable") {
        run_checker_stmt_test(
            R"(
        let ns = 1
        namespace ns {
            static var x: i32
        }
        )",
            Err::NameAlreadyExists
        );
    }

    SECTION("Namespace name conflicts with function") {
        run_checker_stmt_test(
            R"(
        func ns() -> i32 => 0
        namespace ns {
            static var x: i32
        }
        )",
            Err::NameAlreadyExists
        );
    }

    SECTION("Namespace name is reserved") {
        run_checker_stmt_test(
            R"(
        namespace bool {
            static var x: i32
        }
        )",
            Err::NameIsReserved
        );
    }

    SECTION("Reference variable in namespace") {
        run_checker_stmt_test(
            R"(
        ns::x = 1
        namespace ns {
            static var x: i32
        }
        )"
        );
    }

    SECTION("Reference function in namespace") {
        run_checker_stmt_test(
            R"(
        let result = ns::add(1, 2)
        namespace ns {
            func add(a: i32, b: i32) -> i32 => a + b
        }
        )"
        );
    }

    SECTION("Name reference in nested namespace") {
        run_checker_stmt_test(
            R"(
        let result = ns::inner::add(1, 2)
        namespace ns {
            namespace inner {
                func add(a: i32, b: i32) -> i32 => a + b
            }
        }
        )"
        );
    }

    SECTION("Namespace tree search 1") {
        run_checker_stmt_test(
            R"(
        namespace ns1:
          namespace ns2:
            namespace ns3:
              func add(a: i32, b: i32) -> i32 => a + b
              func test() -> i32:
                let result0 = add(1, 2)
                let result1 = ns3::add(1, 2)
                let result2 = ns2::ns3::add(3, 4)
                let result3 = ns1::ns2::ns3::add(5, 6)
                return result0 + result1 + result2 + result3
        )"
        );
    }

    SECTION("Namespace tree search 2") {
        run_checker_stmt_test(
            R"(
        namespace ns1:
          namespace ns2:
            func add(a: i32, b: i32) -> i32 => a + b
          namespace ns3:
            func test() -> i32:
              let result1 = ns2::add(1, 2)
              let result2 = ns1::ns2::add(3, 4)
              return result1 + result2
        )"
        );
    }

    SECTION("Namespace tree search not found 1") {
        run_checker_stmt_test(
            R"(
        namespace ns1:
          namespace ns2:
            func add(a: i32, b: i32) -> i32 => a + b
          namespace ns3:
            func test() -> i32:
              let result = subtract(1, 2)
              return result
        )",
            Err::NameNotFound
        );
    }

    SECTION("Namespace tree search not found 2") {
        run_checker_stmt_test(
            R"(
        namespace ns1:
          namespace ns2:
            func add(a: i32, b: i32) -> i32 => a + b
          namespace ns3:
            func test() -> i32:
              let result = add(1, 2)
              return result
        )",
            Err::NameNotFound
        );
    }
}

TEST_CASE("Check extern block declarations", "[checker]") {
    SECTION("Valid extern block declaration") {
        run_checker_stmt_test(R"(
        extern "C" ex1 {
            func add(a: i32, b: i32) -> i32
            func print_message(msg: &str) -> ()
        }
        )");
    }

    SECTION("Extern block with body") {
        run_checker_stmt_test(
            R"(
        extern "C" ex2 {
            func add(a: i32, b: i32) -> i32 {
                return a + b
            }
        }
        )",
            Err::ExternBlockFuncWithBody
        );
    }

    SECTION("Extern block with non-extern function") {
        run_checker_stmt_test(
            R"(
        extern "C" ex3 {
            func add(a: i32, b: i32) -> i32 => a + b
        }
        )",
            Err::ExternBlockFuncWithBody
        );
    }

    SECTION("Extern block referencing function") {
        run_checker_stmt_test(
            R"(
        let result = ex1::add(1, 2)
        extern "C" ex1 {
            func add(a: i32, b: i32) -> i32
        }
        )"
        );
    }

    SECTION("Extern block referencing variable") {
        run_checker_stmt_test(
            R"(
        let result = ex1::x
        extern "C" ex1 {
            static var x: i32
        }
        )"
        );
    }

    SECTION("Extern block referencing non-existent name") {
        run_checker_stmt_test(
            R"(
        let result = ex1::add(1, 2)
        extern "C" ex1 {
            func subtract(a: i32, b: i32) -> i32
        }
        )",
            Err::NameNotFound
        );
    }

    SECTION("Extern block improper name qualification") {
        run_checker_stmt_test(
            R"(
        let result = add(1, 2)
        extern "C" ex1 {
            func add(a: i32, b: i32) -> i32
        }
        )",
            Err::NameNotFound
        );
    }

    SECTION("Extern block name conflicts with namespace") {
        run_checker_stmt_test(
            R"(
        namespace ex1 {
            func add(a: i32, b: i32) -> i32 => a + b
        }
        extern "C" ex1 {
            func add(a: i32, b: i32) -> i32
        }
        )",
            Err::NameAlreadyExists
        );
    }

    SECTION("Extern block name conflicts with variable") {
        run_checker_stmt_test(
            R"(
        let ex1 = 1
        extern "C" ex1 {
            func add(a: i32, b: i32) -> i32
        }
        )",
            Err::NameAlreadyExists
        );
    }

    SECTION("Extern block name is reserved") {
        run_checker_stmt_test(
            R"(
            extern bool {
                func add(a: i32, b: i32) -> i32
            }
            )",
            Err::NameIsReserved
        );
    }
}

TEST_CASE("Check extern declarations", "[checker]") {
    SECTION("Valid static variable declaration with linkage modifier") {
        run_checker_stmt_test(R"(
            #[linkage("external")]
            static var x: i32 = 42
        )");
    }

    SECTION("Linkage modifier static variable without initializer") {
        run_checker_stmt_test(
            R"(
            #[linkage("external")]
            static y: i32
        )",
            Err::ImmutableWithoutInitializer
        );
    }

    SECTION("Valid function declaration with linkage modifier") {
        run_checker_stmt_test(R"(
            #[linkage("external")]
            func add(a: i32, b: i32) -> i32 => a + b
        )");
    }

    SECTION("Function declaration with linkage modifier without body") {
        run_checker_stmt_test(
            R"(
            #[linkage("external")]
            func add(a: i32, b: i32) -> i32
        )",
            Err::FuncWithoutBody
        );
    }
}

TEST_CASE("Custom symbol declarations", "[checker]") {
    SECTION("Valid custom symbol declaration") {
        run_checker_stmt_test(R"(
            #[symbol("custom_symbol")]
            static var x: i32 = 42
        )");
    }

    SECTION("Symbol already exists 1") {
        run_checker_stmt_test(
            R"(
            #[symbol("custom_symbol")]
            static var x: i32 = 42
            #[symbol("custom_symbol")]
            static var y: i32 = 43
        )",
            Err::SymbolAlreadyExists
        );
    }

    SECTION("Symbol already exists 2") {
        run_checker_stmt_test(
            R"(
            static var x: i32 = 42
            #[symbol("::x")]
            static var y: i32 = 43
        )",
            Err::SymbolAlreadyExists
        );
    }

    SECTION("Symbol is reserved") {
        run_checker_stmt_test(
            R"(
            #[symbol("main")]
            static var x: i32 = 42
        )",
            Err::SymbolIsReserved
        );
    }
}

TEST_CASE("Check typedef declarations", "[checker]") {
    SECTION("Valid typedef declaration") {
        run_checker_stmt_test(
            R"(
            typedef MyInt = i32
        )"
        );
    }

    SECTION("Valid typedef declaration and usage") {
        run_checker_stmt_test(
            R"(
            typedef MyInt = i32
            let var x: MyInt = 42
            x = x + 37
        )"
        );
    }

    SECTION("Typedef name already exists") {
        run_checker_stmt_test(
            R"(
            typedef MyInt = i32
            typedef MyInt = f64
            )",
            Err::NameAlreadyExists
        );
    }

    SECTION("Typedef name is reserved") {
        run_checker_stmt_test(
            R"(
            typedef bool = i32
            )",
            Err::NameIsReserved
        );
    }

    SECTION("Variable name conflicts with typedef name") {
        run_checker_stmt_test(
            R"(
            let var MyInt: i32 = 42
            typedef MyInt = i32
            )",
            Err::NameAlreadyExists
        );
    }

    SECTION("Typedef name is scoped") {
        run_checker_stmt_test(
            R"(
            namespace ns {
                typedef MyInt = i32
                static var x: MyInt = 42
            }
            static var y: ns::MyInt = 43
            )"
        );
    }

    SECTION("Typedef use before declaration") {
        run_checker_stmt_test(
            R"(
            static var x: MyInt = 42
            typedef MyInt = i32
            )"
        );
    }

    SECTION("Typedef never resolves") {
        run_checker_stmt_test(
            R"(
            typedef MyInt = SomeInt
            )",
            Err::TypeNameNotFound
        );
    }

    SECTION("Unknown named type") {
        run_checker_stmt_test(
            R"(
            static var x: UnknownType = 42
            )",
            Err::TypeNameNotFound
        );
    }

    SECTION("Typedef with unknown underlying type") {
        run_checker_stmt_test(
            R"(
            typedef MyInt = UnknownType
            static var x: MyInt = 42
            )",
            Err::TypeNameNotFound
        );
    }

    SECTION("Typedef with tuple with unknown underlying type") {
        run_checker_stmt_test(
            R"(
            typedef MyTuple = (i32, UnknownType)
            static var x: MyTuple = (42, 43)
            )",
            Err::TypeNameNotFound
        );
    }

    SECTION("Typedef self reference") {
        run_checker_stmt_test(
            R"(
            typedef MyInt = MyInt
            )",
            Err::UnsizedNamedType
        );
    }

    SECTION("Typedef cycle of 2") {
        run_checker_stmt_test(
            R"(
            typedef A = B
            typedef B = A
            )",
            Err::UnsizedNamedType
        );
    }

    SECTION("Typedef cycle of 3") {
        run_checker_stmt_test(
            R"(
            typedef A = B
            typedef B = C
            typedef C = A
            )",
            Err::UnsizedNamedType
        );
    }

    SECTION("Typedef self reference behind pointer") {
        run_checker_stmt_test(
            R"(
            typedef MyInt = @MyInt
            )"
        );
    }

    SECTION("Typedef cycle of 2 broken by pointer") {
        run_checker_stmt_test(
            R"(
            typedef A = B
            typedef B = @A
            )"
        );
    }

    SECTION("Typedef cycle of 3 broken by pointer") {
        run_checker_stmt_test(
            R"(
            typedef A = B
            typedef B = C
            typedef C = @A
            )"
        );
    }

    SECTION("Typedef long chain") {
        run_checker_stmt_test(
            R"(
            typedef E = D
            typedef D = C
            typedef C = B
            typedef B = A
            typedef A = i32
            )"
        );
    }

    SECTION("Typedef big tuple") {
        run_checker_stmt_test(
            R"(
            typedef MyTuple = (A, B, C, D, E, F, G)
            
            static t: MyTuple = (1, 2, 3, 4, 5, 6, 7)

            typedef A = i32
            typedef B = i32
            typedef C = i32
            typedef D = i32
            typedef E = i32
            typedef F = i32
            typedef G = i32
            )"
        );
    }

    SECTION("Typedef tree") {
        run_checker_stmt_test(
            R"(
            typedef A = (B, C)
            typedef B = (D, E)
            typedef C = (F, G)
            typedef E = (H, I)
            typedef F = (J, K)

            typedef D = i32
            typedef G = i32
            typedef H = i32
            typedef I = i32
            typedef J = i32
            typedef K = i32
            )"
        );
    }

    SECTION("Typedef diamond dependencies") {
        run_checker_stmt_test(
            R"(
            typedef D = (B, C)
            typedef C = A
            typedef B = A
            typedef A = i32
            )"
        );
    }
}

TEST_CASE("Check struct def declarations", "[checker]") {
    SECTION("Valid struct declaration") {
        run_checker_stmt_test(
            R"(
            struct MyStruct {
                field var x: i32
                field var y: f64
            }
            )"
        );
    }

    SECTION("Struct with reserved name") {
        run_checker_stmt_test(
            R"(
            struct bool {
                field var x: i32
            }
            )",
            Err::NameIsReserved
        );
    }

    SECTION("Struct name already exists") {
        run_checker_stmt_test(
            R"(
            struct MyStruct {
                field var x: i32
            }
            struct MyStruct {
                field var y: f64
            }
            )",
            Err::NameAlreadyExists
        );
    }

    SECTION("Struct field name already exists") {
        run_checker_stmt_test(
            R"(
            struct MyStruct {
                field var x: i32
                field var x: f64
            }
            )",
            Err::DuplicateStructFieldName
        );
    }

    SECTION("Struct field reserved name OK") {
        run_checker_stmt_test(
            R"(
            struct MyStruct {
                field var bool: i32
            }
            )"
        );
    }

    SECTION("Struct field with default value") {
        run_checker_stmt_test(
            R"(
            struct MyStruct {
                field var x: i32 = 42
            }
            )"
        );
    }

    SECTION("Struct field default value type mismatch") {
        run_checker_stmt_test(
            R"(
            struct MyStruct {
                field var x: i32 = true
            }
            )",
            Err::DefaultFieldTypeMismatch
        );
    }

    SECTION("Struct type eventually resolves") {
        run_checker_stmt_test(
            R"(
            let var s: MyStruct

            struct MyStruct {
                field var x: i32
            }
            )"
        );
    }

    SECTION("Struct field access") {
        run_checker_stmt_test(
            R"(
            struct MyStruct {
                field x: i32
            }

            let s: MyStruct = new MyStruct { x: 42 }
            let x: i32 = s.x
        )"
        );
    }

    SECTION("Immutable struct with var field assignment") {
        run_checker_stmt_test(
            R"(
            struct MyStruct {
                field var x: i32
            }

            let s: MyStruct = new MyStruct { x: 42 }
            s.x = 43
        )",
            Err::AssignToImmutable
        );
    }

    SECTION("Mutable struct with immutable field assignment") {
        run_checker_stmt_test(
            R"(
            struct MyStruct {
                field x: i32
            }

            let var s: MyStruct = new MyStruct { x: 42 }
            s.x = 43
        )",
            Err::AssignToImmutable
        );
    }

    SECTION("Immutable struct with mut field assignment") {
        run_checker_stmt_test(
            R"(
            struct MyStruct {
                field mut x: i32
            }

            let s: MyStruct = new MyStruct { x: 42 }
            s.x = 43
        )"
        );
    }

    SECTION("Mutable struct with var field assignment") {
        run_checker_stmt_test(
            R"(
            struct MyStruct {
                field var x: i32
            }

            let var s: MyStruct = new MyStruct { x: 42 }
            s.x = 43
        )"
        );
    }

    SECTION("Struct field does not exist") {
        run_checker_stmt_test(
            R"(
            struct MyStruct {
                field x: i32
            }

            let s: MyStruct = new MyStruct { x: 42 }
            let y: i32 = s.y
        )",
            Err::NonExistentMemberAccess
        );
    }

    SECTION("Struct self referencing field type") {
        run_checker_stmt_test(
            R"(
            struct Node {
                field value: i32
                field next: @Node
            }
            let node1 = new Node { value: 1, next: nullptr }
            let node2 = new Node { value: 2, next: @node1 }
            )"
        );
    }

    SECTION("Struct as namespace") {
        run_checker_stmt_test(
            R"(
            struct Math {
                field var x: i32
                func add(a: i32, b: i32) -> i32 => a + b
            }
            let result = Math::add(1, 2)
            )"
        );
    }
}
