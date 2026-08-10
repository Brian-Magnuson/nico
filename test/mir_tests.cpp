#include <catch2/catch_test_macros.hpp>

#include "nico/frontend/components/global_checker.h"
#include "nico/frontend/components/lexer.h"
#include "nico/frontend/components/local_checker.h"
#include "nico/frontend/components/mir_builder.h"
#include "nico/frontend/components/parser.h"
#include "nico/shared/diagnostics.h"

#include "nico/shared/utils.h"
#include "test_utils.h"

/**
 * @brief An options struct for configuring MIR tests. This allows for more
 * flexible and comprehensive testing of various MIR behaviors, including
 * expected outputs, error codes, panics, and more.
 */
struct MIRTestOptions {
    // The expected output of the JIT, if any. If not provided, the output will
    // not be checked.
    std::string_view expected_output;
    // Whether to print the symbol tree before building MIR. Defaults to false.
    bool print_symbol_tree = false;
    // Whether to print the MIR after building it. Defaults to false.
    bool print_mir = false;
};

void run_mir_test(
    std::string_view source, MIRTestOptions options = MIRTestOptions()
) {
    nico::MIRValue::reset_counters();
    nico::BasicBlock::reset_counters();
    auto context = std::make_unique<nico::FrontendContext>();

    auto file = nico::make_test_code_file(source);
    nico::Lexer::scan(context, file);
    nico::Parser::parse(context);
    nico::GlobalChecker::check(context);
    nico::LocalChecker::check(context);

    if (options.print_symbol_tree) {
        std::cout << context->symbol_tree->to_tree_string() << "\n";
    }

    nico::MIRBuilder::build_mir(context);
    if (options.print_mir) {
        std::cout << context->mir_module->to_string() << "\n";
    }

    CHECK(
        nico::remove_empty_lines(context->mir_module->to_string()) ==
        nico::remove_empty_lines(options.expected_output)
    );

    context->initialize();
    nico::Diagnostics::inst().reset();
}

void run_mir_test(std::string_view source, std::string_view expected_output) {
    run_mir_test(source, MIRTestOptions{expected_output});
}

TEST_CASE("MIR basic generation", "[mir]") {
    SECTION("Basic let statement") {
        run_mir_test(R"(let x = 5)", MIRTestOptions{.expected_output = R"(module

func $script( ) -> void {
  entry#0 <-- [ ]
    alloca i32 (var@i32 ::x)
    store (i32 5) -> (var@i32 ::x)
    jump exit#0
  exit#0 <-- [ entry#0 ]
    return
}
)"});
    }

    SECTION("Local variable read and write") {
        run_mir_test(
            R"(
            let x = 5
            let y = x
            )",
            MIRTestOptions{.expected_output = R"(module
func $script( ) -> void {
  entry#0 <-- [ ]
    alloca i32 (var@i32 ::x)
    store (i32 5) -> (var@i32 ::x)
    alloca i32 (var@i32 ::y)
    load (var@i32 ::x) -> (i32 #0)
    store (i32 #0) -> (var@i32 ::y)
    jump exit#0
  exit#0 <-- [ entry#0 ]
    return
}
)"}
        );
    }

    SECTION("Print statement") {
        run_mir_test(
            R"(
            let x = 5
            printout x
            )",
            MIRTestOptions{.expected_output = R"(module
func $script( ) -> void {
  entry#0 <-- [ ]
    alloca i32 (var@i32 ::x)
    store (i32 5) -> (var@i32 ::x)
    load (var@i32 ::x) -> (i32 #0)
    printout (i32 #0)
    jump exit#0
  exit#0 <-- [ entry#0 ]
    return
}
)"}

        );
    }

    SECTION("Hello world") {
        run_mir_test(
            R"(
            printout "Hello, world!"
            )",
            MIRTestOptions{.expected_output = R"(module
func $script( ) -> void {
  entry#0 <-- [ ]
    printout (str "Hello, world!")
    jump exit#0
  exit#0 <-- [ entry#0 ]
    return
}
)"}
        );
    }

    SECTION("Addition") {
        run_mir_test(
            R"(
            let x = 5
            let y = 10
            let z = x + y
            )",
            MIRTestOptions{.expected_output = R"(module
func $script( ) -> void {
  entry#0 <-- [ ]
    alloca i32 (var@i32 ::x)
    store (i32 5) -> (var@i32 ::x)
    alloca i32 (var@i32 ::y)
    store (i32 10) -> (var@i32 ::y)
    alloca i32 (var@i32 ::z)
    load (var@i32 ::x) -> (i32 #0)
    load (var@i32 ::y) -> (i32 #1)
    binary intadd (i32 #0) (i32 #1) -> (i32 #2)
    store (i32 #2) -> (var@i32 ::z)
    jump exit#0
  exit#0 <-- [ entry#0 ]
    return
}
)"}
        );
    }
}

TEST_CASE("MIR arrays", "[mir]") {
    SECTION("Array creation and access") {
        run_mir_test(
            R"(
            let arr = [1, 2, 3, 4, 5]
            )",
            MIRTestOptions{.expected_output = R"(module
func $script( ) -> void {
  entry#0 <-- [ ]
    alloca [i32; 5] (var@[i32; 5] ::arr)
    array [ (i32 1) (i32 2) (i32 3) (i32 4) (i32 5) ] -> ([i32; 5] #0)
    store ([i32; 5] #0) -> (var@[i32; 5] ::arr)
    jump exit#0
  exit#0 <-- [ entry#0 ]
    return
})"}
        );
    }

    SECTION("Array creation with variable elements") {
        run_mir_test(
            R"(
            let a = 1
            let b = 2
            let arr = [a, b, 3]
            )",
            MIRTestOptions{.expected_output = R"(module
func $script( ) -> void {
  entry#0 <-- [ ]
    alloca i32 (var@i32 ::a)
    store (i32 1) -> (var@i32 ::a)
    alloca i32 (var@i32 ::b)
    store (i32 2) -> (var@i32 ::b)
    alloca [i32; 3] (var@[i32; 3] ::arr)
    load (var@i32 ::a) -> (i32 #0)
    load (var@i32 ::b) -> (i32 #1)
    array [ (i32 #0) (i32 #1) (i32 3) ] -> ([i32; 3] #2)
    store ([i32; 3] #2) -> (var@[i32; 3] ::arr)
    jump exit#0
  exit#0 <-- [ entry#0 ]
    return
})"}
        );
    }
}

TEST_CASE("MIR casting", "[mir]") {
    SECTION("SIntToFP cast") {
        run_mir_test(
            R"(
            let x = 5
            let y = x as f32
            )",
            MIRTestOptions{.expected_output = R"(module
func $script( ) -> void {
  entry#0 <-- [ ]
    alloca i32 (var@i32 ::x)
    store (i32 5) -> (var@i32 ::x)
    alloca f32 (var@f32 ::y)
    load (var@i32 ::x) -> (i32 #0)
    cast sinttofp (i32 #0) -> (f32 #1)
    store (f32 #1) -> (var@f32 ::y)
    jump exit#0
  exit#0 <-- [ entry#0 ]
    return
})"}
        );
    }
}

TEST_CASE("MIR sizeof", "[mir]") {
    SECTION("Size of i32") {
        run_mir_test(
            R"(
            let size = sizeof i32
            )",
            MIRTestOptions{.expected_output = R"(module
func $script( ) -> void {
  entry#0 <-- [ ]
    alloca u64 (var@u64 ::size)
    sizeof i32 -> (u64 #0)
    store (u64 #0) -> (var@u64 ::size)
    jump exit#0
  exit#0 <-- [ entry#0 ]
    return
})"}
        );
    }
}

TEST_CASE("MIR conditional expressions", "[mir]") {
    SECTION("Simple conditional expression") {
        run_mir_test(
            R"(
            let x = 5
            let y = if x > 0 then 10 else 20
            )",
            MIRTestOptions{.expected_output = R"(module
func $script( ) -> void {
  entry#0 <-- [ ]
    alloca i32 (var@i32 ::x)
    store (i32 5) -> (var@i32 ::x)
    alloca i32 (var@i32 ::y)
    load (var@i32 ::x) -> (i32 #0)
    binary sintgt (i32 #0) (i32 0) -> (bool #1)
    branch (bool #1) ? cond_then#0 : cond_else#0
  exit#0 <-- [ cond_merge#0 ]
    return
  cond_then#0 <-- [ entry#0 ]
    jump cond_merge#0
  cond_else#0 <-- [ entry#0 ]
    jump cond_merge#0
  cond_merge#0 <-- [ cond_then#0 cond_else#0 ]
    phi [cond_then#0: (i32 10)] [cond_else#0: (i32 20)] -> (i32 #2)
    store (i32 #2) -> (var@i32 ::y)
    jump exit#0
})"}
        );
    }
}
