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

func $script( ) -> () {
  exit#0 <-- [ entry#0 ]
    return
  entry#0 <-- [ ]
    alloca i32 (var@i32 ::x)
    store (i32 5) -> (var@i32 ::x)
    jump exit#0
}
)"});
    }
}

TEST_CASE("MIR arrays", "[mir]") {
    SECTION("Array creation and access") {
        run_mir_test(
            R"(
            let arr = [1, 2, 3, 4, 5]
            )",
            MIRTestOptions{.expected_output = R"(module
func $script( ) -> () {
  exit#0 <-- [ entry#0 ]
    return
  entry#0 <-- [ ]
    alloca [i32; 5] (var@[i32; 5] ::arr)
    array [ (i32 1) (i32 2) (i32 3) (i32 4) (i32 5) ] -> ([i32; 5] #0)
    store ([i32; 5] #0) -> (var@[i32; 5] ::arr)
    jump exit#0
})"}
        );
    }
}
