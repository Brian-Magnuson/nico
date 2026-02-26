#ifndef NICO_LOCAL_CHECKER_H
#define NICO_LOCAL_CHECKER_H

#include <any>
#include <memory>
#include <vector>

#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/expression_checker.h"
#include "nico/frontend/utils/frontend_context.h"
#include "nico/frontend/utils/symbol_tree.h"
#include "nico/frontend/utils/type_node.h"

namespace nico {

/**
 * @brief A local type checker.
 *
 * The local type checker checks statements and expressions at the local level,
 * i.e., within functions, blocks, and the main script.
 */
class LocalChecker : public Stmt::Visitor {
    // The symbol tree used for type checking.
    const std::shared_ptr<SymbolTree> symbol_tree;
    // Whether or not the checker is running in REPL mode.
    const bool repl_mode = false;
    // The expression checker used for checking expressions.
    ExpressionChecker expression_checker;

    LocalChecker(
        std::shared_ptr<SymbolTree> symbol_tree, bool repl_mode = false
    )
        : symbol_tree(symbol_tree),
          repl_mode(repl_mode),
          expression_checker(symbol_tree, this, repl_mode) {};

    /**
     * @brief Helper function to determine if the current context is unsafe.
     *
     * The context is unsafe if the current scope is a local scope, and that
     * local scope is tied to a block marked as unsafe.
     *
     * @return True if the current context is unsafe, false otherwise.
     */
    bool is_in_unsafe_context();

    std::any visit(Stmt::Expression* stmt) override;
    std::any visit(Stmt::Let* stmt) override;
    std::any visit(Stmt::Static* stmt) override;
    std::any visit(Stmt::Func* stmt) override;
    std::any visit(Stmt::Print* stmt) override;
    std::any visit(Stmt::Dealloc* stmt) override;
    std::any visit(Stmt::Pass* stmt) override;
    std::any visit(Stmt::Yield* stmt) override;
    std::any visit(Stmt::Continue* stmt) override;
    std::any visit(Stmt::Namespace* stmt) override;
    std::any visit(Stmt::Extern* stmt) override;
    std::any visit(Stmt::Eof* stmt) override;

    /**
     * @brief Type checks the given context at the local level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param context The front end context containing the AST to type check.
     */
    void run_check(std::unique_ptr<FrontendContext>& context);

public:
    /**
     * @brief Type checks the given context at the local level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param context The front end context containing the AST to type check.
     * @param repl_mode Whether or not the checker is running in REPL mode.
     * Defaults to false.
     */
    static void
    check(std::unique_ptr<FrontendContext>& context, bool repl_mode = false);
};

} // namespace nico

#endif // NICO_LOCAL_CHECKER_H
