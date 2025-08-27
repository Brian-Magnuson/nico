#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <llvm/Support/Error.h>

#include "../src/checker/global_checker.h"
#include "../src/checker/local_checker.h"
#include "../src/checker/symbol_tree.h"
#include "../src/codegen/code_generator.h"
#include "../src/common/code_file.h"
#include "../src/compiler/jit.h"
#include "../src/debug/test_utils.h"
#include "../src/lexer/lexer.h"
#include "../src/lexer/token.h"
#include "../src/logger/logger.h"
#include "../src/parser/ast.h"
#include "../src/parser/parser.h"

#include "../src/debug/ast_printer.h"

void run_compile_test(
    Lexer& lexer,
    Parser& parser,
    GlobalChecker& global_checker,
    LocalChecker& local_checker,
    CodeGenerator& codegen,
    std::unique_ptr<IJit>& jit,
    std::string_view source,
    std::optional<std::string_view> expected_output = std::nullopt,
    std::optional<int> expected_return_code = std::nullopt
) {
    auto file = make_test_code_file(source);

    auto tokens = lexer.scan(file);
    auto ast = parser.parse(std::move(tokens));
    global_checker.check(ast);
    local_checker.check(ast);
    codegen.generate(ast, false);
    REQUIRE(codegen.generate_main());
    auto output = codegen.eject();
    // llvm::outs() << "===========================================================\n";
    // output.module->print(llvm::outs(), nullptr);
    auto error = jit->add_module(std::move(output.module), std::move(output.context));

    std::optional<llvm::Expected<int>> return_code;

    auto [out, err] = capture_stdout([&]() {
        return_code = jit->run_main(0, nullptr);
    });

    if (expected_output) {
        CHECK(out == *expected_output);
    }
    if (expected_return_code) {
        REQUIRE(return_code.has_value());
        REQUIRE(*return_code);
        CHECK(return_code->get() == *expected_return_code);
    }
}

TEST_CASE("JIT print statements", "[jit]") {
    Lexer lexer;
    Parser parser;
    std::shared_ptr<SymbolTree> symbol_tree = std::make_shared<SymbolTree>();
    GlobalChecker global_checker(symbol_tree);
    LocalChecker local_checker(symbol_tree);
    CodeGenerator codegen;
    std::unique_ptr<IJit> jit = std::make_unique<SimpleJit>();
    Logger::inst().set_printing_enabled(true);

    SECTION("Print hello world 1") {
        run_compile_test(
            lexer,
            parser,
            global_checker,
            local_checker,
            codegen,
            jit,
            R"(printout "Hello, World!")",
            "Hello, World!"
        );
    }

    SECTION("Print hello world 2") {
        run_compile_test(
            lexer,
            parser,
            global_checker,
            local_checker,
            codegen,
            jit,
            R"(printout "Hello, World!" printout "Goodbye, World!")",
            "Hello, World!Goodbye, World!"
        );
    }

    SECTION("Print hello world 3") {
        run_compile_test(
            lexer,
            parser,
            global_checker,
            local_checker,
            codegen,
            jit,
            R"(printout "Hello, World!\n" printout "Goodbye, World!")",
            "Hello, World!\nGoodbye, World!"
        );
    }

    SECTION("Print hello world 4") {
        run_compile_test(
            lexer,
            parser,
            global_checker,
            local_checker,
            codegen,
            jit,
            R"(printout "Hello", ", World!")",
            "Hello, World!"
        );
    }

    lexer.reset();
    parser.reset();
    symbol_tree->reset();
    codegen.reset();
    jit->reset();
}

TEST_CASE("JIT let statements", "[jit]") {
    Lexer lexer;
    Parser parser;
    std::shared_ptr<SymbolTree> symbol_tree = std::make_shared<SymbolTree>();
    GlobalChecker global_checker(symbol_tree);
    LocalChecker local_checker(symbol_tree);
    CodeGenerator codegen;
    std::unique_ptr<IJit> jit = std::make_unique<SimpleJit>();
    Logger::inst().set_printing_enabled(true);

    SECTION("Basic variable reference") {
        run_compile_test(
            lexer,
            parser,
            global_checker,
            local_checker,
            codegen,
            jit,
            R"(let x = 5 printout x)",
            "5"
        );
    }

    lexer.reset();
    parser.reset();
    symbol_tree->reset();
    codegen.reset();
    jit->reset();
}

TEST_CASE("JIT operators", "[jit]") {
    Lexer lexer;
    Parser parser;
    std::shared_ptr<SymbolTree> symbol_tree = std::make_shared<SymbolTree>();
    GlobalChecker global_checker(symbol_tree);
    LocalChecker local_checker(symbol_tree);
    CodeGenerator codegen;
    std::unique_ptr<IJit> jit = std::make_unique<SimpleJit>();
    Logger::inst().set_printing_enabled(true);

    SECTION("Unary minus") {
        run_compile_test(
            lexer,
            parser,
            global_checker,
            local_checker,
            codegen,
            jit,
            R"(let x = -5 printout x, ",", -x)",
            "-5,5"
        );
    }

    SECTION("Divide by zero") {
        codegen.set_panic_recoverable(true);
        run_compile_test(
            lexer,
            parser,
            global_checker,
            local_checker,
            codegen,
            jit,
            R"(printout 1 / 0)",
            std::nullopt,
            101
        );
    }

    lexer.reset();
    parser.reset();
    symbol_tree->reset();
    codegen.reset();
    jit->reset();
}
