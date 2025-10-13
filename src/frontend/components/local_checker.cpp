#include "nico/frontend/components/local_checker.h"

#include "nico/shared/error_code.h"
#include "nico/shared/logger.h"
#include "nico/shared/status.h"
#include "nico/shared/utils.h"

namespace nico {

std::shared_ptr<Type>
LocalChecker::expr_check(std::shared_ptr<Expr>& expr, bool as_lvalue) {
    expr->accept(this, as_lvalue);
    if (auto ref_type =
            std::dynamic_pointer_cast<Type::Reference>(expr->type)) {
        // If the expression is a reference type, modify the expression to be a
        // dereference expression.
        auto deref_expr = std::make_shared<Expr::Deref>(
            std::make_shared<Token>(Tok::Star, *expr->location),
            expr
        );
        deref_expr->type = ref_type->base;
        deref_expr->assignable = ref_type->is_mutable;
        expr = deref_expr;
    }
    return expr->type;
}

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
        expr_type = expr_check(stmt->expression.value(), false);
        if (!expr_type)
            return std::any();
    }
    // If the initializer is not present, the annotation will be (this is
    // checked in the parser).

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
                std::string("Type `") + expr_type->to_string() +
                    "` is not compatible with type `" +
                    annotation_type->to_string() + "`."
            );
            return std::any();
        }
    }

    // Create the field entry.
    Field field(stmt->has_var, stmt->identifier, expr_type);

    auto [node, err] = symbol_tree->add_field_entry(field);
    if (err == Err::NameAlreadyExists) {
        Logger::inst().log_error(
            err,
            stmt->identifier->location,
            "Name `" + std::string(stmt->identifier->lexeme) +
                "` already exists in this scope."
        );
        if (auto locatable =
                std::dynamic_pointer_cast<Node::ILocatable>(node)) {
            Logger::inst().log_note(
                locatable->location_token->location,
                "Previous declaration here."
            );
        }
        return std::any();
    }
    else if (err == Err::NameIsReserved) {
        Logger::inst().log_error(
            err,
            stmt->identifier->location,
            "Name `" + std::string(stmt->identifier->lexeme) + "` is reserved."
        );
        return std::any();
    }
    else if (auto field_node =
                 std::dynamic_pointer_cast<Node::FieldEntry>(node)) {
        stmt->field_entry = field_node;
        return std::any();
    }
    else {
        panic(
            "LocalChecker::visit(Stmt::Let*): Symbol tree returned a non-field "
            "entry for a field entry."
        );
    }

    return std::any();
}

std::any LocalChecker::visit(Stmt::Pass* /*stmt*/) {
    // A pass statement does nothing.
    return std::any();
}

std::any LocalChecker::visit(Stmt::Yield* stmt) {

    std::optional<std::shared_ptr<Node::LocalScope>> target_scope =
        std::nullopt;
    if (stmt->yield_token->tok_type == Tok::KwBreak) {
        target_scope =
            symbol_tree->get_local_scope_of_kind(Expr::Block::Kind::Loop);
        if (!target_scope) {
            Logger::inst().log_error(
                Err::BreakOutsideLoop,
                stmt->yield_token->location,
                "Cannot break outside of a loop."
            );
            return std::any();
        }
    }
    else if (stmt->yield_token->tok_type == Tok::KwReturn) {
        target_scope =
            symbol_tree->get_local_scope_of_kind(Expr::Block::Kind::Function);
        if (!target_scope) {
            Logger::inst().log_error(
                Err::ReturnOutsideFunction,
                stmt->yield_token->location,
                "Cannot return outside of a function."
            );
            return std::any();
        }
    }
    else if (stmt->yield_token->tok_type == Tok::KwYield) {
        target_scope = std::dynamic_pointer_cast<Node::LocalScope>(
            symbol_tree->current_scope
        );
        if (!target_scope.has_value() || target_scope.value() == nullptr) {
            Logger::inst().log_error(
                Err::YieldOutsideLocalScope,
                stmt->yield_token->location,
                "Cannot yield outside of a local scope."
            );
            return std::any();
        }
    }
    else {
        panic("LocalChecker::visit(Stmt::Yield*): Invalid yield token.");
    }
    // At this point, target_scope is guaranteed to be a valid local scope.
    auto local_scope = target_scope.value();

    // Visit the expression in the yield statement.
    auto expr_type = expr_check(stmt->expression, false);
    if (!expr_type)
        return std::any();

    if (!local_scope->yield_type) {
        // If this local scope does not currently have a yield type...
        local_scope->yield_type = expr_type;
    }
    else if (local_scope->yield_type.value() != expr_type) {
        // If this local scope has a yield type, check that the new yield
        // expression is compatible with it.
        Logger::inst().log_error(
            Err::YieldTypeMismatch,
            *stmt->expression->location,
            std::string("Type `") + stmt->expression->type->to_string() +
                "` is not compatible with previously yielded type `" +
                local_scope->yield_type.value()->to_string() + "`."
        );
    }

    return std::any();
}

std::any LocalChecker::visit(Stmt::Print* stmt) {
    // Visit each expression in the print statement.
    for (auto& expr : stmt->expressions) {
        // expr->accept(this, false);
        expr_check(expr, false);
    }
    return std::any();
}

std::any LocalChecker::visit(Stmt::Eof* stmt) {
    return std::any();
}

// MARK: Expressions

std::any LocalChecker::visit(Expr::Assign* expr, bool as_lvalue) {
    // An assignment expression should never be an lvalue.
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            expr->op->location,
            "Assignment expression cannot be an lvalue."
        );
    }

    auto l_type = expr_check(expr->left, true);
    auto l_lvalue = std::dynamic_pointer_cast<Expr::IPLValue>(expr->left);
    if (!l_type || !l_lvalue)
        return std::any();

    // The left side must be an assignable lvalue.
    if (!l_lvalue->assignable) {
        Logger::inst().log_error(
            Err::AssignToImmutable,
            *expr->left->location,
            "Left side of assignment is not assignable."
        );
        if (l_lvalue->error_location) {
            Logger::inst().log_note(
                *l_lvalue->error_location,
                "This is not mutable."
            );
        }
        return std::any();
    }

    auto r_type = expr_check(expr->right, false);
    if (!r_type)
        return std::any();

    // The types of the left and right sides must match.
    if (*l_type != *r_type) {
        Logger::inst().log_error(
            Err::AssignmentTypeMismatch,
            expr->op->location,
            std::string("Type `") + r_type->to_string() +
                "` is not compatible with type `" + l_type->to_string() + "`."
        );
        return std::any();
    }

    expr->type = l_type;
    return std::any();
}

std::any LocalChecker::visit(Expr::Logical* expr, bool as_lvalue) {
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            expr->op->location,
            "Logical expression cannot be an lvalue."
        );
        return std::any();
    }

    bool has_error = false;

    auto l_type = expr_check(expr->left, false);
    if (!l_type)
        has_error = true;
    else if (!PTR_INSTANCEOF(l_type, Type::Bool)) {
        Logger::inst().log_error(
            Err::NoOperatorOverload,
            expr->op->location,
            "Left operand must be of type `bool`."
        );
        has_error = true;
    }

    auto r_type = expr_check(expr->right, false);
    if (!r_type)
        has_error = true;
    else if (!PTR_INSTANCEOF(r_type, Type::Bool)) {
        Logger::inst().log_error(
            Err::NoOperatorOverload,
            expr->op->location,
            "Right operand must be of type `bool`."
        );
        has_error = true;
    }

    if (has_error)
        return std::any();

    expr->type = expr->left->type; // The result type is `Bool`.
    return std::any();
}

std::any LocalChecker::visit(Expr::Binary* expr, bool as_lvalue) {
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            expr->op->location,
            "Binary expression cannot be an lvalue."
        );
        return std::any();
    }

    bool has_error = false;

    auto l_type = expr_check(expr->left, false);
    if (!l_type)
        has_error = true;

    auto r_type = expr_check(expr->right, false);
    if (!r_type)
        has_error = true;

    if (has_error)
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
                std::string("Type `") + r_type->to_string() +
                    "` is not compatible with type `" + l_type->to_string() +
                    "`."
            );
            return std::any();
        }
        // Types must inherit from `Type::INumeric`.
        if (!PTR_INSTANCEOF(l_type, Type::INumeric)) {
            Logger::inst().log_error(
                Err::NoOperatorOverload,
                expr->op->location,
                "Operands must be of a numeric type."
            );
            return std::any();
        }
        expr->type = l_type; // The result type is the same as the operand type.
        break;
    default:
        panic(
            "LocalChecker::visit(Expr::Binary*): Could not handle case for "
            "operator of token type " +
            std::to_string(static_cast<int>(expr->op->tok_type))
        );
    }
    return std::any();
}

std::any LocalChecker::visit(Expr::Unary* expr, bool as_lvalue) {
    // An unary expression should never be an lvalue.
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            expr->op->location,
            "Unary expression cannot be an lvalue."
        );
        return std::any();
    }

    auto r_type = expr_check(expr->right, false);
    if (!r_type)
        return std::any();

    switch (expr->op->tok_type) {
    case Tok::Minus:
        // Types must inherit from `Type::INumeric`.
        if (!PTR_INSTANCEOF(r_type, Type::INumeric)) {
            Logger::inst().log_error(
                Err::NoOperatorOverload,
                expr->op->location,
                "Operand must be of a numeric type."
            );
            return std::any();
        }
        expr->type = r_type;
        return std::any();
    case Tok::KwNot:
        if (!PTR_INSTANCEOF(r_type, Type::Bool)) {
            Logger::inst().log_error(
                Err::NoOperatorOverload,
                expr->op->location,
                "Operand must be of type `bool`."
            );
            return std::any();
        }
        expr->type = r_type; // The result type is `Bool`.
        return std::any();
    default:
        panic(
            "LocalChecker::visit(Expr::Unary*): Could not handle case for "
            "operator of token type " +
            std::to_string(static_cast<int>(expr->op->tok_type))
        );
        return std::any();
    }
}

std::any LocalChecker::visit(Expr::Deref* expr, bool as_lvalue) {
    // TODO: Implement dereference expressions.
    panic("LocalChecker::visit(Expr::Deref*): Not implemented yet.");
    return std::any();
}

std::any LocalChecker::visit(Expr::Access* expr, bool as_lvalue) {
    auto l_type = expr_check(expr->left, true);
    auto l_lvalue = std::dynamic_pointer_cast<Expr::IPLValue>(expr->left);
    if (!l_type || !l_lvalue)
        return std::any();

    if (auto tuple_l_type = std::dynamic_pointer_cast<Type::Tuple>(l_type)) {
        if (expr->right_token->tok_type == Tok::Int) {
            size_t index = std::any_cast<size_t>(expr->right_token->literal);
            if (index >= tuple_l_type->elements.size()) {
                Logger::inst().log_error(
                    Err::TupleIndexOutOfBounds,
                    expr->right_token->location,
                    "Tuple index " + std::to_string(index) +
                        " is out of bounds for tuple of size " +
                        std::to_string(tuple_l_type->elements.size()) + "."
                );
                Logger::inst().log_note(
                    *expr->left->location,
                    "Expression has type `" + l_type->to_string() + "`."
                );
                return std::any();
            }
            expr->type = tuple_l_type->elements[index];

            // For tuple access, the assignability and possible error location
            // is carried over from the left side.
            expr->assignable = l_lvalue->assignable;
            expr->error_location = l_lvalue->error_location;

            return std::any();
        }
        else {
            Logger::inst().log_error(
                Err::InvalidTupleAccess,
                expr->right_token->location,
                "Tuple can only be accessed with an integer literal."
            );
            return std::any();
        }
    }
    else {
        Logger::inst().log_error(
            Err::OperatorNotValidForExpr,
            *expr->left->location,
            "Dot operator is not valid for this kind of expression."
        );
        return std::any();
    }
}

std::any LocalChecker::visit(Expr::NameRef* expr, bool as_lvalue) {
    auto node = symbol_tree->search_name(expr->name);

    if (!node) {
        Logger::inst().log_error(
            Err::UndeclaredName,
            expr->name.parts.back().token->location,
            "Name `" + expr->name.to_string() + "` was not declared."
        );
        return std::any();
    }
    if (!PTR_INSTANCEOF(*node, Node::FieldEntry)) {
        Logger::inst().log_error(
            Err::NotAVariable,
            expr->name.parts.back().token->location,
            "Name reference `" + expr->name.to_string() + "` is not a variable."
        );
        return std::any();
    }
    auto field_entry = std::dynamic_pointer_cast<Node::FieldEntry>(*node);
    // Set assignability and possible error location based on whether the
    // field is mutable.
    if (field_entry->field.is_var) {
        expr->assignable = true;
    }
    else {
        expr->assignable = false;
        expr->error_location = &field_entry->field.token->location;
    }

    expr->type = field_entry->field.type;
    expr->field_entry = field_entry;
    return std::any();
}

std::any LocalChecker::visit(Expr::Literal* expr, bool as_lvalue) {
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            *expr->location,
            "Literal expression cannot be an lvalue."
        );
        return std::any();
    }
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
        panic(
            "LocalChecker::visit(Expr::Literal*): Could not handle case "
            "for "
            "token type " +
            std::to_string(static_cast<int>(expr->token->tok_type))
        );
    }
    return std::any();
}

std::any LocalChecker::visit(Expr::Tuple* expr, bool as_lvalue) {
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            *expr->location,
            "Tuple expression cannot be an lvalue."
        );
        return std::any();
    }
    std::vector<std::shared_ptr<Type>> element_types;
    bool has_error = false;

    for (auto& element : expr->elements) {
        expr_check(element, false);
        if (!element->type)
            has_error = true;
        element_types.push_back(element->type);
    }
    if (has_error)
        return std::any();
    expr->type = std::make_shared<Type::Tuple>(element_types);
    return std::any();
}

std::any LocalChecker::visit(Expr::Block* expr, bool as_lvalue) {
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            *expr->location,
            "Block expression cannot be an lvalue."
        );
        return std::any();
    }
    auto [local_scope, err] = symbol_tree->add_local_scope(expr->kind);
    if (err != Err::Null) {
        panic("LocalChecker::visit(Expr::Block*): Could not add local scope.");
    }
    for (auto& stmt : expr->statements) {
        stmt->accept(this);
    }
    auto yield_type =
        local_scope->yield_type.value_or(std::make_shared<Type::Unit>());
    expr->type = yield_type;
    symbol_tree->exit_scope();

    return std::any();
}

std::any LocalChecker::visit(Expr::Conditional* expr, bool as_lvalue) {
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            *expr->location,
            "Conditional expression cannot be an lvalue."
        );
        return std::any();
    }
    // We use this flag to try and report as many errors as possible before
    // returning.
    bool has_error = false;

    // Check the condition.
    auto cond_type = expr_check(expr->condition, false);
    if (!cond_type)
        has_error = true;
    // The condition must be of type `bool`.
    if (!has_error && !PTR_INSTANCEOF(cond_type, Type::Bool)) {
        Logger::inst().log_error(
            Err::ConditionNotBool,
            *expr->condition->location,
            "Condition expression must be of type `bool`, not `" +
                cond_type->to_string() + "`."
        );
        has_error = true;
    }

    // Check the branches.
    auto then_type = expr_check(expr->then_branch, false);
    if (!then_type)
        has_error = true;
    auto else_type = expr_check(expr->else_branch, false);
    if (!else_type)
        has_error = true;
    // If there was an error in any of the branches, return early.
    if (has_error)
        return std::any();

    // If all branches have the same type, use that type.
    if (*then_type != *else_type) {
        Logger::inst().log_error(
            Err::ConditionalBranchTypeMismatch,
            *expr->location,
            "Yielded expression types of conditional branches do not match."
        );
        Logger::inst().log_note(
            *expr->then_branch->location,
            "Then branch has type `" + then_type->to_string() + "`."
        );
        if (expr->implicit_else) {
            Logger::inst().log_note(
                "Implicit else branch yields a unit value with type `()`."
            );
        }
        else {
            Logger::inst().log_note(
                *expr->else_branch->location,
                "Else branch has type `" + else_type->to_string() + "`."
            );
        }
        has_error = true;
    }

    if (!has_error) {
        expr->type = then_type;
    }

    return std::any();
}

std::any LocalChecker::visit(Expr::Loop* expr, bool as_lvalue) {
    // TODO: Implement loop expressions.
    panic("LocalChecker::visit(Expr::Loop*): Not implemented yet.");
    return std::any();
}

// MARK: Annotations

std::any LocalChecker::visit(Annotation::NameRef* annotation) {
    std::shared_ptr<Type> type = nullptr;
    // Temporary solution: only allow primitive types.
    if (annotation->name.to_string() == "i32") {
        type = std::make_shared<Type::Int>(true, 32);
    }
    else if (annotation->name.to_string() == "f64") {
        type = std::make_shared<Type::Float>(64);
    }
    else if (annotation->name.to_string() == "bool") {
        type = std::make_shared<Type::Bool>();
    }
    else if (annotation->name.to_string() == "str") {
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
    std::shared_ptr<Type> type = nullptr;
    std::vector<std::shared_ptr<Type>> element_types;
    for (const auto& element : annotation->elements) {
        auto elem_any = element->accept(this);
        if (!elem_any.has_value())
            return std::any();
        element_types.push_back(std::any_cast<std::shared_ptr<Type>>(elem_any));
    }
    type = std::make_shared<Type::Tuple>(element_types);
    return type;
}

void LocalChecker::run_check(std::unique_ptr<FrontendContext>& context) {
    symbol_tree->clear_modified();

    for (size_t i = context->stmts_processed; i < context->stmts.size(); ++i) {
        context->stmts[i]->accept(this);
    }

    if (Logger::inst().get_errors().empty()) {
        context->status = Status::Ok();
    }
    else if (repl_mode) {
        if (symbol_tree->was_modified()) {
            context->status = Status::Pause(Request::DiscardWarn);
        }
        else {
            context->status = Status::Pause(Request::Discard);
        }
        context->rollback();
    }
    else {
        context->status = Status::Error();
    }
}

void LocalChecker::check(
    std::unique_ptr<FrontendContext>& context, bool repl_mode
) {
    if (IS_VARIANT(context->status, Status::Error)) {
        panic("LocalChecker::check: Context is already in an error state.");
    }

    LocalChecker checker(context->symbol_tree, repl_mode);
    checker.run_check(context);
}

} // namespace nico
