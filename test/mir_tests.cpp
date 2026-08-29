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

/**
 * @brief Runs a MIR test with the given source code and options.
 *
 * @param source The source code to scan, parse, analyze, and run in the MIR
 * builder.
 * @param options The options for the test, including expected output and
 * printing flags. See the MIRTestOptions struct for more details on the
 * available options and their effects.
 */
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

/**
 * @brief Runs a simple MIR test with the given source code and expected output.
 *
 * This function is only for checking the expected output of the MIR. For all
 * other checks, use the overload that accepts MIRTestOptions.
 *
 * @param source The source code to scan, parse, analyze, and run in the MIR
 * builder.
 * @param expected_output The expected output of the MIR. If the output does not
 * match, the test will fail.
 */
void run_mir_test(std::string_view source, std::string_view expected_output) {
    run_mir_test(source, MIRTestOptions{expected_output});
}

TEST_CASE("MIR basic generation", "[mir]") {
    SECTION("Basic let statement") {
        run_mir_test(R"(let x = 5)", MIRTestOptions{.expected_output = R"(module
global ::x (var@i32 ::x)

func $script( ) -> void {
  entry#0 <-- [ ]
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
global ::x (var@i32 ::x)
global ::y (var@i32 ::y)

func $script( ) -> void {
  entry#0 <-- [ ]
    store (i32 5) -> (var@i32 ::x)
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
global ::x (var@i32 ::x)

func $script( ) -> void {
  entry#0 <-- [ ]
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
global ::x (var@i32 ::x)
global ::y (var@i32 ::y)
global ::z (var@i32 ::z)

func $script( ) -> void {
  entry#0 <-- [ ]
    store (i32 5) -> (var@i32 ::x)
    store (i32 10) -> (var@i32 ::y)
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
global ::arr (var@[i32; 5] ::arr)

func $script( ) -> void {
  entry#0 <-- [ ]
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
global ::a (var@i32 ::a)
global ::b (var@i32 ::b)
global ::arr (var@[i32; 3] ::arr)

func $script( ) -> void {
  entry#0 <-- [ ]
    store (i32 1) -> (var@i32 ::a)
    store (i32 2) -> (var@i32 ::b)
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
global ::x (var@i32 ::x)
global ::y (var@f32 ::y)

func $script( ) -> void {
  entry#0 <-- [ ]
    store (i32 5) -> (var@i32 ::x)
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
global ::size (var@u64 ::size)

func $script( ) -> void {
  entry#0 <-- [ ]
    sizeof i32 -> (u64 #0)
    store (u64 #0) -> (var@u64 ::size)
    jump exit#0
  exit#0 <-- [ entry#0 ]
    return
})"}
        );
    }
}

TEST_CASE("MIR block expressions", "[mir]") {
    SECTION("Simple block expression") {
        run_mir_test(
            R"(
            let x = block { yield 5 }
            )",
            MIRTestOptions{.expected_output = R"(module
global ::x (var@i32 ::x)

func $script( ) -> void {
  entry#0 <-- [ ]
    local i32 (var@i32 $yieldval#0)
    store (i32 5) -> (var@i32 $yieldval#0)
    load (var@i32 $yieldval#0) -> (i32 #0)
    store (i32 #0) -> (var@i32 ::x)
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
global ::x (var@i32 ::x)
global ::y (var@i32 ::y)

func $script( ) -> void {
  entry#0 <-- [ ]
    store (i32 5) -> (var@i32 ::x)
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

TEST_CASE("MIR alloc and dealloc", "[mir]") {
    SECTION("Basic alloc and dealloc") {
        run_mir_test(
            R"(
            let ptr = alloc i32
            unsafe:
              dealloc ptr
            )",
            MIRTestOptions{.expected_output = R"(module
global ::ptr (var@var@i32 ::ptr)

func $script( ) -> void {
  entry#0 <-- [ ]
    sizeof i32 -> (u64 #0)
    alloc (u64 #0) -> (anyptr #1)
    store (anyptr #1) -> (var@var@i32 ::ptr)
    local void (var@void $yieldval#0)
    load (var@var@i32 ::ptr) -> (var@i32 #2)
    dealloc (var@i32 #2)
    load (var@void $yieldval#0) -> (void #3)
    jump exit#0
  exit#0 <-- [ entry#0 ]
    return
})"}
        );
    }
}

TEST_CASE("MIR loops", "[mir]") {
    SECTION("Simple while loop") {
        run_mir_test(
            R"(
            let var x = 0
            while x < 5 {
                x = x + 1
            }
            )",
            MIRTestOptions{.expected_output = R"(module
global ::x (var@i32 ::x)

func $script( ) -> void {
  entry#0 <-- [ ]
    store (i32 0) -> (var@i32 ::x)
    local void (var@void $breakval#0)
    jump loop_cond#0
  exit#0 <-- [ loop_merge#0 ]
    return
  loop_do#0 <-- [ loop_cond#0 ]
    local void (var@void $yieldval#0)
    load (var@i32 ::x) -> (i32 #2)
    binary intadd (i32 #2) (i32 1) -> (i32 #3)
    store (i32 #3) -> (var@i32 ::x)
    load (var@void $yieldval#0) -> (void #4)
    jump loop_cond#0
  loop_merge#0 <-- [ loop_cond#0 ]
    load (var@void $breakval#0) -> (void #5)
    jump exit#0
  loop_cond#0 <-- [ entry#0 loop_do#0 ]
    load (var@i32 ::x) -> (i32 #0)
    binary sintlt (i32 #0) (i32 5) -> (bool #1)
    branch (bool #1) ? loop_do#0 : loop_merge#0
})"}
        );
    }

    SECTION("While loop with break") {
        run_mir_test(
            R"(
          let x = true
          while x {
              break void
          }
          )",
            MIRTestOptions{.expected_output = R"(module
global ::x (var@bool ::x)

func $script( ) -> void {
  entry#0 <-- [ ]
    store (bool true) -> (var@bool ::x)
    local void (var@void $breakval#0)
    jump loop_cond#0
  exit#0 <-- [ loop_merge#0 ]
    return
  loop_do#0 <-- [ loop_cond#0 ]
    local void (var@void $yieldval#0)
    store (void void) -> (var@void $breakval#0)
    jump loop_merge#0
  loop_merge#0 <-- [ loop_cond#0 loop_do#0 ]
    load (var@void $breakval#0) -> (void #2)
    jump exit#0
  loop_cond#0 <-- [ entry#0 unreachable#0 ]
    load (var@bool ::x) -> (bool #0)
    branch (bool #0) ? loop_do#0 : loop_merge#0
  unreachable#0 <-- [ ]
    load (var@void $yieldval#0) -> (void #1)
    jump loop_cond#0
})"}
        );
    }

    SECTION("While loop with continue") {
        run_mir_test(
            R"(
          let x = true
          while x {
              continue
          }
          )",
            MIRTestOptions{.expected_output = R"(module
global ::x (var@bool ::x)

func $script( ) -> void {
  entry#0 <-- [ ]
    store (bool true) -> (var@bool ::x)
    local void (var@void $breakval#0)
    jump loop_cond#0
  exit#0 <-- [ loop_merge#0 ]
    return
  loop_do#0 <-- [ loop_cond#0 ]
    local void (var@void $yieldval#0)
    jump loop_cond#0
  loop_merge#0 <-- [ loop_cond#0 ]
    load (var@void $breakval#0) -> (void #2)
    jump exit#0
  loop_cond#0 <-- [ entry#0 loop_do#0 unreachable#0 ]
    load (var@bool ::x) -> (bool #0)
    branch (bool #0) ? loop_do#0 : loop_merge#0
  unreachable#0 <-- [ ]
    load (var@void $yieldval#0) -> (void #1)
    jump loop_cond#0
})"}
        );
    }

    SECTION("Simple do-while loop") {
        run_mir_test(
            R"(
            let var x = 0
            do {
                x = x + 1
            } while x < 5
            )",
            MIRTestOptions{.expected_output = R"(module
global ::x (var@i32 ::x)

func $script( ) -> void {
  entry#0 <-- [ ]
    store (i32 0) -> (var@i32 ::x)
    local void (var@void $breakval#0)
    jump loop_do#0
  exit#0 <-- [ loop_merge#0 ]
    return
  loop_do#0 <-- [ entry#0 loop_cond#0 ]
    local void (var@void $yieldval#0)
    load (var@i32 ::x) -> (i32 #2)
    binary intadd (i32 #2) (i32 1) -> (i32 #3)
    store (i32 #3) -> (var@i32 ::x)
    load (var@void $yieldval#0) -> (void #4)
    jump loop_cond#0
  loop_merge#0 <-- [ loop_cond#0 ]
    load (var@void $breakval#0) -> (void #5)
    jump exit#0
  loop_cond#0 <-- [ loop_do#0 ]
    load (var@i32 ::x) -> (i32 #0)
    binary sintlt (i32 #0) (i32 5) -> (bool #1)
    branch (bool #1) ? loop_do#0 : loop_merge#0
})"}
        );
    }
}
