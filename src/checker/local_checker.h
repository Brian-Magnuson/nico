#ifndef NICO_LOCAL_CHECKER_H
#define NICO_LOCAL_CHECKER_H

#include <any>
#include <memory>
#include <vector>

#include "../parser/annotation.h"
#include "../parser/stmt.h"
#include "symbol_tree.h"

/**
 * @brief A local type checker.
 *
 * The local type checker checks statements and expressions at the local level, i.e., within functions, blocks, and the main script.
 */
class LocalChecker : public Stmt::Visitor, public Expr::Visitor, public Annotation::Visitor {
    // The symbol tree used for type checking.
    std::unique_ptr<SymbolTree> symbol_tree;

    std::any visit(Stmt::Expression* stmt) override;
    std::any visit(Stmt::Let* stmt) override;
    std::any visit(Stmt::Eof* stmt) override;
    std::any visit(Stmt::Print* stmt) override;

    std::any visit(Expr::Assign* expr, bool as_lvalue) override;
    std::any visit(Expr::Binary* expr, bool as_lvalue) override;
    std::any visit(Expr::Unary* expr, bool as_lvalue) override;
    std::any visit(Expr::Identifier* expr, bool as_lvalue) override;
    std::any visit(Expr::Literal* expr, bool as_lvalue) override;

    std::any visit(Annotation::Named* annotation) override;
    std::any visit(Annotation::Pointer* annotation) override;
    std::any visit(Annotation::Reference* annotation) override;
    std::any visit(Annotation::Array* annotation) override;
    std::any visit(Annotation::Object* annotation) override;
    std::any visit(Annotation::Tuple* annotation) override;

public:
    LocalChecker() : symbol_tree(std::make_unique<SymbolTree>()) {}

    /**
     * @brief Type checks the given AST at the local level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param ast The list of statements to type check.
     */
    void check(std::vector<std::shared_ptr<Stmt>> ast);

    /**
     * @brief Resets the local type checker.
     *
     * This function will reset the symbol table.
     */
    void reset();
};

#endif // NICO_LOCAL_CHECKER_H
