#ifndef NICO_GLOBAL_CHECKER_H
#define NICO_GLOBAL_CHECKER_H

#include <any>
#include <memory>
#include <vector>

#include "nico/frontend/utils/annotation_checker.h"
#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/frontend_context.h"
#include "nico/frontend/utils/symbol_tree.h"

namespace nico {

/**
 * @brief A global type checker.
 *
 * The global type checker checks declarations at the global level.
 * This allows functions and types to be used before they are defined, as long
 * as they are declared somewhere in the global scope.
 */
class GlobalChecker : public Stmt::Visitor {
    // The symbol tree used for type checking.
    const std::shared_ptr<SymbolTree> symbol_tree;
    // Whether or not the checker is running in REPL mode.
    const bool repl_mode = false;
    // The annotation checker used for checking type annotations.
    AnnotationChecker annotation_checker;

    GlobalChecker(
        std::shared_ptr<SymbolTree> symbol_tree, bool repl_mode = false
    )
        : symbol_tree(symbol_tree),
          repl_mode(repl_mode),
          annotation_checker(symbol_tree) {};

    std::any visit(Stmt::Expression* stmt) override;
    std::any visit(Stmt::Let* stmt) override;
    std::any visit(Stmt::Func* stmt) override;
    std::any visit(Stmt::Print* stmt) override;
    std::any visit(Stmt::Dealloc* stmt) override;
    std::any visit(Stmt::Pass* stmt) override;
    std::any visit(Stmt::Yield* stmt) override;
    std::any visit(Stmt::Continue* stmt) override;
    std::any visit(Stmt::Eof* stmt) override;

    /**
     * @brief Type checks the given context at the global level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param context The front end context containing the AST to type check.
     * @param repl_mode Whether or not the checker is running in REPL mode.
     * Defaults to false.
     */
    void run_check(std::unique_ptr<FrontendContext>& context);

public:
    static void
    check(std::unique_ptr<FrontendContext>& context, bool repl_mode = false);
};

} // namespace nico

#endif // NICO_GLOBAL_CHECKER_H
