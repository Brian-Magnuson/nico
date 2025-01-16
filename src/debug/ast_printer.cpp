#include "ast_printer.h"

std::any AstPrinter::visit(Stmt::Expression* stmt) {
    return std::string("(expr " + std::any_cast<std::string>(stmt->expression->accept(this, false)) + ")");
}

std::any AstPrinter::visit(Stmt::Eof* /*stmt*/) {
    return std::string("(stmt:eof)");
}

std::any AstPrinter::visit(Expr::Unary* expr, bool as_lvalue) {
    return std::string("(unary " + std::string(expr->op->lexeme) + " " + std::any_cast<std::string>(expr->right->accept(this, false)) + ")");
}

std::any AstPrinter::visit(Expr::Identifier* expr, bool as_lvalue) {
    return std::string("(ident " + std::string(expr->token->lexeme) + ")");
}

std::any AstPrinter::visit(Expr::Literal* expr, bool as_lvalue) {
    return std::string("(lit " + std::string(expr->token->lexeme) + ")");
}

std::string AstPrinter::stmt_to_string(std::shared_ptr<Stmt> stmt) {
    return std::any_cast<std::string>(stmt->accept(this));
}

std::vector<std::string> AstPrinter::stmts_to_strings(const std::vector<std::shared_ptr<Stmt>>& stmts) {
    std::vector<std::string> strings;
    for (const auto& stmt : stmts) {
        strings.push_back(stmt_to_string(stmt));
    }
    return strings;
}
