#ifndef NICO_LOCAL_CHECKER_H
#define NICO_LOCAL_CHECKER_H

#include <any>
#include <memory>
#include <vector>

#include "nico/frontend/utils/ast_node.h"
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
class LocalChecker : public Stmt::Visitor,
                     public Expr::Visitor,
                     public Annotation::Visitor {
    // The symbol tree used for type checking.
    const std::shared_ptr<SymbolTree> symbol_tree;
    // Whether or not the checker is running in REPL mode.
    const bool repl_mode = false;

    LocalChecker(
        std::shared_ptr<SymbolTree> symbol_tree, bool repl_mode = false
    )
        : symbol_tree(symbol_tree), repl_mode(repl_mode) {};

    /**
     * @brief Checks the type of the expression and returns its type.
     *
     * @param expr The expression to check.
     * @param as_lvalue Whether the expression is being checked as an lvalue.
     * @return The type of the expression. Can be
     * nullptr.
     */
    std::shared_ptr<Type>
    expr_check(std::shared_ptr<Expr>& expr, bool as_lvalue = false);

    std::any visit(Stmt::Expression* stmt) override;
    std::any visit(Stmt::Let* stmt) override;
    std::any visit(Stmt::Print* stmt) override;
    std::any visit(Stmt::Pass* stmt) override;
    std::any visit(Stmt::Yield* stmt) override;
    std::any visit(Stmt::Continue* stmt) override;
    std::any visit(Stmt::Eof* stmt) override;

    std::any visit(Expr::Assign* expr, bool as_lvalue) override;
    std::any visit(Expr::Logical* expr, bool as_lvalue) override;
    std::any visit(Expr::Binary* expr, bool as_lvalue) override;
    std::any visit(Expr::Unary* expr, bool as_lvalue) override;
    std::any visit(Expr::Address* expr, bool as_lvalue) override;
    std::any visit(Expr::Deref* expr, bool as_lvalue) override;
    std::any visit(Expr::Cast* expr, bool as_lvalue) override;
    std::any visit(Expr::Access* expr, bool as_lvalue) override;
    std::any visit(Expr::NameRef* expr, bool as_lvalue) override;
    std::any visit(Expr::Literal* expr, bool as_lvalue) override;
    std::any visit(Expr::Tuple* expr, bool as_lvalue) override;
    std::any visit(Expr::Block* expr, bool as_lvalue) override;
    std::any visit(Expr::Conditional* expr, bool as_lvalue) override;
    std::any visit(Expr::Loop* expr, bool as_lvalue) override;

    std::any visit(Annotation::NameRef* annotation) override;
    std::any visit(Annotation::Pointer* annotation) override;
    std::any visit(Annotation::Nullptr* annotation) override;
    std::any visit(Annotation::Reference* annotation) override;
    std::any visit(Annotation::Array* annotation) override;
    std::any visit(Annotation::Object* annotation) override;
    std::any visit(Annotation::Tuple* annotation) override;

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
