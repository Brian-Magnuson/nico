#ifndef NICO_FRONTEND_H
#define NICO_FRONTEND_H

#include <memory>
#include <utility>
#include <vector>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "../checker/global_checker.h"
#include "../checker/local_checker.h"
#include "../codegen/code_generator.h"
#include "../common/code_file.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "context.h"

/**
 * @brief The compiler front end, which includes the lexer, parser, type
 * checkers, and code generator.
 */
class Frontend {

    // The lexer. Scans source code into tokens.
    Lexer lexer;
    // The parser. Parses tokens into an AST.
    Parser parser;
    // The global type checker. Checks the AST at the global level.
    GlobalChecker global_checker;
    // The local type checker. Checks the AST at the local level.
    LocalChecker local_checker;
    // The code generator. Generates LLVM IR from the AST.
    CodeGenerator codegen;

    // The unique front end context.
    std::unique_ptr<FrontendContext> context;

public:
    Frontend()
        : context(std::make_unique<FrontendContext>()) {}

    /**
     * @brief Compiles the given code file.
     *
     * This function will scan, parse, type check, and generate LLVM IR for the
     * provided code file. The front end context will be updated accordingly.
     *
     * If compilation is successful, the context status will be `OK`, and the
     * context will contain the generated LLVM module and context.
     *
     * @param file The code file to compile.
     * @return A unique pointer reference to the front end context.
     */
    std::unique_ptr<FrontendContext>&
    compile(const std::shared_ptr<CodeFile>& file, bool repl_mode = false);

    /**
     * @brief Sets whether the code generator should use panic recovery.
     *
     * If setting panic recoverable, make sure to call this function before any
     * code is generated.
     *
     * Normally, panics cause the program to terminate. But this makes the
     * program difficult to test. So we provide a means to make a panic
     * "recoverable".
     *
     * When panic recoverable is true, the program will generate instructions to
     * call setjmp and longjmp. These behave similar to throw and catch in C++.
     *
     * @param value True to enable panic recovery, false to disable it.
     */
    void set_panic_recoverable(bool value) {
        codegen.set_panic_recoverable(value);
    }

    /**
     * @brief Sets whether the code generator should print the generated IR just
     * before verification.
     *
     * This is useful for debugging/testing purposes.
     * Make sure to call this function before any code is generated.
     *
     * @param value True to enable IR printing, false to disable it.
     */
    void set_ir_printing_enabled(bool value) {
        codegen.set_ir_printing_enabled(value);
    }

    /**
     * @brief Resets the front end to its initial state.
     *
     * This will clear the AST and symbol tree, eliminating all statements and
     * symbols.
     * Useful for REPLs to clear the current state.
     */
    void reset() {
        lexer.reset();
        parser.reset();
        codegen.reset();
        context->reset();
    }
};

#endif // NICO_FRONTEND_H
