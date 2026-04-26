#include <memory>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "nico/frontend/components/lexer.h"
#include "nico/frontend/components/parser.h"
#include "nico/frontend/utils/ast_node.h"
#include "nico/shared/code_file.h"
#include "nico/shared/logger.h"
#include "nico/shared/token.h"

#include "ast_printer.h"
#include "test_utils.h"

using nico::Err;

/**
 * @brief Run a parser statement test with the given source code and expected
 * AST strings.
 *
 * @param src_code The source code to test.
 * @param expected The expected AST strings.
 */
void run_parser_stmt_test(
    std::string_view src_code, const std::vector<std::string>& expected
) {
    auto context = std::make_unique<nico::FrontendContext>();
    auto file = nico::make_test_code_file(src_code);
    nico::Lexer::scan(context, file);
    nico::Parser::parse(context);
    nico::AstPrinter printer;
    CHECK(printer.stmts_to_strings(context->stmts) == expected);

    context->initialize();
    nico::Logger::inst().reset();
}

/**
 * @brief Run a parser statement error test with the given source code and
 * expected error.
 *
 * Erroneous input can potentially produce multiple errors.
 * Only the first error is checked by this test.
 *
 * @param src_code The source code to test.
 * @param expected_error The expected error.
 * @param print_errors Whether to print errors. Defaults to false.
 */
void run_parser_stmt_error_test(
    std::string_view src_code, Err expected_error, bool print_errors = false
) {
    nico::Logger::inst().set_printing_enabled(print_errors);

    auto context = std::make_unique<nico::FrontendContext>();
    auto file = nico::make_test_code_file(src_code);
    nico::Lexer::scan(context, file);
    nico::Parser::parse(context);

    auto& errors = nico::Logger::inst().get_errors();
    REQUIRE(errors.size() >= 1);
    CHECK(errors.at(0) == expected_error);

    context->initialize();
    nico::Logger::inst().reset();
}

// MARK: Parser stmt tests

TEST_CASE("Parser let statements", "[parser]") {
    SECTION("Let statements 1") {
        run_parser_stmt_test(
            "let a = 1",
            {"(stmt:let a (lit i32 1))", "(stmt:eof)"}
        );
    }

    SECTION("Let statements 2") {
        run_parser_stmt_test(
            "let var a = 1",
            {"(stmt:let var a (lit i32 1))", "(stmt:eof)"}
        );
    }

    SECTION("Let statements 3") {
        run_parser_stmt_test(
            "let a: i32 = 1",
            {"(stmt:let a i32 (lit i32 1))", "(stmt:eof)"}
        );
    }

    SECTION("Let statements 4") {
        run_parser_stmt_test(
            "let var a: i32 let var b: f64",
            {"(stmt:let var a i32)", "(stmt:let var b f64)", "(stmt:eof)"}
        );
    }

    SECTION("Let statements 5") {
        run_parser_stmt_test(
            "let var a: Vector2D",
            {"(stmt:let var a Vector2D)", "(stmt:eof)"}
        );
    }

    SECTION("Let statements 6") {
        run_parser_stmt_test(
            "let var a: i32 let b = 2",
            {"(stmt:let var a i32)", "(stmt:let b (lit i32 2))", "(stmt:eof)"}
        );
    }

    SECTION("Let missing identifier 1") {
        run_parser_stmt_error_test("let", Err::NotAnIdentifier);
    }

    SECTION("Let missing identifier 2") {
        run_parser_stmt_error_test("let var", Err::NotAnIdentifier);
    }

    SECTION("Let without type or value") {
        run_parser_stmt_error_test("let a", Err::VariableWithoutTypeOrValue);
    }

    SECTION("Let with colon colon in identifier") {
        run_parser_stmt_error_test(
            "let a::b = 1",
            Err::DeclarationIdentWithColonColon
        );
    }
}

TEST_CASE("Parser let stmt type annotations", "[parser]") {
    SECTION("Let statements pointers 1") {
        run_parser_stmt_test(
            "let var a: @i32 = 10",
            {"(stmt:let var a @i32 (lit i32 10))", "(stmt:eof)"}
        );
    }

    SECTION("Let statements pointers 2") {
        run_parser_stmt_test(
            "let var ptr: var@f64",
            {"(stmt:let var ptr var@f64)", "(stmt:eof)"}
        );
    }

    SECTION("Let statements pointers 3") {
        run_parser_stmt_test(
            "let data: nullptr = nullptr",
            {"(stmt:let data nullptr (lit nullptr))", "(stmt:eof)"}
        );
    }

    SECTION("Let statements typeof 1") {
        run_parser_stmt_test(
            "let var x: i32 let var size: typeof(x)",
            {"(stmt:let var x i32)",
             "(stmt:let var size typeof(<expr@1:37>))",
             "(stmt:eof)"}
        );
    }

    SECTION("Let statements typeof 2") {
        run_parser_stmt_test(
            "let var x: i32 let var len: typeof(x + 1)",
            {"(stmt:let var x i32)",
             "(stmt:let var len typeof(<expr@1:38>))",
             "(stmt:eof)"}
        );
    }

    SECTION("Let statements arrays 1") {
        run_parser_stmt_test(
            "let var arr: [i32; 10]",
            {"(stmt:let var arr [i32; 10])", "(stmt:eof)"}
        );
    }

    SECTION("Let statements arrays 2") {
        run_parser_stmt_test(
            "let var matrix: [[f64; 5]; 10]",
            {"(stmt:let var matrix [[f64; 5]; 10])", "(stmt:eof)"}
        );
    }

    SECTION("Let statements arrays 3") {
        run_parser_stmt_test(
            "let var buffer: [u8; ?]",
            {"(stmt:let var buffer [u8; ?])", "(stmt:eof)"}
        );
    }

    SECTION("Let statements arrays 4") {
        run_parser_stmt_test(
            "let var data: var@[i32; 20]",
            {"(stmt:let var data var@[i32; 20])", "(stmt:eof)"}
        );
    }

    SECTION("Let statements arrays 5") {
        run_parser_stmt_test(
            "let var empty: [f64; 0]",
            {"(stmt:let var empty [f64; 0])", "(stmt:eof)"}
        );
    }

    SECTION("Let statements void 1") {
        run_parser_stmt_test(
            "let var v: void",
            {"(stmt:let var v void)", "(stmt:eof)"}
        );
    }

    SECTION("Let statements void 2") {
        run_parser_stmt_test(
            "let var v: void = void",
            {"(stmt:let var v void (lit void))", "(stmt:eof)"}
        );
    }

    SECTION("Let improper type") {
        run_parser_stmt_error_test("let a: 1 = 1", Err::NotAType);
    }

    SECTION("Let without ident") {
        run_parser_stmt_error_test("let : i32 = 1", Err::NotAnIdentifier);
    }

    SECTION("Unexpected var in annotation") {
        run_parser_stmt_error_test(
            "let a: var i32 = 1",
            Err::UnexpectedVarInAnnotation
        );
    }

    SECTION("Typeof missing opening parenthesis") {
        run_parser_stmt_error_test(
            "let a: typeof x = 1",
            Err::TypeofWithoutOpeningParen
        );
    }

    SECTION("Comma in typeof annotation") {
        run_parser_stmt_error_test(
            "let a: typeof(x,) = 1",
            Err::UnexpectedToken
        );
    }
}

TEST_CASE("Parser static statements", "[parser]") {
    SECTION("Static statement 1") {
        run_parser_stmt_test(
            "static var s1: i32",
            {"(stmt:static var s1 i32)", "(stmt:eof)"}
        );
    }

    SECTION("Static statement 2") {
        run_parser_stmt_test(
            "static var s2: @f64",
            {"(stmt:static var s2 @f64)", "(stmt:eof)"}
        );
    }

    SECTION("Static without compile-time constant value") {
        run_parser_stmt_error_test(
            "static s: i32 = x",
            Err::NonCompileTimeExpr
        );
    }

    SECTION("Static with compile-time initializer 1") {
        run_parser_stmt_test(
            "static s: i32 = 10",
            {"(stmt:static s i32 (lit i32 10))", "(stmt:eof)"}
        );
    }

    SECTION("Static with compile-time initializer 2") {
        run_parser_stmt_test(
            "static s = [1, 2]",
            {"(stmt:static s (array (lit i32 1) (lit i32 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Static with compile-time initializer 3") {
        run_parser_stmt_test(
            "static s = ()",
            {"(stmt:static s (tuple))", "(stmt:eof)"}
        );
    }

    SECTION("Static with compile-time initializer 4") {
        run_parser_stmt_test(
            "static s = (1, 2)",
            {"(stmt:static s (tuple (lit i32 1) (lit i32 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Static without type or value") {
        run_parser_stmt_error_test("static s", Err::VariableWithoutTypeOrValue);
    }
}

TEST_CASE("Parser function statements", "[parser]") {
    SECTION("Func statement 1") {
        run_parser_stmt_test(
            "func f1() {}",
            {"(stmt:func f1 => (block))", "(stmt:eof)"}
        );
    }

    SECTION("Func statement 2") {
        run_parser_stmt_test(
            "func f2():\n    pass",
            {"(stmt:func f2 => (block (stmt:pass)))", "(stmt:eof)"}
        );
    }

    SECTION("Func statement 3") {
        run_parser_stmt_test(
            "func f3() -> i32 => 10",
            {"(stmt:func f3 i32 => (block (stmt:yield => (lit i32 "
             "10))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Func statement 4") {
        run_parser_stmt_test(
            "func f4(x: i32) -> i32 => x + 1",
            {"(stmt:func f4 i32 (x i32) => (block (stmt:yield => (binary + "
             "(nameref x) (lit i32 1)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Func statement 5") {
        run_parser_stmt_test(
            "func f5(var y: f64) { y += 1.0 }",
            {"(stmt:func f5 (var y f64) => (block (expr (assign (nameref "
             "y) "
             "(binary + (nameref y) (lit f64 1.000000))))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Func statement 6") {
        run_parser_stmt_test(
            "func f6(a: i32 = 0) -> i32 => a * 2",
            {"(stmt:func f6 i32 (a i32 (lit i32 0)) => (block (stmt:yield "
             "=> "
             "(binary * (nameref a) (lit i32 2)))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Func statement multiple parameters") {
        run_parser_stmt_test(
            "func f(x: i32, y: f64) -> i32 => x",
            {"(stmt:func f i32 (x i32) (y f64) => (block (stmt:yield => "
             "(nameref x))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Func statement 7") {
        run_parser_stmt_test(
            "func f7() { pass pass pass }",
            {"(stmt:func f7 => (block (stmt:pass) (stmt:pass) "
             "(stmt:pass)))",
             "(stmt:eof)"}
        );
    }

    SECTION("Func header only 1") {
        run_parser_stmt_test(
            "func f1()",
            {"(stmt:func f1 no body)", "(stmt:eof)"}
        );
    }

    SECTION("Func header only 2") {
        run_parser_stmt_test(
            "func f2() -> i32",
            {"(stmt:func f2 i32 no body)", "(stmt:eof)"}
        );
    }

    SECTION("Func header only 3") {
        run_parser_stmt_test(
            "func f3(x: i32)",
            {"(stmt:func f3 (x i32) no body)", "(stmt:eof)"}
        );
    }

    SECTION("Multiple function headers") {
        run_parser_stmt_test(
            "func f4() func f5() func f6()",
            {"(stmt:func f4 no body)",
             "(stmt:func f5 no body)",
             "(stmt:func f6 no body)",
             "(stmt:eof)"}
        );
    }

    SECTION("Function header surrounded by stmts") {
        run_parser_stmt_test(
            "pass func f() pass",
            {"(stmt:pass)",
             "(stmt:func f no body)",
             "(stmt:pass)",
             "(stmt:eof)"}
        );
    }

    SECTION("Func statement with variadic parameter") {
        run_parser_stmt_test(
            "func f(x: i32, ...) => x",
            {"(stmt:func f (x i32) (...) => (block (stmt:yield => (nameref "
             "x))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Func statement with only variadic parameter") {
        run_parser_stmt_test(
            "func f(...) => 42",
            {"(stmt:func f (...) => (block (stmt:yield => (lit i32 42))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Func statement with variadic parameter and no body") {
        run_parser_stmt_test(
            "func f(x: i32, ...)",
            {"(stmt:func f (x i32) (...) no body)", "(stmt:eof)"}
        );
    }

    SECTION("Variadic parameter not last") {
        run_parser_stmt_error_test(
            "func f(..., x: i32) {}",
            Err::UnexpectedTokenAfterVariadicParam
        );
    }

    SECTION("Func missing identifier") {
        run_parser_stmt_error_test("func {}", Err::NotAnIdentifier);
    }

    SECTION("Func missing opening parenthesis") {
        run_parser_stmt_error_test("func f1 {}", Err::FuncWithoutOpeningParen);
    }

    SECTION("Func missing annotation after arrow") {
        run_parser_stmt_error_test("func f() ->", Err::NotAType);
    }

    SECTION("Func missing parameter type") {
        run_parser_stmt_error_test("func f(x) {}", Err::NotAType);
    }

    SECTION("Func missing parameter identifier") {
        run_parser_stmt_error_test("func f(: i32) {}", Err::NotAnIdentifier);
    }

    SECTION("Func parameter missing expr after equals") {
        run_parser_stmt_error_test(
            "func f(x: i32 = ) {}",
            Err::NotAnExpression
        );
    }

    SECTION("Semicolon in func declaration parameters") {
        run_parser_stmt_error_test(
            "func f(a: i32; b: i32) {}",
            Err::UnexpectedToken
        );
    }

    SECTION("Func with colon colon in identifier") {
        run_parser_stmt_error_test(
            "func f::g() {}",
            Err::DeclarationIdentWithColonColon
        );
    }
}

TEST_CASE("Parser namespace statements", "[parser]") {
    SECTION("Empty namespace braced") {
        run_parser_stmt_test(
            "namespace ns1 {}",
            {"(stmt:namespace ns1 { })", "(stmt:eof)"}
        );
    }

    SECTION("Braced namespace with statements 1") {
        run_parser_stmt_test(
            "namespace ns1 { pass pass } pass",
            {"(stmt:namespace ns1 { (stmt:pass) (stmt:pass) })",
             "(stmt:pass)",
             "(stmt:eof)"}
        );
    }

    SECTION("Nested braced namespaces") {
        run_parser_stmt_test(
            "namespace outer {\n"
            "    namespace inner1 {\n"
            "        pass\n"
            "    }\n"
            "    namespace inner2 {\n"
            "        pass\n"
            "    }\n"
            "    pass\n"
            "}\n",
            {"(stmt:namespace outer { "
             "(stmt:namespace inner1 { (stmt:pass) }) "
             "(stmt:namespace inner2 { (stmt:pass) }) "
             "(stmt:pass) })",
             "(stmt:eof)"}
        );
    }

    SECTION("Empty namespace indented") {
        run_parser_stmt_test(
            "namespace ns2:\n  // empty namespace\n",
            {"(stmt:namespace ns2 { })", "(stmt:eof)"}
        );
    }

    SECTION("Indented namespace with statements 1") {
        run_parser_stmt_test(
            "namespace ns2:\n    pass\n    pass\npass\n",
            {"(stmt:namespace ns2 { (stmt:pass) (stmt:pass) })",
             "(stmt:pass)",
             "(stmt:eof)"}
        );
    }

    SECTION("Nested indented namespaces") {
        run_parser_stmt_test(
            "namespace outer:\n"
            "    namespace inner1:\n"
            "        pass\n"
            "    namespace inner2:\n"
            "        pass\n"
            "    pass\n",
            {"(stmt:namespace outer { "
             "(stmt:namespace inner1 { (stmt:pass) }) "
             "(stmt:namespace inner2 { (stmt:pass) }) "
             "(stmt:pass) })",
             "(stmt:eof)"}
        );
    }

    SECTION("Namespace with static statements") {
        run_parser_stmt_test(
            "namespace ns3 {\n"
            "    static var t: f64\n"
            "}\n",
            {"(stmt:namespace ns3 { "
             "(stmt:static var t f64) })",
             "(stmt:eof)"}
        );
    }

    SECTION("Namespace without block") {
        run_parser_stmt_error_test("namespace ns3", Err::NamespaceWithoutBlock);
    }

    SECTION("Namespace with non-decl-allowed statement") {
        run_parser_stmt_error_test(
            "namespace ns4 { let x = 10 }",
            Err::NonDeclAllowedStmt
        );
    }

    SECTION("Namespace with colon colon in identifier") {
        run_parser_stmt_error_test(
            "namespace ns::inner {}",
            Err::DeclarationIdentWithColonColon
        );
    }
}

TEST_CASE("Parser extern block statements", "[parser]") {
    SECTION("Valid extern block 1") {
        run_parser_stmt_test(
            "extern ex1 {\n"
            "    func add(a: i32, b: i32) -> i32\n"
            "}\n",
            {"(stmt:externblock \"C\" ex1 { (stmt:func add i32 (a i32) (b i32) "
             "no "
             "body) "
             "})",
             "(stmt:eof)"}
        );
    }

    SECTION("Valid extern block 2") {
        run_parser_stmt_test(
            "extern ex2 {\n"
            "    static var x: f64\n"
            "}\n",
            {"(stmt:externblock \"C\" ex2 { (stmt:static var x f64) })",
             "(stmt:eof)"}
        );
    }

    SECTION("Extern block ABI specified") {
        run_parser_stmt_test(
            "extern \"C\" ex3 {\n"
            "    func foo()\n"
            "}\n",
            {"(stmt:externblock \"C\" ex3 { (stmt:func foo no body) })",
             "(stmt:eof)"}
        );
    }

    SECTION("Extern block multiple statements") {
        run_parser_stmt_test(
            "extern ex4 {\n"
            "    func foo()\n"
            "    static var x: i32\n"
            "}\n",
            {"(stmt:externblock \"C\" ex4 { (stmt:func foo no body) "
             "(stmt:static var x i32) })",
             "(stmt:eof)"}
        );
    }

    SECTION("Extern block indented form") {
        run_parser_stmt_test(
            "extern ex5:\n"
            "    func foo()\n"
            "    static var x: i32\n",
            {"(stmt:externblock \"C\" ex5 { (stmt:func foo no body) "
             "(stmt:static var x i32) })",
             "(stmt:eof)"}
        );
    }

    SECTION("Extern block missing block") {
        run_parser_stmt_error_test("extern ex4", Err::ExternBlockWithoutBlock);
    }

    SECTION("Extern block unrecognized ABI") {
        run_parser_stmt_error_test(
            "extern \"UnknownABI\" ex5 {\n"
            "    func bar()\n"
            "}\n",
            Err::ExternBlockUnrecognizedABI
        );
    }

    SECTION("Extern block with non-var/non-func statement") {
        run_parser_stmt_error_test(
            "extern \"C\" ex3 {\n"
            "    let x = 10\n"
            "}\n",
            Err::ExternBlockStmtNotVarOrFunc
        );
    }

    SECTION("Extern block missing identifier") {
        run_parser_stmt_error_test(
            "extern {\n"
            "    func foo()\n"
            "}\n",
            Err::UnexpectedTokenAfterExtern
        );
    }

    SECTION("Extern block with colon colon in identifier") {
        run_parser_stmt_error_test(
            "extern ex::6 {\n"
            "    func foo()\n"
            "}\n",
            Err::DeclarationIdentWithColonColon
        );
    }

    SECTION("Extern block colon instead of indent") {
        run_parser_stmt_error_test(
            R"(
            namespace ns1 {
                extern ex6:
                    func foo()
            }
            )",
            Err::ColonInsteadOfIndent
        );
    }
}

TEST_CASE("Parser tuple annotations", "[parser]") {
    SECTION("Tuple annotation 1") {
        run_parser_stmt_test(
            "let var a: (i32)",
            {"(stmt:let var a (i32))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple annotation 2") {
        run_parser_stmt_test(
            "let var a: (i32, f64, String)",
            {"(stmt:let var a (i32, f64, String))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple annotation 3") {
        run_parser_stmt_test(
            "let var a: (i32,)",
            {"(stmt:let var a (i32))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple annotation 4") {
        run_parser_stmt_test(
            "let var a: ((i32, f64), String)",
            {"(stmt:let var a ((i32, f64), String))", "(stmt:eof)"}
        );
    }

    SECTION("Tuple annotation 5") {
        run_parser_stmt_test(
            "let var a: ()",
            {"(stmt:let var a ())", "(stmt:eof)"}
        );
    }

    SECTION("Semicolon in tuple annotation") {
        run_parser_stmt_error_test(
            "let var a: (i32; f64)",
            Err::UnexpectedToken
        );
    }
}

TEST_CASE("Parser print statements", "[parser]") {
    SECTION("Print statements 1") {
        run_parser_stmt_test(
            "printout 1",
            {"(stmt:print (lit i32 1))", "(stmt:eof)"}
        );
    }

    SECTION("Print statements 2") {
        run_parser_stmt_test(
            "printout 1, 2",
            {"(stmt:print (lit i32 1) (lit i32 2))", "(stmt:eof)"}
        );
    }

    SECTION("Print statements 3") {
        run_parser_stmt_test(
            "printout 1, 2, 3",
            {"(stmt:print (lit i32 1) (lit i32 2) (lit i32 3))", "(stmt:eof)"}
        );
    }

    SECTION("Print statements 4") {
        run_parser_stmt_test(
            "printout 1 / 2",
            {"(stmt:print (binary / (lit i32 1) (lit i32 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Print statements 5") {
        run_parser_stmt_test(
            "printout \"Hello, World!\" printout \"Goodbye, World!\"",
            {"(stmt:print (lit \"Hello, World!\"))",
             "(stmt:print (lit \"Goodbye, World!\"))",
             "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser statement separation", "[parser]") {
    SECTION("Unseparated binary statement") {
        run_parser_stmt_test(
            "1 - 2",
            {"(expr (binary - (lit i32 1) (lit i32 2)))", "(stmt:eof)"}
        );
    }

    SECTION("Separated unary statements") {
        run_parser_stmt_test(
            "1; - 2",
            {"(expr (lit i32 1))", "(expr (lit i32 -2))", "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser non-declaring statements", "[parser]") {
    SECTION("Yield statement") {
        run_parser_stmt_test(
            "yield 42",
            {"(stmt:yield yield (lit i32 42))", "(stmt:eof)"}
        );
    }

    SECTION("Continue statement") {
        run_parser_stmt_test("continue", {"(stmt:continue)", "(stmt:eof)"});
    }

    SECTION("Break statement") {
        run_parser_stmt_test(
            "break 10",
            {"(stmt:yield break (lit i32 10))", "(stmt:eof)"}
        );
    }

    SECTION("Yield missing expression") {
        run_parser_stmt_error_test("yield", Err::NotAnExpression);
    }

    SECTION("Break missing expression") {
        run_parser_stmt_error_test("break", Err::NotAnExpression);
    }

    SECTION("Dealloc missing expression") {
        run_parser_stmt_error_test("dealloc", Err::NotAnExpression);
    }
}

TEST_CASE("Parser dealloc statement", "[parser]") {
    SECTION("Dealloc stmt 1") {
        run_parser_stmt_test(
            "dealloc ptr",
            {"(stmt:dealloc (nameref ptr))", "(stmt:eof)"}
        );
    }

    SECTION("Dealloc stmt 2") {
        run_parser_stmt_test(
            "dealloc @ptr",
            {"(stmt:dealloc (address @ (nameref ptr)))", "(stmt:eof)"}
        );
    }

    SECTION("Dealloc stmt 3") {
        run_parser_stmt_test(
            "dealloc alloc i32",
            {"(stmt:dealloc (alloc i32))", "(stmt:eof)"}
        );
    }

    SECTION("Dealloc stmt 4") {
        run_parser_stmt_test(
            "dealloc alloc [i32; 3]",
            {"(stmt:dealloc (alloc [i32; 3]))", "(stmt:eof)"}
        );
    }

    SECTION("Dealloc stmt 5") {
        run_parser_stmt_test(
            "dealloc nullptr",
            {"(stmt:dealloc (lit nullptr))", "(stmt:eof)"}
        );
    }

    SECTION("Dealloc stmt 6") {
        run_parser_stmt_test(
            "dealloc f()",
            {"(stmt:dealloc (call (nameref f)))", "(stmt:eof)"}
        );
    }

    SECTION("Dealloc stmt 7") {
        run_parser_stmt_test(
            "dealloc ptr as @[i32; 10]",
            {"(stmt:dealloc (cast (nameref ptr) as @[i32; 10]))", "(stmt:eof)"}
        );
    }
}

TEST_CASE("Parser modifiers linkage and symbol", "[parser]") {
    SECTION("External linkage static variable") {
        run_parser_stmt_test(
            R"(
            #[linkage("external")]
            static var x: i32 = 5
            )",
            {"(stmt:static [linkage:external] var x i32 (lit i32 5))",
             "(stmt:eof)"}
        );
    }

    SECTION("External linkage function") {
        run_parser_stmt_test(
            R"(
            #[linkage("external")]
            func foo() -> i32 => 42
            )",
            {"(stmt:func [linkage:external] foo i32 => (block (stmt:yield => "
             "(lit i32 42))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Internal linkage static variable") {
        run_parser_stmt_test(
            R"(
            #[linkage("internal")]
            static var y: f64 = 3.14
            )",
            {"(stmt:static [linkage:internal] var y f64 (lit f64 3.140000))",
             "(stmt:eof)"}
        );
    }

    SECTION("Internal linkage function") {
        run_parser_stmt_test(
            R"(
            #[linkage("internal")]
            func bar() -> i32 => 7
            )",
            {"(stmt:func [linkage:internal] bar i32 => (block (stmt:yield => "
             "(lit i32 7))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Invalid linkage value") {
        run_parser_stmt_error_test(
            R"(
            #[linkage("invalid")]
            static var z: i32 = 0
            )",
            Err::ModifierInvalidArguments
        );
    }

    SECTION("Linkage modifier on wrong statement type") {
        run_parser_stmt_error_test(
            R"(
            #[linkage("external")]
            let a = 10
            )",
            Err::InvalidModifierForStatement
        );
    }

    SECTION("Symbol modifier on static variable") {
        run_parser_stmt_test(
            R"(
            #[symbol("my_symbol")]
            static var s: i32 = 1
            )",
            {"(stmt:static [symbol:\"my_symbol\"] var s i32 (lit i32 1))",
             "(stmt:eof)"}
        );
    }

    SECTION("Symbol modifier on function") {
        run_parser_stmt_test(
            R"(
            #[symbol("my_func")]
            func my_func() -> void => void
            )",
            {"(stmt:func [symbol:\"my_func\"] my_func void => (block "
             "(stmt:yield => "
             "(lit void))))",
             "(stmt:eof)"}
        );
    }

    SECTION("Symbol modifier with special characters") {
        run_parser_stmt_test(
            R"(
            #[symbol("sym!@#$%^&*(")]
            static var t: i32 = 0
            )",
            {"(stmt:static [symbol:\"sym!@#$%^&*(\"] var t i32 (lit i32 0))",
             "(stmt:eof)"}
        );
    }

    SECTION("Symbol modifier with non-string argument") {
        run_parser_stmt_error_test(
            R"(
            #[symbol(123)]
            static var t: i32 = 0
            )",
            Err::ModifierInvalidArguments
        );
    }

    SECTION("Symbol modifier on wrong statement type") {
        run_parser_stmt_error_test(
            R"(
            #[symbol("not_a_var_or_func")]
            let x = 10
            )",
            Err::InvalidModifierForStatement
        );
    }

    SECTION("Combined modifiers") {
        run_parser_stmt_test(
            R"(
            #[linkage("external"), symbol("combined_symbol")]
            static var u: f64 = 2.718
            )",
            {"(stmt:static [linkage:external] [symbol:\"combined_symbol\"] var "
             "u f64 (lit f64 2.718000))",
             "(stmt:eof)"}
        );
    }

    SECTION("Modifier already applied") {
        run_parser_stmt_error_test(
            R"(
            #[linkage("external"), linkage("internal")]
            static var v: i32 = 0
            )",
            Err::ModifierAlreadyApplied
        );
    }
}
