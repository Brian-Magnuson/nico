#include "local_checker.h"

#include "../common/utils.h"
#include "../logger/error_code.h"
#include "../logger/logger.h"
#include "../parser/type.h"

// MARK: Statements

std::any LocalChecker::visit(Stmt::Expression* stmt) {
    // Visit the expression.
    stmt->expression->accept(this, false);
    return std::any();
}

std::any LocalChecker::visit(Stmt::Let* stmt) {
    std::shared_ptr<Type> expr_type = nullptr;

    // Visit the initializer (if present).
    if (stmt->expression.has_value()) {
        stmt->expression.value()->accept(this, false);
        expr_type = stmt->expression.value()->type;
        if (!expr_type)
            return std::any();
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

    auto [node, err] = symbol_tree->add_field_entry(field);
    if (err == Err::NameAlreadyExists) {
        Logger::inst().log_error(err, stmt->identifier->location, "Name `" + std::string(stmt->identifier->lexeme) + "` already exists in this scope.");
        if (auto locatable = std::dynamic_pointer_cast<Node::ILocatable>(node)) {
            Logger::inst().log_note(locatable->location_token->location, "Previous declaration here.");
        }
        return std::any();
    } else if (err == Err::NameIsReserved) {
        Logger::inst().log_error(err, stmt->identifier->location, "Name `" + std::string(stmt->identifier->lexeme) + "` is reserved.");
        return std::any();
    } else if (auto field_node = std::dynamic_pointer_cast<Node::FieldEntry>(node)) {
        stmt->field_entry = field_node;
        return std::any();
    } else {
        panic("LocalChecker::visit(Stmt::Let*): Symbol tree returned a non-field entry for a field entry.");
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

    expr->left->accept(this, true);
    auto l_type = expr->left->type;
    if (!l_type)
        return std::any();

    expr->right->accept(this, false);
    auto r_type = expr->right->type;
    if (!r_type)
        return std::any();

    // The types of the left and right sides must match.
    if (*l_type != *r_type) {
        Logger::inst().log_error(
            Err::AssignmentTypeMismatch,
            expr->op->location,
            std::string("Type `") + r_type->to_string() + "` is not compatible with type `" + l_type->to_string() + "`."
        );
        return std::any();
    }

    expr->type = l_type;
    return std::any();
}

std::any LocalChecker::visit(Expr::Binary* expr, bool as_lvalue) {
    expr->left->accept(this, false);
    auto l_type = expr->left->type;
    if (!l_type)
        return std::any();

    expr->right->accept(this, false);
    auto r_type = expr->right->type;
    if (!r_type)
        return std::any();

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
        if (!PTR_INSTANCEOF(l_type, Type::INumeric)) {
            Logger::inst().log_error(Err::NoOperatorOverload, expr->op->location, "Operands must be of a numeric type.");
            return std::any();
        }
        expr->type = l_type; // The result type is the same as the operand type.
        break;
    default:
        panic("LocalChecker::visit(Expr::Binary*): Could not handle case for operator of token type " + std::to_string(static_cast<int>(expr->op->tok_type)));
    }
    return std::any();
}

std::any LocalChecker::visit(Expr::Unary* expr, bool as_lvalue) {
    expr->right->accept(this, false);
    auto r_type = expr->right->type;
    if (!r_type)
        return std::any();

    switch (expr->op->tok_type) {
    case Tok::Minus:
        // Types must inherit from `Type::INumeric`.
        if (!PTR_INSTANCEOF(r_type, Type::INumeric)) {
            Logger::inst().log_error(Err::NoOperatorOverload, expr->op->location, "Operand must be of a numeric type.");
            return std::any();
        }
        expr->type = r_type;
        return std::any();
    default:
        panic("LocalChecker::visit(Expr::Unary*): Could not handle case for operator of token type " + std::to_string(static_cast<int>(expr->op->tok_type)));
        return std::any();
    }
}

std::any LocalChecker::visit(Expr::NameRef* expr, bool as_lvalue) {
    auto node = symbol_tree->search_name(expr->name);

    if (!node) {
        Logger::inst().log_error(Err::UndeclaredName, expr->name.parts.back().token->location, "Name `" + expr->name.to_string() + "` was not declared.");
        return std::any();
    }
    if (!PTR_INSTANCEOF(*node, Node::FieldEntry)) {
        Logger::inst().log_error(Err::NotAVariable, expr->name.parts.back().token->location, "Name reference `" + expr->name.to_string() + "` is not a variable.");
        return std::any();
    }
    auto field_entry = std::dynamic_pointer_cast<Node::FieldEntry>(*node);
    if (!field_entry->field.is_var && as_lvalue) {
        Logger::inst().log_error(Err::AssignToImmutable, expr->name.parts.back().token->location, "Cannot assign to immutable binding `" + expr->name.to_string() + "`.");
        Logger::inst().log_note(field_entry->field.token->location, "Binding introduced here.");
        return std::any();
    }

    expr->type = field_entry->field.type;
    expr->field_entry = field_entry;
    return std::any();
}

std::any LocalChecker::visit(Expr::Literal* expr, bool as_lvalue) {
    switch (expr->token->tok_type) {
    case Tok::Int:
        expr->type = std::make_shared<Type::Int>(true, 32);
        break;
    case Tok::Float:
        expr->type = std::make_shared<Type::Float>(64);
        break;
    case Tok::Bool:
        expr->type = std::make_shared<Type::Bool>();
        break;
    case Tok::Str:
        expr->type = std::make_shared<Type::Str>();
        break;
    default:
        panic("LocalChecker::visit(Expr::Literal*): Could not handle case for token type " + std::to_string(static_cast<int>(expr->token->tok_type)));
    }
    return std::any();
}

// MARK: Annotations

std::any LocalChecker::visit(Annotation::NameRef* annotation) {
    std::shared_ptr<Type> type = nullptr;
    // Temporary solution: only allow primitive types.
    if (annotation->name.to_string() == "i32") {
        type = std::make_shared<Type::Int>(true, 32);
    } else if (annotation->name.to_string() == "f64") {
        type = std::make_shared<Type::Float>(64);
    } else if (annotation->name.to_string() == "bool") {
        type = std::make_shared<Type::Bool>();
    } else if (annotation->name.to_string() == "str") {
        type = std::make_shared<Type::Str>();
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

void LocalChecker::check(std::vector<std::shared_ptr<Stmt>>& ast) {
    for (auto& stmt : ast) {
        stmt->accept(this);
    }
}
