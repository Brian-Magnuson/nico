#ifndef NICO_LOCAL_CHECKER_H
#define NICO_LOCAL_CHECKER_H

#include <any>
#include <memory>
#include <vector>

#include "../nodes/ast_node.h"
#include "../parser/ast.h"

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

    std::any visit(Stmt::Expression* stmt) override;
    std::any visit(Stmt::Let* stmt) override;
    std::any visit(Stmt::Print* stmt) override;
    std::any visit(Stmt::Yield* stmt) override;
    std::any visit(Stmt::Eof* stmt) override;

    std::any visit(Expr::Assign* expr, bool as_lvalue) override;
    std::any visit(Expr::Binary* expr, bool as_lvalue) override;
    std::any visit(Expr::Unary* expr, bool as_lvalue) override;
    std::any visit(Expr::NameRef* expr, bool as_lvalue) override;
    std::any visit(Expr::Literal* expr, bool as_lvalue) override;
    std::any visit(Expr::Tuple* expr, bool as_lvalue) override;
    std::any visit(Expr::Block* expr, bool as_lvalue) override;

    std::any visit(Annotation::NameRef* annotation) override;
    std::any visit(Annotation::Pointer* annotation) override;
    std::any visit(Annotation::Reference* annotation) override;
    std::any visit(Annotation::Array* annotation) override;
    std::any visit(Annotation::Object* annotation) override;
    std::any visit(Annotation::Tuple* annotation) override;

public:
    LocalChecker() = default;

    /**
     * @brief Type checks the given AST at the local level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param ast The AST to type check.
     */
    void check(Ast& ast);
};

#endif // NICO_LOCAL_CHECKER_H
