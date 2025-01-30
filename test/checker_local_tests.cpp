#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <utility>
#include <vector>

#include "../src/checker/global_checker.h"
#include "../src/checker/local_checker.h"
#include "../src/compiler/code_file.h"
#include "../src/debug/test_utils.h"
#include "../src/lexer/lexer.h"
#include "../src/lexer/token.h"
#include "../src/logger/logger.h"
#include "../src/parser/parser.h"
#include "../src/parser/stmt.h"

TEST_CASE("Local variable declarations", "[checker]") {
    Lexer lexer;
    Parser parser;
    GlobalChecker global_checker;
    LocalChecker local_checker;
    Logger::inst().set_printing_enabled(false);

    SECTION("Valid local variable declarations") {
        auto file = make_test_code_file("let a = 1");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);

        CHECK(Logger::inst().get_errors().empty());
    }

    SECTION("Let type mismatch") {
        // Logger::inst().set_printing_enabled(true);
        auto file = make_test_code_file("let a: i32 = true");
        auto tokens = lexer.scan(file);
        auto ast = parser.parse(std::move(tokens));
        global_checker.check(ast);
        local_checker.check(ast);
        auto& errors = Logger::inst().get_errors();

        REQUIRE(errors.size() >= 1);
        CHECK(errors.at(0) == Err::LetTypeMismatch);
    }

    lexer.reset();
    parser.reset();
    Logger::inst().reset();
}
