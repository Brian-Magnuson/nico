#include "ast_printer.h"

std::any AstPrinter::visit(Stmt::Expression* stmt) {
    return std::string(
        "(expr " +
        std::any_cast<std::string>(stmt->expression->accept(this, false)) + ")"
    );
}

std::any AstPrinter::visit(Stmt::Let* stmt) {
    std::string str = "(stmt:let ";
    if (stmt->has_var) {
        str += "var ";
    }
    str += std::string(stmt->identifier->lexeme);
    if (stmt->annotation.has_value()) {
        str += " " + stmt->annotation.value()->to_string();
    }
    if (stmt->expression.has_value()) {
        str += " " + std::any_cast<std::string>(
                         stmt->expression.value()->accept(this, false)
                     );
    }
    str += ")";
    return str;
}

std::any AstPrinter::visit(Stmt::Print* stmt) {
    std::string str = "(stmt:print";
    for (const auto& expr : stmt->expressions) {
        str += " " + std::any_cast<std::string>(expr->accept(this, false));
    }
    str += ")";
    return str;
}

std::any AstPrinter::visit(Stmt::Pass* /*stmt*/) {
    return std::string("(stmt:pass)");
}

std::any AstPrinter::visit(Stmt::Yield* stmt) {
    std::string str = "(stmt:yield ";
    str += std::any_cast<std::string>(stmt->expression->accept(this, false));
    str += ")";
    return str;
}

std::any AstPrinter::visit(Stmt::Eof* /*stmt*/) {
    return std::string("(stmt:eof)");
}

std::any AstPrinter::visit(Expr::Assign* expr, bool as_lvalue) {
    auto left = std::any_cast<std::string>(expr->left->accept(this, true));
    auto right = std::any_cast<std::string>(expr->right->accept(this, false));
    return std::string("(assign " + left + " " + right + ")");
}

std::any AstPrinter::visit(Expr::Binary* expr, bool as_lvalue) {
    auto left = std::any_cast<std::string>(expr->left->accept(this, false));
    auto right = std::any_cast<std::string>(expr->right->accept(this, false));
    return std::string(
        "(binary " + std::string(expr->op->lexeme) + " " + left + " " + right +
        ")"
    );
}

std::any AstPrinter::visit(Expr::Unary* expr, bool as_lvalue) {
    return std::string(
        "(unary " + std::string(expr->op->lexeme) + " " +
        std::any_cast<std::string>(expr->right->accept(this, false)) + ")"
    );
}

std::any AstPrinter::visit(Expr::Access* expr, bool as_lvalue) {
    auto left = std::any_cast<std::string>(expr->left->accept(this, false));
    auto right = std::any_cast<std::string>(expr->right->accept(this, false));
    return std::string(
        "(access " + left + " " + std::string(expr->op->lexeme) + " " + right +
        ")"
    );
}

std::any AstPrinter::visit(Expr::NameRef* expr, bool as_lvalue) {
    return std::string("(nameref " + expr->name.to_string() + ")");
}

std::any AstPrinter::visit(Expr::Literal* expr, bool as_lvalue) {
    return std::string("(lit " + std::string(expr->token->lexeme) + ")");
}

std::any AstPrinter::visit(Expr::Tuple* expr, bool as_lvalue) {
    std::string str = "(tuple";
    for (const auto& element : expr->elements) {
        str += " " + std::any_cast<std::string>(element->accept(this, false));
    }
    str += ")";
    return str;
}

std::any AstPrinter::visit(Expr::Block* expr, bool as_lvalue) {
    std::string str = "(block";
    for (const auto& stmt : expr->statements) {
        str += " " + std::any_cast<std::string>(stmt->accept(this));
    }
    str += ")";
    return str;
}

std::string AstPrinter::stmt_to_string(std::shared_ptr<Stmt> stmt) {
    return std::any_cast<std::string>(stmt->accept(this));
}

std::vector<std::string> AstPrinter::stmts_to_strings(const Ast& ast) {
    std::vector<std::string> strings;
    for (const auto& stmt : ast.stmts) {
        strings.push_back(stmt_to_string(stmt));
    }
    return strings;
}
