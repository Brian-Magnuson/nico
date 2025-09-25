#ifndef NICO_CONTEXT_H
#define NICO_CONTEXT_H

#include <memory>
#include <utility>
#include <vector>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "../lexer/token.h"
#include "../nodes/ast_node.h"
#include "../parser/symbol_tree.h"

/**
 * @brief A front end context, which contains the current status, AST, and
 * symbol tree.
 *
 * This struct is neither copyable nor movable. It can only be borrowed once
 * created.
 */
struct Context {
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

    // The current status of the front end.
    Status status = Status::OK;
    // The tokens scanned from the last input.
    std::vector<std::shared_ptr<Token>> scanned_tokens;
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
    std::unique_ptr<llvm::LLVMContext> llvm_context = nullptr;

    Context() { reset(); }

    // Move constructors and move assignment are disabled due to LLVM context
    // and module complexities.
    Context(Context&&) = delete;
    Context(const Context&) = delete;
    Context& operator=(Context&&) = delete;
    Context& operator=(const Context&) = delete;
    // You cannot move this struct; you can only borrow it from its owner.

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
        llvm_context = nullptr;
    }
};

#endif // NICO_CONTEXT_H
