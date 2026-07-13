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
void run_checker_expr_test(
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

TEST_CASE("Check unary expressions", "[checker]") {
    SECTION("Valid unary expression 1") {
        run_checker_expr_test("let a = -1");
    }

    SECTION("Valid unary expression 2") {
        run_checker_expr_test("let a = not true");
    }

    SECTION("Valid unary expression 3") {
        run_checker_expr_test("let a = !false");
    }

    SECTION("Unary type mismatch 1") {
        run_checker_expr_test("let a = -true", Err::NoOperatorOverload);
    }

    SECTION("Unary type mismatch 2") {
        run_checker_expr_test("let a = not 1", Err::NoOperatorOverload);
    }

    SECTION("Unary type mismatch 3") {
        run_checker_expr_test("let a = !1.0", Err::NoOperatorOverload);
    }

    SECTION("Negative on unsigned type") {
        run_checker_expr_test("-(1_u32)", Err::NegativeOnUnsignedType);
    }
}

TEST_CASE("Check sizeof expressions", "[checker]") {
    SECTION("Valid sizeof expression 1") {
        run_checker_expr_test("let a: u64 = sizeof i32");
    }

    SECTION("Valid sizeof expression 2") {
        run_checker_expr_test("let a: u64 = sizeof @i32");
    }

    SECTION("Valid sizeof expression 3") {
        run_checker_expr_test("let a: u64 = sizeof var@i32");
    }

    SECTION("Valid sizeof expression 4") {
        run_checker_expr_test(
            "let x: i32 = 1 let var a: u64 = sizeof typeof(x)"
        );
    }

    SECTION("Sizeof unsized type") {
        run_checker_expr_test(
            "let a: u64 = sizeof [i32; ?]",
            Err::SizeOfUnsizedType
        );
    }
}

TEST_CASE("Check alloc expressions", "[checker]") {
    SECTION("Valid alloc type expr 1") {
        run_checker_expr_test("let a: @i32 = alloc i32");
    }

    SECTION("Valid alloc type expr 2") {
        run_checker_expr_test("let a: var@i32 = alloc i32");
    }

    SECTION("Valid alloc type expr 3") {
        run_checker_expr_test("let a: @@i32 = alloc @i32");
    }

    SECTION("Unsized type allocation") {
        run_checker_expr_test(
            "let a: @[i32; ?] = alloc [i32; ?]",
            Err::UnsizedTypeAllocation
        );
    }

    SECTION("Alloc type mismatch") {
        run_checker_expr_test("let a: @i32 = alloc f64", Err::LetTypeMismatch);
    }

    SECTION("Valid alloc type with expr 1") {
        run_checker_expr_test("let a: @i32 = alloc i32 with 10");
    }

    SECTION("Valid alloc type with expr 2") {
        run_checker_expr_test(
            "let a: var@[i32; 5] = alloc [i32; 5] with [1, 2, 3, 4, 5]"
        );
    }

    SECTION("Valid alloc type with expr 3") {
        run_checker_expr_test("let a: @@f64 = alloc @f64 with alloc f64");
    }

    SECTION("Valid alloc type with expr 4") {
        run_checker_expr_test("let a: @@i32 = alloc @i32 with nullptr");
    }

    SECTION("Alloc init type mismatch 1") {
        run_checker_expr_test(
            "let a: @i32 = alloc i32 with 10.0",
            Err::AllocInitTypeMismatch
        );
    }

    SECTION("Alloc init type mismatch 2") {
        run_checker_expr_test(
            "let a: var@[i32; 5] = alloc [i32; 5] with [1, 2, 3]",
            Err::AllocInitTypeMismatch
        );
    }

    SECTION("Alloc init type mismatch 3") {
        run_checker_expr_test(
            "let a: @f64 = alloc f64 with nullptr",
            Err::AllocInitTypeMismatch
        );
    }

    SECTION("Valid alloc with expr 1") {
        run_checker_expr_test("let a: @i32 = alloc with 10");
    }

    SECTION("Valid alloc with expr 2") {
        run_checker_expr_test("let a: @[i32; 3] = alloc with [1, 2, 3]");
    }

    SECTION("Valid alloc with expr 3") {
        run_checker_expr_test("let a: @@f64 = alloc with nullptr");
    }

    SECTION("Valid alloc with expr 4") {
        run_checker_expr_test("let a: @nullptr = alloc with nullptr");
    }

    SECTION("Alloc with expr type mismatch 1") {
        run_checker_expr_test(
            "let a: i32 = alloc with 10.0",
            Err::LetTypeMismatch
        );
    }

    SECTION("Alloc with expr type mismatch 2") {
        run_checker_expr_test(
            "let a: @[i32; 5] = alloc with [1, 2, 3]",
            Err::LetTypeMismatch
        );
    }

    SECTION("Alloc with expr type mismatch 3") {
        run_checker_expr_test(
            "let a: nullptr = alloc with nullptr",
            Err::LetTypeMismatch
        );
    }
}

TEST_CASE("Check alloc-for expressions", "[checker]") {
    SECTION("Valid alloc-for expression 1") {
        run_checker_expr_test("let a: @[i32; ?] = alloc for 1 of i32");
    }

    SECTION("Valid alloc-for expression 2") {
        run_checker_expr_test("let a: var@[i32; ?] = alloc for 10 of i32");
    }

    SECTION("Valid alloc-for expression 3") {
        run_checker_expr_test(
            "let a: @[@[i32; ?]; ?] = alloc for 5 of @[i32; ?]"
        );
    }

    SECTION("Valid alloc-for expression 4") {
        run_checker_expr_test(
            "let n = 10 let a: @[i32; ?] = alloc for n of i32"
        );
    }

    SECTION("Alloc-for amount not integer") {
        run_checker_expr_test(
            "let a: @[i32; ?] = alloc for 1.0 of i32",
            Err::AllocAmountNotInteger
        );
    }

    SECTION("Alloc-for unsized type allocation") {
        run_checker_expr_test(
            "let a: @[i32; ?] = alloc for 10 of [i32; ?]",
            Err::UnsizedTypeAllocation
        );
    }
}

TEST_CASE("Check non pointer cast expressions") {
    SECTION("Valid cast NoOp") {
        run_checker_expr_test("let a: i32 = 1 let b: i32 = a as i32");
    }

    SECTION("Valid cast IntToBool") {
        run_checker_expr_test("let a: i32 = 1 let b: bool = a as bool");
    }

    SECTION("Valid cast FPToBool") {
        run_checker_expr_test("let a: f64 = 1.0 let b: bool = a as bool");
    }

    SECTION("Valid cast SignExt") {
        run_checker_expr_test("let a: i8 = 1_i8 let b: i32 = a as i32");
    }

    SECTION("Valid cast ZeroExt 1") {
        run_checker_expr_test("let a: u8 = 1_u8 let b: u32 = a as u32");
    }

    SECTION("Valid cast ZeroExt 2") {
        run_checker_expr_test("let a: u8 = 1_u8 let b: i32 = a as i32");
    }

    SECTION("Valid cast ZeroExt 3") {
        run_checker_expr_test("let a: i8 = -1_i8 let b: u32 = a as u32");
    }

    SECTION("Valid cast IntTrunc") {
        run_checker_expr_test("let a: i32 = 1 let b: i8 = a as i8");
    }

    SECTION("Valid cast NoOp ints") {
        run_checker_expr_test("let a: u32 = 1_u32 let b: i32 = a as i32");
    }

    SECTION("Valid cast SIntToFP") {
        run_checker_expr_test("let a: i32 = 1 let b: f64 = a as f64");
    }

    SECTION("Valid cast UIntToFP") {
        run_checker_expr_test("let a: u32 = 1_u32 let b: f64 = a as f64");
    }

    SECTION("Valid cast FPExt") {
        run_checker_expr_test("let a: f32 = 1.0_f32 let b: f64 = a as f64");
    }

    SECTION("Valid cast FPTrunc") {
        run_checker_expr_test("let a: f64 = 1.0 let b: f32 = a as f32");
    }

    SECTION("Valid cast FPToSInt") {
        run_checker_expr_test("let a: f64 = 1.0 let b: i32 = a as i32");
    }

    SECTION("Valid cast FPToUInt") {
        run_checker_expr_test("let a: f64 = 1.0 let b: u32 = a as u32");
    }

    SECTION("Invalid cast operation") {
        run_checker_expr_test(
            "let a: bool = true let b: () = a as ()",
            Err::InvalidCastOperation
        );
    }
}

TEST_CASE("Check pointer cast expressions", "[checker]") {
    SECTION("Valid nullptr cast to pointer") {
        run_checker_expr_test("let a: @i32 = nullptr as @i32");
    }
}

TEST_CASE("Check address-of expressions", "[checker]") {
    SECTION("Valid address-of expression 1") {
        run_checker_expr_test("let a = 1 let b: @i32 = @a");
    }

    SECTION("Valid address-of expression 2") {
        run_checker_expr_test("let var a = 1 let b: var@i32 = var@a");
    }

    SECTION("Valid address-of expression 3") {
        run_checker_expr_test("let var a = 1 let b: @i32 = var@a");
    }

    SECTION("Address-of mutability gain") {
        run_checker_expr_test(
            "let a = 1 let b: var@i32 = @a",
            Err::LetTypeMismatch
        );
    }

    SECTION("Valid pointer pointer 1") {
        run_checker_expr_test("let a = 1 let b: @i32 = @a let c: @@i32 = @b");
    }

    SECTION("Valid pointer pointer 2") {
        run_checker_expr_test(
            "let var a = 1 let b: var@i32 = var@a let c: @var@i32 = @b"
        );
    }

    SECTION("Pointer pointer mutability gain") {
        run_checker_expr_test(
            "let var a = 1 let b: @i32 = @a let c: @var@i32 = @b",
            Err::LetTypeMismatch
        );
    }

    SECTION("Address-of not an lvalue") {
        run_checker_expr_test(
            "let a = 1 let b: @i32 = @(a + 1)",
            Err::NotAPossibleLValue
        );
    }

    SECTION("Address-of immutable value as mutable") {
        run_checker_expr_test(
            "let a = 1 let b: var@i32 = var@a",
            Err::AddressOfImmutable
        );
    }
}

TEST_CASE("Check dereference expressions", "[checker]") {
    SECTION("Dereference without unsafe") {
        run_checker_expr_test(
            "let a = 1 let b: @i32 = @a let c = ^b",
            Err::PtrDerefOutsideUnsafeBlock
        );
    }

    SECTION("Valid dereference with unsafe") {
        run_checker_expr_test(
            "let a = 1 let b: @i32 = @a let c: i32 = unsafe { yield ^b }"
        );
    }

    SECTION("Dereference non-pointer") {
        run_checker_expr_test(
            "let a = 1 let b = ^a",
            Err::DereferenceNonPointer
        );
    }

    SECTION("Valid mutable pointer") {
        run_checker_expr_test(
            "let var a = 1 let b: var@i32 = var@a unsafe { ^b = 2 }"
        );
    }

    SECTION("Valid mutable pointer pointer") {
        run_checker_expr_test(
            "let var a = 1 let b = var@a let c = @b "
            "unsafe { ^^c = 2 }"
        );
    }

    SECTION("Assign to immutable via pointer") {
        run_checker_expr_test(
            "let a = 1 let b: @i32 = @a unsafe { ^b = 2 }",
            Err::AssignToImmutable
        );
    }

    SECTION("Dereference raw pointer with nullptr value") {
        run_checker_expr_test("let b: @i32 = nullptr unsafe { ^b }");
    }

    SECTION("Dereference nullptr type pointer") {
        run_checker_expr_test(
            "let p = nullptr unsafe { ^p }",
            Err::DereferenceNonTypedPointer
        );
    }

    SECTION("Unsafeness does not propagate") {
        run_checker_expr_test(
            "let a = 1 let b: @i32 = @a unsafe { block { ^b } }",
            Err::PtrDerefOutsideUnsafeBlock
        );
    }

    SECTION("Implicit dereference nullptr") {
        run_checker_expr_test(
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

TEST_CASE("Check binary expressions", "[checker]") {
    SECTION("Valid binary expressions 1") {
        run_checker_expr_test("let a = 1 + 2");
    }

    SECTION("Valid binary expressions 2") {
        run_checker_expr_test("let a = 1.0 + 2.0");
    }

    SECTION("Binary type mismatch 1") {
        run_checker_expr_test("let a = 1 + true", Err::NoOperatorOverload);
    }

    SECTION("Binary type mismatch 2") {
        run_checker_expr_test("let a = true + 1", Err::NoOperatorOverload);
    }

    SECTION("Binary type mismatch 3") {
        run_checker_expr_test("let a = true + false", Err::NoOperatorOverload);
    }
}

TEST_CASE("Check comparison expressions", "[checker]") {
    SECTION("Valid comparison expressions 1") {
        run_checker_expr_test("let a = 1 < 2");
    }

    SECTION("Valid comparison expressions 2") {
        run_checker_expr_test("let a = 1.0 >= 2.0");
    }

    SECTION("Valid comparison expressions 3") {
        run_checker_expr_test("let a = 1 == 1");
    }

    SECTION("Valid comparison expressions 4") {
        run_checker_expr_test("let a = 2.0 != 1.0");
    }

    SECTION("Valid comparison expressions 5") {
        run_checker_expr_test("let a = true == false");
    }

    SECTION("Valid comparison expressions 6") {
        run_checker_expr_test(
            "let a: @i32 = nullptr let b = a == nullptr let c = a != nullptr"
        );
    }

    SECTION("Valid comparison expressions 7") {
        run_checker_expr_test(
            "let a: @i32 = nullptr let b: @@i32 = @a let c = b == a"
        );
    }

    SECTION("Comparison type mismatch 1") {
        run_checker_expr_test("let a = 1 < true", Err::NoOperatorOverload);
    }

    SECTION("Comparison type mismatch 2") {
        run_checker_expr_test("let a = true >= 1", Err::NoOperatorOverload);
    }

    SECTION("Comparison type mismatch 3") {
        run_checker_expr_test("let a = true < false", Err::NoOperatorOverload);
    }

    SECTION("Comparison type mismatch 4") {
        run_checker_expr_test("let a = 1 == 1.0", Err::NoOperatorOverload);
    }

    SECTION("Comparison type mismatch 5") {
        run_checker_expr_test("let a = 1.0 != true", Err::NoOperatorOverload);
    }

    SECTION("Comparison type mismatch 6") {
        run_checker_expr_test("let a = \"\" == 1", Err::NoOperatorOverload);
    }
}

TEST_CASE("Check logical expressions", "[checker]") {
    SECTION("Valid logical expressions 1") {
        run_checker_expr_test("let a = true and false");
    }

    SECTION("Valid logical expressions 2") {
        run_checker_expr_test("let a = true or false and false");
    }

    SECTION("Valid logical expressions 3") {
        run_checker_expr_test("let a = true or not true");
    }

    SECTION("Logical type mismatch 1") {
        run_checker_expr_test("let a = 1 and true", Err::NoOperatorOverload);
    }

    SECTION("Logical type mismatch 2") {
        run_checker_expr_test("let a = true and 1", Err::NoOperatorOverload);
    }

    SECTION("Logical type mismatch 3") {
        run_checker_expr_test(
            "let a: i32 = true and true",
            Err::LetTypeMismatch
        );
    }
}

TEST_CASE("Check assignment expressions", "[checker]") {
    SECTION("Valid assignment expressions") {
        run_checker_expr_test("let var a = 1 a = 2");
    }

    SECTION("Assignment type mismatch 1") {
        run_checker_expr_test(
            "let var a: i32 = 1 a = true",
            Err::AssignmentTypeMismatch
        );
    }

    SECTION("Assignment type mismatch 2") {
        run_checker_expr_test(
            "let var a: i32 = 1 a = 1.0",
            Err::AssignmentTypeMismatch
        );
    }

    SECTION("Assignment not an lvalue 1") {
        run_checker_expr_test("1 = 2", Err::NotAPossibleLValue);
    }

    SECTION("Assignment not an lvalue 2") {
        run_checker_expr_test("(1 + 1) = 2", Err::NotAPossibleLValue);
    }

    SECTION("Assignment not an lvalue 3") {
        run_checker_expr_test(
            "let var a = 1; (a = 1) = 2",
            Err::NotAPossibleLValue
        );
    }

    SECTION("Assignment not an lvalue 4") {
        run_checker_expr_test("let var a = 1; -a = 2", Err::NotAPossibleLValue);
    }
}

TEST_CASE("Check block expressions", "[checker]") {
    SECTION("Valid block expression") {
        run_checker_expr_test("block { let a = 1 printout a }");
    }

    SECTION("Block expression yield 1") {
        run_checker_expr_test("block { let a = 1 printout a yield a }");
    }

    SECTION("Block expression yield 2") {
        run_checker_expr_test("let var a = 1 a = block { yield 2 }");
    }

    SECTION("Block expression nested yield") {
        run_checker_expr_test(
            "let var a = 1 a = block { yield block { yield 2 } }"
        );
    }

    SECTION("Block expression multiple yields") {
        run_checker_expr_test("let var a = 1 a = block { yield 2 yield 3 }");
    }

    SECTION("Block that yields nothing yields void 1") {
        run_checker_expr_test("let a: void = block { }");
    }

    SECTION("Block that yields nothing yields void 2") {
        run_checker_expr_test("let a: () = block { }");
    }

    SECTION("Yield outside local scope") {
        run_checker_expr_test("yield 1", Err::YieldOutsideLocalScope);
    }

    SECTION("Incompatible yield types") {
        run_checker_expr_test(
            "block { yield 1 yield true }",
            Err::YieldTypeMismatch
        );
    }

    SECTION("Block without yield") {
        run_checker_expr_test(
            "let var a = 1 a = block { let b = 1 }",
            Err::AssignmentTypeMismatch
        );
    }
}

TEST_CASE("Check tuple expressions", "[checker]") {
    SECTION("Valid tuple expression 1") {
        run_checker_expr_test("let a = (1, 2.1, true)");
    }

    SECTION("Valid tuple expression 2") {
        run_checker_expr_test("let a: (i32, f64, bool) = (1, 2.1, true)");
    }

    SECTION("Tuple expression type mismatch") {
        run_checker_expr_test(
            "let a: (i32, bool, f64) = (1, 2, true)",
            Err::LetTypeMismatch
        );
    }

    SECTION("Tuple access valid") {
        run_checker_expr_test(
            "let a = (1, 2.1, true) let b: i32 = a.0 let c: f64 = a.1 "
            "let d: bool = a.2"
        );
    }

    SECTION("Tuple access invalid index") {
        run_checker_expr_test(
            "let a = (1, 2.1, true) let b = a.3",
            Err::TupleIndexOutOfBounds
        );
    }

    SECTION("Tuple access invalid right side") {
        run_checker_expr_test(
            "let a = (1, 2.1, true) let b = a.x",
            Err::InvalidTupleAccess
        );
    }

    SECTION("Tuple access as valid lvalue") {
        run_checker_expr_test(
            "let var a = (1, 2.1, true) a.0 = 2 a.1 = 3.5 a.2 = false"
        );
    }

    SECTION("Tuple access assign to immutable") {
        run_checker_expr_test(
            "let a = (1, 2.1, true) a.0 = 2",
            Err::AssignToImmutable
        );
    }

    SECTION("Tuple access out of bounds 1") {
        run_checker_expr_test(
            "let var a = (1, 2.1, true) a.3 = 2",
            Err::TupleIndexOutOfBounds
        );
    }

    SECTION("Tuple access out of bounds 2") {
        run_checker_expr_test(
            "let var a = () a.0 = 2",
            Err::TupleIndexOutOfBounds
        );
    }

    SECTION("Tuple implicit dereference 1") {
        run_checker_expr_test(
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
        run_checker_expr_test(
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
        run_checker_expr_test(
            R"(
            let var a = (1,) 
            let p = @a
            let c = p.0
            )",
            Err::PtrDerefOutsideUnsafeBlock
        );
    }
}

TEST_CASE("Check array expressions", "[checker]") {
    SECTION("Valid array expression 1") {
        run_checker_expr_test("let a = [1, 2, 3, 4, 5]");
    }

    SECTION("Valid array expression 2") {
        run_checker_expr_test("let a: [i32; 3] = [1, 2, 3]");
    }

    SECTION("Array element type mismatch") {
        run_checker_expr_test(
            "let a: [i32; 3] = [1, 2.0, 3]",
            Err::ArrayElementTypeMismatch
        );
    }

    SECTION("Array type mismatch") {
        run_checker_expr_test(
            "let a: [i32; 4] = [1, 2, 3]",
            Err::LetTypeMismatch
        );
    }

    SECTION("Empty array expression") {
        run_checker_expr_test("let a: [i32; 0] = []");
    }

    SECTION("Unsized type allocation 1") {
        run_checker_expr_test(
            "let a: [i32; ?] = [1, 2, 3]",
            Err::UnsizedTypeAllocation
        );
    }

    SECTION("Unsized type allocation 2") {
        run_checker_expr_test(
            "let a: ([i32; ?]) = ([1],)",
            Err::UnsizedTypeAllocation
        );
    }

    SECTION("Unsized type under pointer") {
        run_checker_expr_test(
            "let a: [i32; 3] = [1, 2, 3] let b: @[i32; ?] = @a"
        );
    }

    SECTION("Unsized array pointer cast") {
        run_checker_expr_test(
            "let a: [i32; 3] = [1, 2, 3] let b = @a as @[i32; ?]"
        );
    }

    SECTION("Unsized array access") {
        run_checker_expr_test(
            "let a: [i32; 3] = [1, 2, 3] let b = @a as @[i32; ?] "
            "let var c: i32 unsafe { c = b[0] }"
        );
    }

    SECTION("Unsized type as rvalue") {
        run_checker_expr_test(
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
        run_checker_expr_test(
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
        run_checker_expr_test(
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
        run_checker_expr_test(
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
        run_checker_expr_test(
            R"(
            let var a: [i32; 3] = [1, 2, 3]
            let p = @a
            let c = p[0]
            )",
            Err::PtrDerefOutsideUnsafeBlock
        );
    }

    SECTION("Array assignment") {
        run_checker_expr_test("let a = [1, 2, 3] let b = a");
    }
}

TEST_CASE("Check subscript expressions", "[checker]") {
    SECTION("Valid subscript expression 1") {
        run_checker_expr_test("let a = [10, 20, 30] let b: i32 = a[1]");
    }

    SECTION("Valid subscript expression 2") {
        run_checker_expr_test(
            "let a = [[1,2], [3,4], [5,6]] let b: i32 = a[2][0]"
        );
    }

    SECTION("Subscript index type mismatch") {
        run_checker_expr_test(
            "let a = [10, 20, 30] let b = a[true]",
            Err::ArrayIndexNotInteger
        );
    }

    SECTION("Subscript on non-array type") {
        run_checker_expr_test(
            "let a = 10 let b = a[0]",
            Err::OperatorNotValidForExpr
        );
    }

    SECTION("Subscript as valid lvalue") {
        run_checker_expr_test("let var a = [10, 20, 30] a[1] = 25");
    }

    SECTION("Subscript assign to immutable") {
        run_checker_expr_test(
            "let a = [10, 20, 30] a[1] = 25",
            Err::AssignToImmutable
        );
    }
}

// TODO: Object access needs further testing.

TEST_CASE("Check object expressions", "[checker]") {
    SECTION("Valid object expression 1") {
        run_checker_expr_test("let a = { x: 1, y: 2.0 }");
    }

    SECTION("Valid object expression 2") {
        run_checker_expr_test("let a: { x: i32, y: f64 } = { x: 1, y: 2.0 }");
    }

    SECTION("Object field type mismatch") {
        run_checker_expr_test(
            "let a: { x: i32, y: f64 } = { x: 1, y: true }",
            Err::LetTypeMismatch
        );
    }

    SECTION("Object type mismatch") {
        run_checker_expr_test(
            "let a: { x: i32, z: f64 } = { x: 1, y: 2.0 }",
            Err::LetTypeMismatch
        );
    }

    SECTION("Object fields in wrong order") {
        run_checker_expr_test(
            "let a: { x: i32, y: f64 } = { y: 2.0, x: 1 }",
            Err::LetTypeMismatch
        );
    }

    SECTION("Object access valid") {
        run_checker_expr_test(
            "let a = { x: 1, y: 2.0 } let b: i32 = a.x let c: f64 = a.y"
        );
    }

    SECTION("Object field assignable") {
        run_checker_expr_test(
            "let var a = { var x: 1, var y: 2.0 } a.x = 10 a.y = 3.5"
        );
    }

    SECTION("Object field not assignable") {
        run_checker_expr_test(
            "let a = { x: 1, y: 2.0 } a.x = 10",
            Err::AssignToImmutable
        );
    }

    SECTION("Object immutable with var field") {
        run_checker_expr_test(
            "let a = { var x: 1, y: 2.0 } a.x = 10",
            Err::AssignToImmutable
        );
    }

    SECTION("Object immutable with mut field") {
        run_checker_expr_test("let a = { mut x: 1, y: 2.0 } a.x = 10");
    }

    SECTION("Object field with reserved name OK") {
        run_checker_expr_test("let var a = { var i32: 1 } a.i32 = 2");
    }
}

TEST_CASE("Check conditional expressions", "[checker]") {
    SECTION("Valid conditional expression 1") {
        run_checker_expr_test("if true { 1 } else { false }");
    }

    SECTION("Valid conditional expression 2") {
        run_checker_expr_test(R"(
        if true:
            1
        else:
            2
        )");
    }

    SECTION("Valid conditional expression 3") {
        run_checker_expr_test("let a: i32 = if true then 1 else 2");
    }

    SECTION("Valid conditional expression 4") {
        run_checker_expr_test("if true {}");
    }

    SECTION("Valid if else if expression 1") {
        run_checker_expr_test(R"(
        if false:
            1
        else if true:
            2
        else:
            3
        )");
    }

    SECTION("Valid if else if expression 2") {
        run_checker_expr_test(R"(
        if false then 1 else if true then 2 else 3
        )");
    }

    SECTION("Conditional condition not bool") {
        run_checker_expr_test("if 1 { 1 } else { 2 }", Err::ConditionNotBool);
    }

    SECTION("Conditional branch type mismatch 1") {
        run_checker_expr_test(
            "if true { yield 1 } else { yield false }",
            Err::ConditionalBranchTypeMismatch
        );
    }

    SECTION("Conditional branch type mismatch 2") {
        run_checker_expr_test(
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
        run_checker_expr_test(
            "let a: i32 = if true then 1 else false",
            Err::ConditionalBranchTypeMismatch
        );
    }

    SECTION("Conditional branch type mismatch 4") {
        run_checker_expr_test(
            "if true then 1",
            Err::ConditionalBranchTypeMismatch
        );
    }

    SECTION("If else if branch type mismatch") {
        run_checker_expr_test(
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
        run_checker_expr_test(
            "let a: bool = if true then 1 else 2",
            Err::LetTypeMismatch
        );
    }

    SECTION("Yield type mismatch with conditional") {
        run_checker_expr_test(
            "if true { yield 1 yield 2.0 } else { yield 3 }",
            Err::YieldTypeMismatch
        );
    }

    SECTION("Conditional with many errors") {
        run_checker_expr_test(
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

TEST_CASE("Check loop expressions", "[checker]") {
    SECTION("Valid loop expression 1") {
        run_checker_expr_test("loop { printout \"Hello, World!\" }");
    }

    SECTION("Valid loop expression 2") {
        run_checker_expr_test(
            "let cond = true while cond { printout \"Hello, World!\" }"
        );
    }

    SECTION("Valid loop expression 3") {
        run_checker_expr_test(R"(
        let cond = true
        while cond:
            printout "Hello, World!"
        )");
    }

    SECTION("Valid loop expression 4") {
        run_checker_expr_test(
            "let cond = true do { printout \"Hello, World!\" } while cond"
        );
    }

    SECTION("Valid loop expression 5") {
        run_checker_expr_test(R"(
        let result = loop:
            break 1
        )");
    }

    SECTION("Valid loop expression 6") {
        run_checker_expr_test(R"(
        let result = while true:
            break 1
        )");
    }

    SECTION("Valid loop expression 7") {
        run_checker_expr_test(R"(
        let result = 
        do:
            break 1
        while true
        )");
    }

    SECTION("Valid short loop expression 1") {
        run_checker_expr_test("let var x = 1 loop x = x + 1");
    }

    SECTION("Valid short loop expression 2") {
        run_checker_expr_test("let var x = 1 while false do x = x + 1");
    }

    SECTION("Valid short loop expression 3") {
        run_checker_expr_test("let var x = 1 do x = x + 1 while false");
    }

    SECTION("Loop condition not bool") {
        run_checker_expr_test("while 1 { }", Err::ConditionNotBool);
    }

    SECTION("While loop yielding non-unit") {
        run_checker_expr_test(
            "let result = while false { yield 1 }",
            Err::WhileLoopYieldingNonVoid
        );
    }

    SECTION("Do-while loop yielding non-unit") {
        run_checker_expr_test(
            "let result = do { yield 1 } while false",
            Err::WhileLoopYieldingNonVoid
        );
    }

    SECTION("Break outside loop") {
        run_checker_expr_test("break ()", Err::BreakOutsideLoop);
    }

    SECTION("Continue outside loop") {
        run_checker_expr_test("continue", Err::ContinueOutsideLoop);
    }
}
