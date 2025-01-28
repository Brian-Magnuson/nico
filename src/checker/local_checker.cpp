#include "local_checker.h"

#include "../logger/error_code.h"
#include "../logger/logger.h"

std::any LocalChecker::visit(Stmt::Expression* stmt) {
    // Visit the expression.
    stmt->expression->accept(this, false);
    return std::any();
}

std::any LocalChecker::visit(Stmt::Let* stmt) {
    std::shared_ptr<Type> expr_type = nullptr;

    // Visit the initializer (if present).
    if (stmt->expression.has_value()) {
        auto type = stmt->expression.value()->accept(this, false);
        if (type.has_value()) {
            expr_type = std::any_cast<std::shared_ptr<Type>>(type);
        } else {
            // Error should already be logged.
            return std::any();
        }
    }
    // If the initializer is not present, the annotation will be.

    // If the type annotation is present, check that it matches the initializer.
    if (expr_type != nullptr && stmt->annotation.has_value()) {
        auto annotation_type = stmt->annotation.value();
        if (*expr_type != *annotation_type) {
            Logger::inst().log_error(
                Err::LetTypeMismatch,
                stmt->identifier->location,
                std::string("Type `") + expr_type->to_string() + "` is not compatible with type `" + annotation_type->to_string() + "`."
            );
            return std::any();
        }
    }

    // If the annotation is not present at this point, we add it in manually.
    if (!stmt->annotation) {
        stmt->annotation = expr_type;
    }

    // Insert the variable into the symbol table.
    symbol_table->insert(
        std::string(stmt->identifier->lexeme),
        stmt->annotation.value()
    );
}

std::any LocalChecker::visit(Stmt::Eof* stmt) {
    return std::any();
}

std::any LocalChecker::visit(Stmt::Print* stmt) {
    // Visit each expression in the print statement.
    for (auto& expr : stmt->expressions) {
        expr->accept(this, false);
    }
    return std::any();
}

std::any LocalChecker::visit(Expr::Assign* expr, bool as_lvalue) {
    // An assignment expression should never be an lvalue.
    if (as_lvalue) {
        Logger::inst().log_error(Err::NotAnLValue, expr->op->location, "Assignment expression cannot be an lvalue.");
    }

    auto l_type = std::any_cast<std::shared_ptr<Type>>(expr->left->accept(this, true));
    auto r_type = std::any_cast<std::shared_ptr<Type>>(expr->right->accept(this, false));

    // The types of the left and right sides must match.
    if (*l_type != *r_type) {
        Logger::inst().log_error(
            Err::AssignmentTypeMismatch,
            expr->op->location,
            std::string("Type `") + r_type->to_string() + "` is not compatible with type `" + l_type->to_string() + "`."
        );
    }

    return l_type;
}

std::any LocalChecker::visit(Expr::Binary* expr, bool as_lvalue) {
    auto l_type = std::any_cast<std::shared_ptr<Type>>(expr->left->accept(this, false));
    auto r_type = std::any_cast<std::shared_ptr<Type>>(expr->right->accept(this, false));

    switch (expr->op->tok_type) {
    case Tok::Plus:
    case Tok::Minus:
    case Tok::Star:
    case Tok::Slash:
        // Both operands must be of the same type.
        if (*l_type != *r_type) {
            Logger::inst().log_error(
                Err::TypeMismatch,
                expr->op->location,
                std::string("Type `") + r_type->to_string() + "` is not compatible with type `" + l_type->to_string() + "`."
            );
        }
        // Types must inherit from `Type::INumeric`.
        if (!dynamic_cast<Type::INumeric*>(l_type.get())) {
            Logger::inst().log_error(Err::AssignmentTypeMismatch, expr->op->location, "Operands must be of a numeric type.");
        }
        return l_type;
    default:
        Logger::inst().log_error(Err::Unimplemented, expr->op->location, "Binary operator not implemented.");
        return std::any();
    }
}

std::any LocalChecker::visit(Expr::Unary* expr, bool as_lvalue) {
    auto type = std::any_cast<std::shared_ptr<Type>>(expr->right->accept(this, false));

    switch (expr->op->tok_type) {
    case Tok::Minus:
        // Types must inherit from `Type::INumeric`.
        if (!dynamic_cast<Type::INumeric*>(type.get())) {
            Logger::inst().log_error(Err::AssignmentTypeMismatch, expr->op->location, "Operand must be of a numeric type.");
        }
        return type;
    default:
        Logger::inst().log_error(Err::Unimplemented, expr->op->location, "Unary operator not implemented.");
    }

    return std::any();
}

std::any LocalChecker::visit(Expr::Identifier* expr, bool /*as_lvalue*/) {
    // Ignore `as_value` since identifiers may be lvalues or rvalues.
    auto var_type = symbol_table->get(std::string(expr->token->lexeme));
    if (!var_type.has_value()) {
        Logger::inst().log_error(Err::UndeclaredIdentifier, expr->token->location, "Identifier `" + std::string(expr->token->lexeme) + "` was not declared.");
        return std::any();
    }
    return *var_type;
}

std::any LocalChecker::visit(Expr::Literal* expr, bool as_lvalue) {
    std::shared_ptr<Type> type = nullptr;
    switch (expr->token->tok_type) {
    case Tok::Int:
        type = std::make_shared<Type::Int>(32);
        break;
    case Tok::Float:
        type = std::make_shared<Type::Float>(64);
        break;
    case Tok::Bool:
        type = std::make_shared<Type::Bool>();
        break;
    default:
        Logger::inst().log_error(Err::Unimplemented, expr->token->location, "Literal type not implemented.");
        return std::any();
    }
    return type;
}

void LocalChecker::check(std::vector<std::shared_ptr<Stmt>> ast) {
    for (auto& stmt : ast) {
        stmt->accept(this);
    }
}
