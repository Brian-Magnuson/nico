#ifndef NICO_AST_PRINTER_H
#define NICO_AST_PRINTER_H

#include <any>
#include <memory>
#include <string>
#include <vector>

#include "stmt.h"

/**
 * @brief A class for printing the AST for debugging purposes.
 *
 * All visit functions should return an std::string representing the AST node.
 */
class AstPrinter : public Stmt::Visitor, public Expr::Visitor {
public:
    std::any visit(Stmt::Expression* stmt) override;
    std::any visit(Stmt::Eof* stmt) override;

    std::any visit(Expr::Identifier* expr, bool as_lvalue) override;
    std::any visit(Expr::Literal* expr, bool as_lvalue) override;

    std::string stmt_to_string(std::shared_ptr<Stmt> stmt);
    std::vector<std::string> stmts_to_strings(const std::vector<std::shared_ptr<Stmt>>& stmts);
};

#endif // NICO_AST_PRINTER_H
