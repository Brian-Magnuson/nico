#ifndef NICO_AST_PRINTER_H
#define NICO_AST_PRINTER_H

#include <any>
#include <memory>
#include <string>
#include <vector>

#include "../parser/ast.h"

/**
 * @brief A class for printing the AST for debugging purposes.
 *
 * All visit functions should return an std::string representing the AST node.
 * This class does not need to be reset after use as it does not store any
 * state.
 */
class AstPrinter : public Stmt::Visitor, public Expr::Visitor {
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

public:
    /**
     * @brief Converts a single statement to a string.
     *
     * Used for debugging purposes.
     *
     * @param stmt The statement to convert.
     * @return A string representing the statement.
     */
    std::string stmt_to_string(std::shared_ptr<Stmt> stmt);

    /**
     * @brief Converts a list of statements to a list of strings.
     *
     * Used for debugging purposes.
     *
     * @param stmts The list of statements to convert.
     * @return A list of strings representing the statements.
     */
    std::vector<std::string>
    stmts_to_strings(const std::vector<std::shared_ptr<Stmt>>& stmts);
};

#endif // NICO_AST_PRINTER_H
