#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <utility>
#include <vector>

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
        auto file = make_test_code_file("print \"Hello, World!\"");

        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        codegen.generate(ast, false);
        REQUIRE(codegen.generate_main());
        auto output = codegen.eject();
        auto err = jit->add_module(std::move(output.module), std::move(output.context));
        auto printout = capture_stdout([&]() {
            auto result = jit->run_main(0, nullptr);
        });

        CHECK(printout == "Hello, World!");
    }

    SECTION("Print hello world 2") {
        auto file = make_test_code_file(R"(print "Hello, World!" print "Goodbye, World!")");

        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));

        global_checker.check(ast);
        local_checker.check(ast);
        codegen.generate(ast, false);
        REQUIRE(codegen.generate_main());
        auto output = codegen.eject();
        auto err = jit->add_module(std::move(output.module), std::move(output.context));
        auto printout = capture_stdout([&]() {
            auto result = jit->run_main(0, nullptr);
        });

        CHECK(printout == "Hello, World!Goodbye, World!");
    }

    SECTION("Print hello world 3") {
        auto file = make_test_code_file(R"(print "Hello, World!\n" print "Goodbye, World!")");

        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));

        global_checker.check(ast);
        local_checker.check(ast);
        codegen.generate(ast, false);
        REQUIRE(codegen.generate_main());
        auto output = codegen.eject();
        auto err = jit->add_module(std::move(output.module), std::move(output.context));
        auto printout = capture_stdout([&]() {
            auto result = jit->run_main(0, nullptr);
        });

        CHECK(printout == "Hello, World!\nGoodbye, World!");
    }

    SECTION("Print hello world 4") {
        auto file = make_test_code_file(R"(print "Hello", ", World!")");

        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));

        global_checker.check(ast);
        local_checker.check(ast);
        codegen.generate(ast, false);
        REQUIRE(codegen.generate_main());
        auto output = codegen.eject();
        auto err = jit->add_module(std::move(output.module), std::move(output.context));
        auto printout = capture_stdout([&]() {
            auto result = jit->run_main(0, nullptr);
        });

        CHECK(printout == "Hello, World!");
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
        auto file = make_test_code_file(R"(let x = 5 print x)");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        codegen.generate(ast, false);
        REQUIRE(codegen.generate_main());
        auto output = codegen.eject();
        auto err = jit->add_module(std::move(output.module), std::move(output.context));
        auto printout = capture_stdout([&]() {
            auto result = jit->run_main(0, nullptr);
        });

        CHECK(printout == "5");
    }

    lexer.reset();
    parser.reset();
    symbol_tree->reset();
    codegen.reset();
    jit->reset();
}
