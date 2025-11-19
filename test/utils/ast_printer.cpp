#include "ast_printer.h"

namespace nico {

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

std::any AstPrinter::visit(Stmt::Func* stmt) {
    /*
    (stmt:func func_name ret_type (var param1 type1 default1) (param2 type2) =>
    body_expr)
    */
    std::string str =
        "(stmt:func " + std::string(stmt->identifier->lexeme) + " ";
    if (stmt->annotation.has_value()) {
        str += stmt->annotation.value()->to_string() + " ";
    }
    str += "(";
    for (const auto& param : stmt->parameters) {

        if (param.has_var) {
            str += "var ";
        }
        str += std::string(param.identifier->lexeme) + " " +
               param.annotation->to_string();

        if (param.expression.has_value()) {
            str += " " + std::any_cast<std::string>(
                             param.expression.value()->accept(this, false)
                         );
        }
    }
    str +=
        ") => " + std::any_cast<std::string>(stmt->body->accept(this, false));
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
    str += std::string(stmt->yield_token->lexeme) + " ";
    str += std::any_cast<std::string>(stmt->expression->accept(this, false));
    str += ")";
    return str;
}

std::any AstPrinter::visit(Stmt::Continue* /*stmt*/) {
    return std::string("(stmt:continue)");
}

std::any AstPrinter::visit(Stmt::Eof* /*stmt*/) {
    return std::string("(stmt:eof)");
}

std::any AstPrinter::visit(Expr::Assign* expr, bool as_lvalue) {
    auto left = std::any_cast<std::string>(expr->left->accept(this, true));
    auto right = std::any_cast<std::string>(expr->right->accept(this, false));
    return std::string("(assign " + left + " " + right + ")");
}

std::any AstPrinter::visit(Expr::Logical* expr, bool as_lvalue) {
    auto left = std::any_cast<std::string>(expr->left->accept(this, false));
    auto right = std::any_cast<std::string>(expr->right->accept(this, false));
    return std::string(
        "(logical " + std::string(expr->op->lexeme) + " " + left + " " + right +
        ")"
    );
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

std::any AstPrinter::visit(Expr::Address* expr, bool as_lvalue) {
    return std::string(
        std::string("(address ") + (expr->has_var ? "var" : "") +
        std::string(expr->op->lexeme) + " " +
        std::any_cast<std::string>(expr->right->accept(this, false)) + ")"
    );
}

std::any AstPrinter::visit(Expr::Deref* expr, bool as_lvalue) {
    return std::string(
        "(deref " +
        std::any_cast<std::string>(expr->right->accept(this, false)) + ")"
    );
}

std::any AstPrinter::visit(Expr::Cast* expr, bool as_lvalue) {
    auto inner =
        std::any_cast<std::string>(expr->expression->accept(this, false));
    return std::string(
        "(cast " + inner + " as " + expr->target_type->to_string() + ")"
    );
}

std::any AstPrinter::visit(Expr::Access* expr, bool as_lvalue) {
    auto left = std::any_cast<std::string>(expr->left->accept(this, false));
    return std::string(
        "(access " + std::string(expr->op->lexeme) + " " + left + " " +
        std::string(expr->right_token->lexeme) + ")"
    );
}

std::any AstPrinter::visit(Expr::Call* expr, bool as_lvalue) {
    std::string str = "(call ";
    str += std::any_cast<std::string>(expr->callee->accept(this, false));
    for (const auto& arg : expr->provided_pos_args) {
        str += " " + std::any_cast<std::string>(arg->accept(this, false));
    }
    for (const auto& [name, arg] : expr->provided_named_args) {
        str += " (" + name + ": " +
               std::any_cast<std::string>(arg->accept(this, false)) + ")";
    }
    str += ")";
    return str;
}

std::any AstPrinter::visit(Expr::NameRef* expr, bool as_lvalue) {
    return std::string("(nameref " + expr->name.to_string() + ")");
}

std::any AstPrinter::visit(Expr::Literal* expr, bool as_lvalue) {
    if (expr->token->tok_type == Tok::IntAny) {
        return std::string(
            "(lit " +
            std::to_string(std::any_cast<int32_t>(expr->token->literal)) + ")"
        );
    }
    else if (expr->token->tok_type == Tok::FloatAny) {
        return std::string(
            "(lit " +
            std::to_string(std::any_cast<double>(expr->token->literal)) + ")"
        );
    }
    else {
        return std::string("(lit " + std::string(expr->token->lexeme) + ")");
    }
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
    if (expr->is_unsafe) {
        str += " unsafe";
    }
    for (const auto& stmt : expr->statements) {
        str += " " + std::any_cast<std::string>(stmt->accept(this));
    }
    str += ")";
    return str;
}

std::any AstPrinter::visit(Expr::Conditional* expr, bool as_lvalue) {
    std::string str = "(if ";
    str += std::any_cast<std::string>(expr->condition->accept(this, false));
    str += " then ";
    str += std::any_cast<std::string>(expr->then_branch->accept(this, false));
    str += " else ";
    str += std::any_cast<std::string>(expr->else_branch->accept(this, false));

    str += ")";
    return str;
}

std::any AstPrinter::visit(Expr::Loop* expr, bool as_lvalue) {
    std::string str = "(loop ";
    if (expr->condition.has_value()) {
        if (expr->loops_once)
            str += "do ";
        str += "while ";
        str += std::any_cast<std::string>(
            expr->condition.value()->accept(this, false)
        );
        str += " ";
    }
    str += std::any_cast<std::string>(expr->body->accept(this, false));
    str += ")";
    return str;
}

std::string AstPrinter::stmt_to_string(std::shared_ptr<Stmt> stmt) {
    return std::any_cast<std::string>(stmt->accept(this));
}

std::vector<std::string>
AstPrinter::stmts_to_strings(const std::vector<std::shared_ptr<Stmt>>& ast) {
    std::vector<std::string> strings;
    for (const auto& stmt : ast) {
        strings.push_back(stmt_to_string(stmt));
    }
    return strings;
}

} // namespace nico
