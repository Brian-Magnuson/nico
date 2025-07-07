#include "local_checker.h"

#include "../logger/error_code.h"
#include "../logger/logger.h"

// MARK: Statements

std::any LocalChecker::visit(Stmt::Expression* stmt) {
    // Visit the expression.
    stmt->expression->accept(this, false);
    return std::any();
}

std::any LocalChecker::visit(Stmt::Let* stmt) {
    std::shared_ptr<Type> expr_type = nullptr;

    // // Visit the initializer (if present).
    // if (stmt->expression.has_value()) {
    //     auto type = stmt->expression.value()->accept(this, false);
    //     if (type.has_value()) {
    //         expr_type = std::any_cast<std::shared_ptr<Type>>(type);
    //     } else {
    //         // Error should already be logged.
    //         return std::any();
    //     }
    // }
    // // If the initializer is not present, the annotation will be.

    // // If the type annotation is present, check that it matches the initializer.
    // if (expr_type != nullptr && stmt->_annotation.has_value()) {
    //     auto annotation_type = stmt->_annotation.value();
    //     if (*expr_type != *annotation_type) {
    //         Logger::inst().log_error(
    //             Err::LetTypeMismatch,
    //             *stmt->expression.value()->location,
    //             std::string("Type `") + expr_type->to_string() + "` is not compatible with type `" + annotation_type->to_string() + "`."
    //         );
    //         return std::any();
    //     }
    // }

    // // If the annotation is not present at this point, we add it in manually.
    // if (!stmt->_annotation) {
    //     stmt->_annotation = expr_type;
    // }

    // // Insert the variable into the symbol table.
    // symbol_table->insert(
    //     std::string(stmt->identifier->lexeme),
    //     stmt->has_var,
    //     stmt->identifier,
    //     stmt->_annotation.value()
    // );

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
    // If the initializer is not present, the annotation will be (this is checked in the parser).

    // If the type annotation is present, check that it matches the initializer.
    if (expr_type != nullptr && stmt->annotation.has_value()) {
        auto anno_any = stmt->annotation.value()->accept(this);
        if (!anno_any.has_value())
            return std::any();
        auto annotation_type = std::any_cast<std::shared_ptr<Type>>(anno_any);
        if (*expr_type != *annotation_type) {
            Logger::inst().log_error(
                Err::LetTypeMismatch,
                *stmt->expression.value()->location,
                std::string("Type `") + expr_type->to_string() + "` is not compatible with type `" + annotation_type->to_string() + "`."
            );
            return std::any();
        }
    }

    // Create the field entry.
    Field field(stmt->has_var, stmt->identifier, expr_type);
    auto new_node = symbol_tree->add_field_entry(field);
    if (!new_node) {
        Logger::inst().log_error(Err::Impossible, stmt->identifier->location, "Failed to add variable to symbol tree.");
        return std::any();
    }

    return std::any();
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

// MARK: Expressions

std::any LocalChecker::visit(Expr::Assign* expr, bool as_lvalue) {
    // An assignment expression should never be an lvalue.
    if (as_lvalue) {
        Logger::inst().log_error(Err::NotAnLValue, expr->op->location, "Assignment expression cannot be an lvalue.");
    }

    auto l_any = expr->left->accept(this, true);
    if (!l_any.has_value())
        return std::any();
    auto l_type = std::any_cast<std::shared_ptr<Type>>(l_any);

    auto r_any = expr->right->accept(this, false);
    if (!r_any.has_value())
        return std::any();
    auto r_type = std::any_cast<std::shared_ptr<Type>>(r_any);

    // The types of the left and right sides must match.
    if (*l_type != *r_type) {
        Logger::inst().log_error(
            Err::AssignmentTypeMismatch,
            expr->op->location,
            std::string("Type `") + r_type->to_string() + "` is not compatible with type `" + l_type->to_string() + "`."
        );
        return std::any();
    }

    return l_type;
}

std::any LocalChecker::visit(Expr::Binary* expr, bool as_lvalue) {
    auto l_any = expr->left->accept(this, true);
    if (!l_any.has_value())
        return std::any();
    auto l_type = std::any_cast<std::shared_ptr<Type>>(l_any);

    auto r_any = expr->right->accept(this, false);
    if (!r_any.has_value())
        return std::any();
    auto r_type = std::any_cast<std::shared_ptr<Type>>(r_any);

    switch (expr->op->tok_type) {
    case Tok::Plus:
    case Tok::Minus:
    case Tok::Star:
    case Tok::Slash:
        // Both operands must be of the same type.
        if (*l_type != *r_type) {
            Logger::inst().log_error(
                Err::NoOperatorOverload,
                expr->op->location,
                std::string("Type `") + r_type->to_string() + "` is not compatible with type `" + l_type->to_string() + "`."
            );
            return std::any();
        }
        // Types must inherit from `Type::INumeric`.
        if (!dynamic_cast<Type::INumeric*>(l_type.get())) {
            Logger::inst().log_error(Err::NoOperatorOverload, expr->op->location, "Operands must be of a numeric type.");
            return std::any();
        }
        return l_type;
    default:
        Logger::inst().log_error(Err::Unimplemented, expr->op->location, "Binary operator not implemented.");
        return std::any();
    }
}

std::any LocalChecker::visit(Expr::Unary* expr, bool as_lvalue) {
    auto r_any = expr->right->accept(this, false);
    if (!r_any.has_value())
        return std::any();
    auto type = std::any_cast<std::shared_ptr<Type>>(r_any);

    switch (expr->op->tok_type) {
    case Tok::Minus:
        // Types must inherit from `Type::INumeric`.
        if (!dynamic_cast<Type::INumeric*>(type.get())) {
            Logger::inst().log_error(Err::NoOperatorOverload, expr->op->location, "Operand must be of a numeric type.");
            return std::any();
        }
        return type;
    default:
        Logger::inst().log_error(Err::Unimplemented, expr->op->location, "Unary operator not implemented.");
        return std::any();
    }
}

std::any LocalChecker::visit(Expr::Identifier* expr, bool as_lvalue) {
    // auto var_entry = symbol_table->get(std::string(expr->token->lexeme));
    // if (!var_entry.has_value()) {
    //     Logger::inst().log_error(Err::UndeclaredIdentifier, expr->token->location, "Identifier `" + std::string(expr->token->lexeme) + "` was not declared.");
    //     return std::any();
    // }
    // if (!var_entry->is_var && as_lvalue) {
    //     Logger::inst().log_error(Err::AssignToImmutable, expr->token->location, "Cannot assign to immutable identifier `" + std::string(expr->token->lexeme) + "`.");
    //     Logger::inst().log_note(var_entry->token->location, "Identifier declared here.");
    //     return std::any();
    // }

    auto node = symbol_tree->search_ident(expr->ident);
    if (!node) {
        Logger::inst().log_error(Err::UndeclaredIdentifier, expr->ident.parts.back().token->location, "Identifier `" + expr->ident.to_string() + "` was not declared.");
        return std::any();
    }
    if (std::dynamic_pointer_cast<Node::FieldEntry>(*node) == nullptr) {
        Logger::inst().log_error(Err::NotAVariable, expr->ident.parts.back().token->location, "Identifier `" + expr->ident.to_string() + "` is not a variable.");
        return std::any();
    }
    auto field_entry = std::dynamic_pointer_cast<Node::FieldEntry>(*node);
    if (!field_entry->field.is_var && as_lvalue) {
        Logger::inst().log_error(Err::AssignToImmutable, expr->ident.parts.back().token->location, "Cannot assign to immutable identifier `" + expr->ident.to_string() + "`.");
        Logger::inst().log_note(field_entry->field.token->location, "Identifier declared here.");
        return std::any();
    }

    return field_entry->field.type;
}

std::any LocalChecker::visit(Expr::Literal* expr, bool as_lvalue) {
    std::shared_ptr<Type> type = nullptr;
    switch (expr->token->tok_type) {
    case Tok::Int:
        type = std::make_shared<Type::Int>(true, 32);
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

// MARK: Annotations

std::any LocalChecker::visit(Annotation::Named* annotation) {
    std::shared_ptr<Type> type = nullptr;
    // Temporary solution: only allow primitive types.
    if (annotation->ident.to_string() == "i32") {
        type = std::make_shared<Type::Int>(true, 32);
    } else if (annotation->ident.to_string() == "f64") {
        type = std::make_shared<Type::Float>(64);
    } else if (annotation->ident.to_string() == "bool") {
        type = std::make_shared<Type::Bool>();
    }
    return type;
}

std::any LocalChecker::visit(Annotation::Pointer* annotation) {
    std::shared_ptr<Type> type = nullptr;
    auto base_any = annotation->base->accept(this);
    if (!base_any.has_value())
        return std::any();
    auto base_type = std::any_cast<std::shared_ptr<Type>>(base_any);
    type = std::make_shared<Type::Pointer>(base_type, annotation->is_mutable);
    return type;
}

std::any LocalChecker::visit(Annotation::Reference* annotation) {
    std::shared_ptr<Type> type = nullptr;
    auto base_any = annotation->base->accept(this);
    if (!base_any.has_value())
        return std::any();
    auto base_type = std::any_cast<std::shared_ptr<Type>>(base_any);
    type = std::make_shared<Type::Reference>(base_type, annotation->is_mutable);
    return type;
}

std::any LocalChecker::visit(Annotation::Array* annotation) {
    return std::any();
}

std::any LocalChecker::visit(Annotation::Object* annotation) {
    return std::any();
}

std::any LocalChecker::visit(Annotation::Tuple* annotation) {
    return std::any();
}

void LocalChecker::check(std::vector<std::shared_ptr<Stmt>> ast) {
    for (auto& stmt : ast) {
        stmt->accept(this);
    }
}

void LocalChecker::reset() {
    symbol_tree = std::make_unique<SymbolTree>();
}
