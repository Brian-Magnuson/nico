#ifndef NICO_FRONTEND_CONTEXT_H
#define NICO_FRONTEND_CONTEXT_H

#include <memory>
#include <utility>
#include <vector>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/symbol_tree.h"
#include "nico/shared/ir_module_context.h"
#include "nico/shared/token.h"

/**
 * @brief A front end context, which contains the current status, AST, and
 * symbol tree.
 *
 * This class is move only. It cannot be copied.
 *
 * It is recommended to use a `std::unique_ptr` for this class.
 */
class FrontendContext {
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
    // The LLVM module and context used for code generation.
    IrModuleContext mod_ctx;

    FrontendContext() { reset(); }

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
        mod_ctx.reset();
    }
};

#endif // NICO_FRONTEND_CONTEXT_H
