#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "nico/frontend/components/global_checker.h"
#include "nico/frontend/components/lexer.h"
#include "nico/frontend/components/local_checker.h"
#include "nico/frontend/components/parser.h"
#include "nico/frontend/utils/ast_node.h"
#include "nico/shared/code_file.h"
#include "nico/shared/logger.h"
#include "nico/shared/token.h"

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
void run_checker_test(
    std::string_view src_code,
    std::optional<Err> expected_error = std::nullopt,
    std::optional<bool> print_errors = std::nullopt,
    bool print_tree = false
) {
    // If there is no expected error, we enable printing to look for unexpected
    // errors.
    bool printing_enabled = print_errors.value_or(!expected_error.has_value());
    nico::Logger::inst().set_printing_enabled(printing_enabled);

    auto& errors = nico::Logger::inst().get_errors();
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

    context->reset();
    nico::Logger::inst().reset();
}

TEST_CASE("Local variable declarations", "[checker]") {
    SECTION("Valid local variable declarations 1") {
        run_checker_test("let a = 1");
    }

    SECTION("Valid local variable declarations 2") {
        run_checker_test("let a: i32 = 1");
    }

    SECTION("Typeof annotation") {
        run_checker_test("let a = 1 let b: typeof(a) = 2");
    }

    SECTION("Nullptr assignment single pointer") {
        run_checker_test("let var a: @i32 = nullptr");
    }

    SECTION("Nullptr assignment double pointer") {
        run_checker_test("let var a: @@i32 = nullptr");
    }

    SECTION("Nullptr assignment var pointer") {
        run_checker_test("let var a: var@i32 = nullptr");
    }

    SECTION("Nullptr assignment deep pointer") {
        run_checker_test("let var a: var@var@var@var@var@i32 = nullptr");
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

    SECTION("Let type mismatch 4") {
        run_checker_test("let var a: i32 = 1_i8", Err::LetTypeMismatch);
    }

    SECTION("Let type mismatch 5") {
        run_checker_test("let a: [i32; 3] = [1, 2]", Err::LetTypeMismatch);
    }

    SECTION("Let type mismatch 6") {
        run_checker_test("let a: i32 = nullptr", Err::LetTypeMismatch);
    }

    SECTION("Undeclared identifier") {
        run_checker_test("let a = b", Err::UndeclaredName);
    }

    SECTION("Immutable variables") {
        run_checker_test("let a = 1 let b = 2 a = b", Err::AssignToImmutable);
    }

    SECTION("Immutable without initializer") {
        run_checker_test("let a: i32", Err::ImmutableWithoutInitializer);
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

    SECTION("Valid unary expression 3") {
        run_checker_test("let a = !false");
    }

    SECTION("Unary type mismatch 1") {
        run_checker_test("let a = -true", Err::NoOperatorOverload);
    }

    SECTION("Unary type mismatch 2") {
        run_checker_test("let a = not 1", Err::NoOperatorOverload);
    }

    SECTION("Unary type mismatch 3") {
        run_checker_test("let a = !1.0", Err::NoOperatorOverload);
    }

    SECTION("Negative on unsigned type") {
        run_checker_test("-(1_u32)", Err::NegativeOnUnsignedType);
    }
}

TEST_CASE("Local sizeof expressions", "[checker]") {
    SECTION("Valid sizeof expression 1") {
        run_checker_test("let a: u64 = sizeof i32");
    }

    SECTION("Valid sizeof expression 2") {
        run_checker_test("let a: u64 = sizeof @i32");
    }

    SECTION("Valid sizeof expression 3") {
        run_checker_test("let a: u64 = sizeof var@i32");
    }

    SECTION("Valid sizeof expression 4") {
        run_checker_test("let x: i32 = 1 let var a: u64 = sizeof typeof(x)");
    }

    SECTION("Sizeof unsized type") {
        run_checker_test(
            "let a: u64 = sizeof [i32; ?]",
            Err::SizeOfUnsizedType
        );
    }
}

TEST_CASE("Local alloc expressions", "[checker]") {
    SECTION("Valid alloc type expr 1") {
        run_checker_test("let a: @i32 = alloc i32");
    }

    SECTION("Valid alloc type expr 2") {
        run_checker_test("let a: var@i32 = alloc i32");
    }

    SECTION("Valid alloc type expr 3") {
        run_checker_test("let a: @@i32 = alloc @i32");
    }

    SECTION("Unsized type allocation") {
        run_checker_test(
            "let a: @[i32; ?] = alloc [i32; ?]",
            Err::UnsizedTypeAllocation
        );
    }

    SECTION("Alloc type mismatch") {
        run_checker_test("let a: @i32 = alloc f64", Err::LetTypeMismatch);
    }

    SECTION("Valid alloc type with expr 1") {
        run_checker_test("let a: @i32 = alloc i32 with 10");
    }

    SECTION("Valid alloc type with expr 2") {
        run_checker_test(
            "let a: var@[i32; 5] = alloc [i32; 5] with [1, 2, 3, 4, 5]"
        );
    }

    SECTION("Valid alloc type with expr 3") {
        run_checker_test("let a: @@f64 = alloc @f64 with alloc f64");
    }

    SECTION("Valid alloc type with expr 4") {
        run_checker_test("let a: @@i32 = alloc @i32 with nullptr");
    }

    SECTION("Alloc init type mismatch 1") {
        run_checker_test(
            "let a: @i32 = alloc i32 with 10.0",
            Err::AllocInitTypeMismatch
        );
    }

    SECTION("Alloc init type mismatch 2") {
        run_checker_test(
            "let a: var@[i32; 5] = alloc [i32; 5] with [1, 2, 3]",
            Err::AllocInitTypeMismatch
        );
    }

    SECTION("Alloc init type mismatch 3") {
        run_checker_test(
            "let a: @f64 = alloc f64 with nullptr",
            Err::AllocInitTypeMismatch
        );
    }

    SECTION("Valid alloc with expr 1") {
        run_checker_test("let a: @i32 = alloc with 10");
    }

    SECTION("Valid alloc with expr 2") {
        run_checker_test("let a: @[i32; 3] = alloc with [1, 2, 3]");
    }

    SECTION("Valid alloc with expr 3") {
        run_checker_test("let a: @@f64 = alloc with nullptr");
    }

    SECTION("Valid alloc with expr 4") {
        run_checker_test("let a: @nullptr = alloc with nullptr");
    }

    SECTION("Alloc with expr type mismatch 1") {
        run_checker_test("let a: i32 = alloc with 10.0", Err::LetTypeMismatch);
    }

    SECTION("Alloc with expr type mismatch 2") {
        run_checker_test(
            "let a: @[i32; 5] = alloc with [1, 2, 3]",
            Err::LetTypeMismatch
        );
    }

    SECTION("Alloc with expr type mismatch 3") {
        run_checker_test(
            "let a: nullptr = alloc with nullptr",
            Err::LetTypeMismatch
        );
    }
}

// TODO: Add a new test case for alloc-for and dealloc

TEST_CASE("Local alloc-for expressions", "[checker]") {
    SECTION("Valid alloc-for expression 1") {
        run_checker_test("let a: @[i32; ?] = alloc for 1 of i32");
    }

    SECTION("Valid alloc-for expression 2") {
        run_checker_test("let a: var@[i32; ?] = alloc for 10 of i32");
    }

    SECTION("Valid alloc-for expression 3") {
        run_checker_test("let a: @[@[i32; ?]; ?] = alloc for 5 of @[i32; ?]");
    }

    SECTION("Valid alloc-for expression 4") {
        run_checker_test("let n = 10 let a: @[i32; ?] = alloc for n of i32");
    }

    SECTION("Alloc-for amount not integer") {
        run_checker_test(
            "let a: @[i32; ?] = alloc for 1.0 of i32",
            Err::AllocAmountNotInteger
        );
    }

    SECTION("Alloc-for unsized type allocation") {
        run_checker_test(
            "let a: @[i32; ?] = alloc for 10 of [i32; ?]",
            Err::UnsizedTypeAllocation
        );
    }
}

TEST_CASE("Local dealloc statements", "[checker]") {
    SECTION("Valid dealloc statement 1") {
        run_checker_test("let a: @i32 = alloc i32 unsafe { dealloc a }");
    }

    SECTION("Valid dealloc statement 2") {
        run_checker_test("let var a: var@i32 = alloc i32 unsafe { dealloc a }");
    }

    SECTION("Valid dealloc statement 3") {
        run_checker_test(
            "let a: @i32 = alloc i32 let b = a unsafe { dealloc b }"
        );
    }

    SECTION("Valid dealloc statement 4") {
        run_checker_test(
            "let a: @[i32; 5] = alloc [i32; 5] with [1,2,3,4,5] "
            "unsafe { dealloc a }"
        );
    }

    SECTION("Valid dealloc statement 5") {
        run_checker_test(
            "let a: @[i32; ?] = alloc for 10 of i32 unsafe { dealloc a }"
        );
    }

    SECTION("Dealloc non-pointer") {
        run_checker_test(
            "let a = 1 unsafe { dealloc a }",
            Err::DeallocNonRawPointer
        );
    }

    SECTION("Dealloc nullptr") {
        run_checker_test(
            "let a = nullptr unsafe { dealloc a }",
            Err::DeallocNullptr
        );
    }

    SECTION("Dealloc outside unsafe") {
        run_checker_test(
            "let a: @i32 = alloc i32 dealloc a",
            Err::DeallocOutsideUnsafeBlock
        );
    }
}

TEST_CASE("Local non pointer cast expressions") {
    SECTION("Valid cast NoOp") {
        run_checker_test("let a: i32 = 1 let b: i32 = a as i32");
    }

    SECTION("Valid cast IntToBool") {
        run_checker_test("let a: i32 = 1 let b: bool = a as bool");
    }

    SECTION("Valid cast FPToBool") {
        run_checker_test("let a: f64 = 1.0 let b: bool = a as bool");
    }

    SECTION("Valid cast SignExt") {
        run_checker_test("let a: i8 = 1_i8 let b: i32 = a as i32");
    }

    SECTION("Valid cast ZeroExt 1") {
        run_checker_test("let a: u8 = 1_u8 let b: u32 = a as u32");
    }

    SECTION("Valid cast ZeroExt 2") {
        run_checker_test("let a: u8 = 1_u8 let b: i32 = a as i32");
    }

    SECTION("Valid cast ZeroExt 3") {
        run_checker_test("let a: i8 = -1_i8 let b: u32 = a as u32");
    }

    SECTION("Valid cast IntTrunc") {
        run_checker_test("let a: i32 = 1 let b: i8 = a as i8");
    }

    SECTION("Valid cast NoOp ints") {
        run_checker_test("let a: u32 = 1_u32 let b: i32 = a as i32");
    }

    SECTION("Valid cast SIntToFP") {
        run_checker_test("let a: i32 = 1 let b: f64 = a as f64");
    }

    SECTION("Valid cast UIntToFP") {
        run_checker_test("let a: u32 = 1_u32 let b: f64 = a as f64");
    }

    SECTION("Valid cast FPExt") {
        run_checker_test("let a: f32 = 1.0_f32 let b: f64 = a as f64");
    }

    SECTION("Valid cast FPTrunc") {
        run_checker_test("let a: f64 = 1.0 let b: f32 = a as f32");
    }

    SECTION("Valid cast FPToSInt") {
        run_checker_test("let a: f64 = 1.0 let b: i32 = a as i32");
    }

    SECTION("Valid cast FPToUInt") {
        run_checker_test("let a: f64 = 1.0 let b: u32 = a as u32");
    }

    SECTION("Invalid cast operation") {
        run_checker_test(
            "let a: bool = true let b: () = a as ()",
            Err::InvalidCastOperation
        );
    }
}

TEST_CASE("Local pointer cast expressions", "[checker]") {
    SECTION("Valid nullptr cast to pointer") {
        run_checker_test("let a: @i32 = nullptr as @i32");
    }
}

TEST_CASE("Local address-of expressions", "[checker]") {
    SECTION("Valid address-of expression 1") {
        run_checker_test("let a = 1 let b: @i32 = @a");
    }

    SECTION("Valid address-of expression 2") {
        run_checker_test("let var a = 1 let b: var@i32 = var@a");
    }

    SECTION("Valid address-of expression 3") {
        run_checker_test("let var a = 1 let b: @i32 = var@a");
    }

    SECTION("Address-of mutability gain") {
        run_checker_test("let a = 1 let b: var@i32 = @a", Err::LetTypeMismatch);
    }

    SECTION("Valid pointer pointer 1") {
        run_checker_test("let a = 1 let b: @i32 = @a let c: @@i32 = @b");
    }

    SECTION("Valid pointer pointer 2") {
        run_checker_test(
            "let var a = 1 let b: var@i32 = var@a let c: @var@i32 = @b"
        );
    }

    SECTION("Pointer pointer mutability gain") {
        run_checker_test(
            "let var a = 1 let b: @i32 = @a let c: @var@i32 = @b",
            Err::LetTypeMismatch
        );
    }

    SECTION("Address-of not an lvalue") {
        run_checker_test(
            "let a = 1 let b: @i32 = @(a + 1)",
            Err::NotAPossibleLValue
        );
    }

    SECTION("Address-of immutable value as mutable") {
        run_checker_test(
            "let a = 1 let b: var@i32 = var@a",
            Err::AddressOfImmutable
        );
    }
}

TEST_CASE("Local dereference expressions", "[checker]") {
    SECTION("Dereference without unsafe") {
        run_checker_test(
            "let a = 1 let b: @i32 = @a let c = ^b",
            Err::PtrDerefOutsideUnsafeBlock
        );
    }

    SECTION("Valid dereference with unsafe") {
        run_checker_test(
            "let a = 1 let b: @i32 = @a let c: i32 = unsafe { yield ^b }"
        );
    }

    SECTION("Dereference non-pointer") {
        run_checker_test("let a = 1 let b = ^a", Err::DereferenceNonPointer);
    }

    SECTION("Valid mutable pointer") {
        run_checker_test(
            "let var a = 1 let b: var@i32 = var@a unsafe { ^b = 2 }"
        );
    }

    SECTION("Valid mutable pointer pointer") {
        run_checker_test(
            "let var a = 1 let b = var@a let c = @b "
            "unsafe { ^^c = 2 }"
        );
    }

    SECTION("Assign to immutable via pointer") {
        run_checker_test(
            "let a = 1 let b: @i32 = @a unsafe { ^b = 2 }",
            Err::AssignToImmutable
        );
    }

    SECTION("Dereference raw pointer with nullptr value") {
        run_checker_test("let b: @i32 = nullptr unsafe { ^b }");
    }

    SECTION("Dereference nullptr type pointer") {
        run_checker_test(
            "let p = nullptr unsafe { ^p }",
            Err::DereferenceNonTypedPointer
        );
    }

    SECTION("Unsafeness does not propagate") {
        run_checker_test(
            "let a = 1 let b: @i32 = @a unsafe { block { ^b } }",
            Err::PtrDerefOutsideUnsafeBlock
        );
    }

    SECTION("Implicit dereference nullptr") {
        run_checker_test(
            R"(
            let p = nullptr
            unsafe {
                let x = p.0
            }
            )",
            Err::DereferenceNonTypedPointer
        );
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

TEST_CASE("Local comparison expressions", "[checker]") {
    SECTION("Valid comparison expressions 1") {
        run_checker_test("let a = 1 < 2");
    }

    SECTION("Valid comparison expressions 2") {
        run_checker_test("let a = 1.0 >= 2.0");
    }

    SECTION("Valid comparison expressions 3") {
        run_checker_test("let a = 1 == 1");
    }

    SECTION("Valid comparison expressions 4") {
        run_checker_test("let a = 2.0 != 1.0");
    }

    SECTION("Valid comparison expressions 5") {
        run_checker_test("let a = true == false");
    }

    SECTION("Valid comparison expressions 6") {
        run_checker_test(
            "let a: @i32 = nullptr let b = a == nullptr let c = a != nullptr"
        );
    }

    SECTION("Valid comparison expressions 7") {
        run_checker_test(
            "let a: @i32 = nullptr let b: @@i32 = @a let c = b == a"
        );
    }

    SECTION("Comparison type mismatch 1") {
        run_checker_test("let a = 1 < true", Err::NoOperatorOverload);
    }

    SECTION("Comparison type mismatch 2") {
        run_checker_test("let a = true >= 1", Err::NoOperatorOverload);
    }

    SECTION("Comparison type mismatch 3") {
        run_checker_test("let a = true < false", Err::NoOperatorOverload);
    }

    SECTION("Comparison type mismatch 4") {
        run_checker_test("let a = 1 == 1.0", Err::NoOperatorOverload);
    }

    SECTION("Comparison type mismatch 5") {
        run_checker_test("let a = 1.0 != true", Err::NoOperatorOverload);
    }

    SECTION("Comparison type mismatch 6") {
        run_checker_test("let a = \"\" == 1", Err::NoOperatorOverload);
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
        run_checker_test("let var a = 1; (a = 1) = 2", Err::NotAPossibleLValue);
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

    SECTION("Block expression multiple yields") {
        run_checker_test("let var a = 1 a = block { yield 2 yield 3 }");
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

    SECTION("Tuple implicit dereference 1") {
        run_checker_test(
            R"(
            let var a = (1,) 
            let b = @a 
            let var c: i32 
            unsafe { 
                c = (^b).0 
                c = b.0 
            }
            )"
        );
    }

    SECTION("Tuple implicit dereference 2") {
        run_checker_test(
            R"(
            let var a = (1,) 
            let p = @a
            let pp = @p
            let ppp = @pp 
            let var c: i32 
            unsafe { 
                c = (^^^ppp).0 
                c = (^^ppp).0 
                c = (^ppp).0
                c = ppp.0 
            }
            )"
        );
    }

    SECTION("Tuple implicit deref outside unsafe") {
        run_checker_test(
            R"(
            let var a = (1,) 
            let p = @a
            let c = p.0
            )",
            Err::PtrDerefOutsideUnsafeBlock
        );
    }
}

TEST_CASE("Local array expressions", "[checker]") {
    SECTION("Valid array expression 1") {
        run_checker_test("let a = [1, 2, 3, 4, 5]");
    }

    SECTION("Valid array expression 2") {
        run_checker_test("let a: [i32; 3] = [1, 2, 3]");
    }

    SECTION("Array element type mismatch") {
        run_checker_test(
            "let a: [i32; 3] = [1, 2.0, 3]",
            Err::ArrayElementTypeMismatch
        );
    }

    SECTION("Array type mismatch") {
        run_checker_test("let a: [i32; 4] = [1, 2, 3]", Err::LetTypeMismatch);
    }

    SECTION("Empty array expression") {
        run_checker_test("let a: [i32; 0] = []");
    }

    SECTION("Unsized type allocation 1") {
        run_checker_test(
            "let a: [i32; ?] = [1, 2, 3]",
            Err::UnsizedTypeAllocation
        );
    }

    SECTION("Unsized type allocation 2") {
        run_checker_test(
            "let a: ([i32; ?]) = ([1],)",
            Err::UnsizedTypeAllocation
        );
    }

    SECTION("Unsized type under pointer") {
        run_checker_test("let a: [i32; 3] = [1, 2, 3] let b: @[i32; ?] = @a");
    }

    SECTION("Unsized array pointer cast") {
        run_checker_test("let a: [i32; 3] = [1, 2, 3] let b = @a as @[i32; ?]");
    }

    SECTION("Unsized array access") {
        run_checker_test(
            "let a: [i32; 3] = [1, 2, 3] let b = @a as @[i32; ?] "
            "let var c: i32 unsafe { c = b[0] }"
        );
    }

    SECTION("Unsized type as rvalue") {
        run_checker_test(
            R"(
            let var a: [i32; 3] = [1, 2, 3]
            let p = var@a as var@[i32; ?]
            unsafe:
                let q = ^p
            )",
            Err::UnsizedRValue
        );
    }

    SECTION("Unsized type as lvalue") {
        run_checker_test(
            R"(
            let var a: [i32; 3] = [1, 2, 3]
            let p = var@a as var@[i32; ?]
            unsafe:
                let q = (^p)[0];
                (^p)[0] = 10
                ^p = [4, 5, 6]
            )"
        );
    }

    SECTION("Array implicit dereference 1") {
        run_checker_test(
            R"(
            let var a: [i32; 3] = [1, 2, 3]
            let b: @[i32; 3] = @a
            let var c: i32
            unsafe {
                c = (^b)[0]
                c = b[0]
            }
            )"
        );
    }

    SECTION("Array implicit dereference 2") {
        run_checker_test(
            R"(
            let var a: [i32; 3] = [1, 2, 3]
            let p = @a
            let pp = @p
            let ppp = @pp
            let var c: i32
            unsafe {
                c = (^^^ppp)[0]
                c = (^^ppp)[0]
                c = (^ppp)[0]
                c = ppp[0]
            }
            )"
        );
    }

    SECTION("Array implicit deref outside unsafe") {
        run_checker_test(
            R"(
            let var a: [i32; 3] = [1, 2, 3]
            let p = @a
            let c = p[0]
            )",
            Err::PtrDerefOutsideUnsafeBlock
        );
    }

    SECTION("Array assignment") {
        run_checker_test("let a = [1, 2, 3] let b = a");
    }
}

TEST_CASE("Local subscript expressions", "[checker]") {
    SECTION("Valid subscript expression 1") {
        run_checker_test("let a = [10, 20, 30] let b: i32 = a[1]");
    }

    SECTION("Valid subscript expression 2") {
        run_checker_test("let a = [[1,2], [3,4], [5,6]] let b: i32 = a[2][0]");
    }

    SECTION("Subscript index type mismatch") {
        run_checker_test(
            "let a = [10, 20, 30] let b = a[true]",
            Err::ArrayIndexNotInteger
        );
    }

    SECTION("Subscript on non-array type") {
        run_checker_test(
            "let a = 10 let b = a[0]",
            Err::OperatorNotValidForExpr
        );
    }

    SECTION("Subscript as valid lvalue") {
        run_checker_test("let var a = [10, 20, 30] a[1] = 25");
    }

    SECTION("Subscript assign to immutable") {
        run_checker_test(
            "let a = [10, 20, 30] a[1] = 25",
            Err::AssignToImmutable
        );
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

TEST_CASE("Local loop expressions", "[checker]") {
    SECTION("Valid loop expression 1") {
        run_checker_test("loop { printout \"Hello, World!\" }");
    }

    SECTION("Valid loop expression 2") {
        run_checker_test(
            "let cond = true while cond { printout \"Hello, World!\" }"
        );
    }

    SECTION("Valid loop expression 3") {
        run_checker_test(R"(
        let cond = true
        while cond:
            printout "Hello, World!"
        )");
    }

    SECTION("Valid loop expression 4") {
        run_checker_test(
            "let cond = true do { printout \"Hello, World!\" } while cond"
        );
    }

    SECTION("Valid loop expression 5") {
        run_checker_test(R"(
        let result = loop:
            break 1
        )");
    }

    SECTION("Valid loop expression 6") {
        run_checker_test(R"(
        let result = while true:
            break 1
        )");
    }

    SECTION("Valid loop expression 7") {
        run_checker_test(R"(
        let result = 
        do:
            break 1
        while true
        )");
    }

    SECTION("Valid short loop expression 1") {
        run_checker_test("let var x = 1 loop x = x + 1");
    }

    SECTION("Valid short loop expression 2") {
        run_checker_test("let var x = 1 while false do x = x + 1");
    }

    SECTION("Valid short loop expression 3") {
        run_checker_test("let var x = 1 do x = x + 1 while false");
    }

    SECTION("Loop condition not bool") {
        run_checker_test("while 1 { }", Err::ConditionNotBool);
    }

    SECTION("While loop yielding non-unit") {
        run_checker_test(
            "let result = while false { yield 1 }",
            Err::WhileLoopYieldingNonUnit
        );
    }

    SECTION("Do-while loop yielding non-unit") {
        run_checker_test(
            "let result = do { yield 1 } while false",
            Err::WhileLoopYieldingNonUnit
        );
    }

    SECTION("Break outside loop") {
        run_checker_test("break ()", Err::BreakOutsideLoop);
    }

    SECTION("Continue outside loop") {
        run_checker_test("continue", Err::ContinueOutsideLoop);
    }
}

TEST_CASE("Local function declarations", "[checker]") {
    SECTION("Valid function declaration braced form") {
        run_checker_test(R"(
        func add(a: i32, b: i32) -> i32 {
            return a + b
        }
        )");
    }

    SECTION("Valid function declaration indented form") {
        run_checker_test(R"(
        func add(a: i32, b: i32) -> i32:
            return a + b
        )");
    }

    SECTION("Valid function declaration short form") {
        run_checker_test("func add(a: i32, b: i32) -> i32 => a + b");
    }

    SECTION("Function with reserved name") {
        run_checker_test(
            "func bool(a: i32) -> i32 { return a }",
            Err::NameIsReserved
        );
    }

    SECTION("Variable name already exists") {
        run_checker_test(
            "let add = 1 "
            "func add(a: i32, b: i32) -> i32 { return a + b }",
            Err::NameAlreadyExists
        );
    }

    SECTION("Variable name OK") {
        run_checker_test(
            "block { let add = 1 } "
            "func add(a: i32, b: i32) -> i32 => a + b "
            "block { let add = 1 }"
        );
    }

    SECTION("Function name already exists") {
        run_checker_test(
            "func add(a: i32, b: i32) -> i32 { return a + b } "
            "let add = 1",
            Err::NameAlreadyExists
        );
    }

    SECTION("Duplicate function parameter name") {
        run_checker_test(
            "func add(a: i32, a: i32) -> i32 { return a + a }",
            Err::DuplicateFunctionParameterName
        );
    }

    SECTION("Function parameter default argument type mismatch") {
        run_checker_test(
            "func add(a: i32 = true, b: i32) -> i32 { return a + b }",
            Err::DefaultArgTypeMismatch
        );
    }

    SECTION("Function immutable parameter assignment") {
        run_checker_test(
            "func add(a: i32, b: i32) -> i32 { a = 2 return a + b }",
            Err::AssignToImmutable
        );
    }

    SECTION("Function return type mismatch") {
        run_checker_test(
            "func add(a: i32, b: i32) -> i32 { return true }",
            Err::FunctionReturnTypeMismatch
        );
    }
}

TEST_CASE("Local function overload declarations", "[checker]") {
    SECTION("Valid overloads 1") {
        run_checker_test(R"(
        func add(a: i32, b: i32) -> i32 => a + b
        func add(a: f64, b: f64) -> f64 => a + b
        )");
    }

    SECTION("Valid overloads 2") {
        run_checker_test(R"(
        func add(a: i32, b: i32, c: i32) -> i32 => a + b + c
        func add(a: i32, b: i32) -> i32 => a + b
        )");
    }

    SECTION("Valid overloads 3") {
        run_checker_test(R"(
        func add(b: i32) -> i32 => b + 1
        func add(a: i32) -> i32 => a + 1
        )");
    }

    SECTION("Valid overloads 4") {
        run_checker_test(R"(
        func add(a: i32, b: i32, c: i32 = 0) -> i32 => a + b + c
        func add(a: i32) -> i32 => a + 1
        )");
    }

    SECTION("Valid overloads 5") {
        run_checker_test(R"(
            func add(a: i32, b: i32, c: i32 = 0) -> i32 => a + b + c
            func add(a: i32, b: f64) -> i32 => 0
        )");
    }

    SECTION("Valid overloads 6") {
        run_checker_test(
            R"(
        func add(a: i32, b: i32 = 0, c: i32 = 0) -> i32 => a + b + c
        func add(a: i32, d: i32 = 0) -> i32 => a + d
        )"
        );
    }

    SECTION("Many valid overloads") {
        run_checker_test(
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
        run_checker_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        func add(a: i32, b: i32) -> i32 => a - b
        )",
            Err::FunctionOverloadConflict
        );
    }

    SECTION("Overload conflicts 2") {
        run_checker_test(
            R"(
        func add(a: i32, b: i32, c: i32 = 0) -> i32 => a + b + c
        func add(a: i32, b: i32) -> i32 => a + b
        )",
            Err::FunctionOverloadConflict
        );
    }

    SECTION("Overload conflicts 3") {
        run_checker_test(
            R"(
        func add(a: i32, b: f64) -> i32 => 0
        func add(b: f64, a: i32) -> i32 => 0
        )",
            Err::FunctionOverloadConflict
        );
    }

    SECTION("Overload conflicts 4") {
        run_checker_test(
            R"(
        func add(a: i32) -> i32 => a + 1
        func add(a: i32, b: i32) -> i32 => a + b
        func add(a: i32, b: i32 = 0) -> i32 => a + b
        )",
            Err::FunctionOverloadConflict
        );
    }

    SECTION("Overload conflicts 5") {
        run_checker_test(
            R"(
        func add() -> i32 => 0
        func add(a: i32 = 0) -> i32 => a + 1
        )",
            Err::FunctionOverloadConflict
        );
    }
}

TEST_CASE("Local function call", "[checker]") {
    SECTION("Valid function call") {
        run_checker_test(R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let result: i32 = add(1, 2)
        )");
    }

    SECTION("Function call no arguments") {
        run_checker_test(R"(
        func get_five() -> i32 => 5
        let result: i32 = get_five()
        )");
    }

    SECTION("Function call undeclared name") {
        run_checker_test("let result = add(1, 2)", Err::UndeclaredName);
    }

    SECTION("Function call wrong number of arguments") {
        run_checker_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let result: i32 = add(1)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("Function call too many arguments") {
        run_checker_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let result: i32 = add(1, 2, 3)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("Not a callable") {
        run_checker_test(
            R"(
        let add = 1
        let result = add(1, 2)
        )",
            Err::NotACallable
        );
    }

    SECTION("Function parameter type mismatch") {
        run_checker_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let result: i32 = add(1, true)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("Function call error in argument") {
        run_checker_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let result: i32 = add(1, undeclared_var)
        )",
            Err::UndeclaredName
        );
    }

    SECTION("Function call with default arguments") {
        run_checker_test(R"(
        func add(a: i32, b: i32 = 2) -> i32 => a + b
        let result1: i32 = add(3)
        let result2: i32 = add(3, 4)
        )");
    }

    SECTION("Function call with named arguments") {
        run_checker_test(R"(
        func add(a: i32, b: i32, c: i32) -> i32 => a + b + c
        let result1: i32 = add(a: 1, b: 2, c: 3)
        let result2: i32 = add(c: 3, a: 1, b: 2)
        )");
    }

    SECTION("Function call with named arguments and defaults") {
        run_checker_test(R"(
        func add(a: i32, b: i32 = 2, c: i32) -> i32 => a + b + c
        let result: i32 = add(1, c: 3)
        )");
    }

    SECTION("Function call with invalid named argument") {
        run_checker_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let result: i32 = add(a: 1, c: 2)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("Function pointer call") {
        run_checker_test(R"(
        func add(a: i32, b: i32) -> i32 => a + b
        let func_ptr = add
        let result: i32 = func_ptr(1, 2)
        )");
    }

    SECTION("Function call before declaration") {
        run_checker_test(
            R"(
        let result: i32 = add(1, 2)
        func add(a: i32, b: i32) -> i32 => a + b
        )"
        );
    }
}

TEST_CASE("Local function overload calls", "[checker]") {
    SECTION("Valid overload call 1") {
        run_checker_test(R"(
        func add(a: i32, b: i32) -> i32 => a + b
        func add(a: f64, b: f64) -> f64 => a + b
        let result1: i32 = add(1, 2)
        let result2: f64 = add(1.0, 2.0)
        )");
    }

    SECTION("Valid overload call 2") {
        run_checker_test(R"(
            func f() -> bool => true
            func f(p1: i32) -> i32 => 1
            func f(p1: i32, p2: i32) -> f64 => 2.0
            let a: bool = f()
            let b: i32 = f(10)
            let c: f64 = f(10, 20)
        )");
    }

    SECTION("Valid overload call 3") {
        run_checker_test(R"(
        func f(a: i32) -> i32 => 0
        func f(b: i32) -> f64 => 0.0
        let a: i32 = f(a: 10)
        let b: f64 = f(b: 20)
        )");
    }

    SECTION("Valid overload call 4") {
        run_checker_test(R"(
        func f(a: i32, b: i32) -> i32 => 0
        func f(a: i32, c: i32) -> f64 => 0.0
        let a: i32 = f(a: 10, b: 20)
        let b: f64 = f(a: 30, c: 40)
        )");
    }

    SECTION("Valid overload call 5") {
        run_checker_test(R"(
        func f(a: i32, b: i32) -> i32 => 0
        func f(a: i32) -> f64 => 0.0
        let a: i32 = f(a: 10, b: 20)
        let b: f64 = f(a: 30)
        )");
    }

    SECTION("Ambiguous call 1") {
        run_checker_test(
            R"(
        func add(a: i32, b: i32 = 2) -> i32 => a + b
        func add(a: i32, c: i32 = 3) -> i32 => a + c
        let result: i32 = add(1)
        )",
            Err::MultipleMatchingFunctionOverloads
        );
    }

    SECTION("Ambiguous call 2") {
        run_checker_test(
            R"(
        func f(a: i32, b: i32) -> i32 => 0
        func f(a: i32, c: i32) -> f64 => 0.0
        let result: i32 = f(1, 2)
        )",
            Err::MultipleMatchingFunctionOverloads
        );
    }

    SECTION("Many matching overloads") {
        run_checker_test(
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
        run_checker_test(
            R"(
        func add(a: i32, b: i32) -> i32 => a + b
        func add(a: f64, b: f64) -> f64 => a + b
        let result: i32 = add(1, 2.0)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("No matching overload 2") {
        run_checker_test(
            R"(
        func f() -> i32 => 0
        func f(a: i32) -> i32 => 0
        let result: i32 = f(1, 2)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("No matching overload 3") {
        run_checker_test(
            R"(
        func f(a: i32) -> i32 => 0
        func f(b: i32) -> i32 => 0
        let result: i32 = f(c: 1)
        )",
            Err::NoMatchingFunctionOverload
        );
    }

    SECTION("Many non-matching overloads") {
        run_checker_test(
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
        run_checker_test(
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
        run_checker_test(
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
