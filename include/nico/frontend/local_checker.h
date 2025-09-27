#ifndef NICO_LOCAL_CHECKER_H
#define NICO_LOCAL_CHECKER_H

#include <any>
#include <memory>
#include <vector>

#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/frontend_context.h"
#include "nico/frontend/utils/symbol_tree.h"
#include "nico/frontend/utils/type_node.h"

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
    std::shared_ptr<SymbolTree> symbol_tree;

    /**
     * @brief Checks the type of the expression, dereferencing any references.
     *
     * If the type of the expression is a reference type, this function will
     * modify the expression to be a dereference expression, and return the base
     * type of the reference. Otherwise, it will leave the expression unchanged
     * and return its type, even if it is nullptr.
     *
     * @param expr The expression to check. May be modified by this function.
     * @param as_lvalue Whether the expression is being checked as an lvalue.
     * @return The type of the expression, with references dereferenced. Can be
     * nullptr.
     */
    std::shared_ptr<Type>
    expr_check(std::shared_ptr<Expr>& expr, bool as_lvalue = false);

    std::any visit(Stmt::Expression* stmt) override;
    std::any visit(Stmt::Let* stmt) override;
    std::any visit(Stmt::Print* stmt) override;
    std::any visit(Stmt::Pass* stmt) override;
    std::any visit(Stmt::Yield* stmt) override;
    std::any visit(Stmt::Eof* stmt) override;

    std::any visit(Expr::Assign* expr, bool as_lvalue) override;
    std::any visit(Expr::Logical* expr, bool as_lvalue) override;
    std::any visit(Expr::Binary* expr, bool as_lvalue) override;
    std::any visit(Expr::Unary* expr, bool as_lvalue) override;
    std::any visit(Expr::Deref* expr, bool as_lvalue) override;
    std::any visit(Expr::Access* expr, bool as_lvalue) override;
    std::any visit(Expr::NameRef* expr, bool as_lvalue) override;
    std::any visit(Expr::Literal* expr, bool as_lvalue) override;
    std::any visit(Expr::Tuple* expr, bool as_lvalue) override;
    std::any visit(Expr::Block* expr, bool as_lvalue) override;
    std::any visit(Expr::Conditional* expr, bool as_lvalue) override;

    std::any visit(Annotation::NameRef* annotation) override;
    std::any visit(Annotation::Pointer* annotation) override;
    std::any visit(Annotation::Reference* annotation) override;
    std::any visit(Annotation::Array* annotation) override;
    std::any visit(Annotation::Object* annotation) override;
    std::any visit(Annotation::Tuple* annotation) override;

public:
    LocalChecker() = default;

    /**
     * @brief Type checks the given context at the local level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param context The front end context containing the AST to type check.
     */
    void check(std::unique_ptr<FrontendContext>& context);
};

#endif // NICO_LOCAL_CHECKER_H
