#ifndef NICO_LOCAL_CHECKER_H
#define NICO_LOCAL_CHECKER_H

#include <any>
#include <memory>
#include <vector>

#include "../parser/stmt.h"
#include "symbol_table.h"

/**
 * @brief A local type checker.
 *
 * The local type checker checks statements and expressions at the local level, i.e., within functions, blocks, and the main script.
 */
class LocalChecker : public Stmt::Visitor, public Expr::Visitor {
    // The symbol table for this local type checker.
    std::unique_ptr<SymbolTable> symbol_table;

    std::any visit(Stmt::Expression* stmt) override;
    std::any visit(Stmt::Let* stmt) override;
    std::any visit(Stmt::Eof* stmt) override;
    std::any visit(Stmt::Print* stmt) override;

    std::any visit(Expr::Assign* expr, bool as_lvalue) override;
    std::any visit(Expr::Binary* expr, bool as_lvalue) override;
    std::any visit(Expr::Unary* expr, bool as_lvalue) override;
    std::any visit(Expr::Identifier* expr, bool as_lvalue) override;
    std::any visit(Expr::Literal* expr, bool as_lvalue) override;

public:
    LocalChecker() : symbol_table(std::make_unique<SymbolTable>()) {}

    /**
     * @brief Type checks the given AST at the local level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param ast The list of statements to type check.
     */
    void check(std::vector<std::shared_ptr<Stmt>> ast);

    // TODO: Consider adding `reset` function.
};

#endif // NICO_LOCAL_CHECKER_H
