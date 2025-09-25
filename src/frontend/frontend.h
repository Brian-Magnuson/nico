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
#include "../lexer/token.h"
#include "../nodes/ast_node.h"
#include "../parser/parser.h"
#include "../parser/symbol_tree.h"

/**
 * @brief The compiler front end, which includes the lexer, parser, type
 * checkers, and code generator.
 */
class Frontend {
public:
    /**
     * @brief The status of the front end.
     */
    enum class Status {
        // The front end is ready to accept input.
        OK,
        // The front end could not complete processing, but can try again after
        // receiving more input.
        Pause,
        // The front end encountered an unrecoverable error and cannot continue.
        Error
    };

    /**
     * @brief A front end context, which contains the current status, AST, and
     * symbol tree.
     */
    struct Context {
        // The current status of the front end.
        Status status = Status::OK;
        // The AST containing all statements processed so far.
        std::vector<std::shared_ptr<Stmt>> stmts;
        // The number of statements at the beginning of `stmts` that have been
        // type-checked.
        size_t stmts_checked = 0;
        // The symbol tree used for type checking.
        std::shared_ptr<SymbolTree> symbol_tree;

        // The generated LLVM module.
        std::unique_ptr<llvm::Module> ir_module = nullptr;
        // The LLVM context used to generate the module.
        std::unique_ptr<llvm::LLVMContext> context = nullptr;

        Context() { reset(); }

        /**
         * @brief Resets the context to its initial state.
         *
         * Useful for resetting the front end.
         */
        void reset() {
            status = Status::OK;
            stmts.clear();
            stmts_checked = 0;
            symbol_tree = std::make_shared<SymbolTree>();
            ir_module = nullptr;
            context = nullptr;
        }
    };

private:
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
    std::unique_ptr<Context> context;

public:
    Frontend()
        : context(std::make_unique<Context>()) {}

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
    std::unique_ptr<Context>&
    compile(const std::shared_ptr<CodeFile>& file, bool repl_mode = false);

    /**
     * @brief Resets the front end context to its initial state.
     *
     * This will clear the AST and symbol tree, eliminating all statements and
     * symbols.
     * Useful for REPLs to clear the current state.
     */
    void reset() { context->reset(); }
};

#endif // NICO_FRONTEND_H
