#include "nico/frontend/components/local_checker.h"

#include <unordered_set>

#include "nico/shared/error_code.h"
#include "nico/shared/logger.h"
#include "nico/shared/status.h"
#include "nico/shared/utils.h"

namespace nico {

std::shared_ptr<Type>
LocalChecker::expr_check(std::shared_ptr<Expr> expr, bool as_lvalue) {
    expr->accept(this, as_lvalue);
    if (!expr->type)
        return nullptr;
    if (!expr->type->is_sized_type() && !as_lvalue) {
        Logger::inst().log_error(
            Err::UnsizedRValue,
            expr->location,
            "Unsized type `" + expr->type->to_string() +
                "` cannot be used as an rvalue."
        );
        return nullptr;
    }
    return expr->type;
}

bool LocalChecker::is_in_unsafe_context() {
    auto local_scope =
        std::dynamic_pointer_cast<Node::LocalScope>(symbol_tree->current_scope);
    return local_scope && local_scope->block->is_unsafe;
}

std::shared_ptr<Type>
LocalChecker::implicit_full_dereference(std::shared_ptr<Expr>& expr) {
    if (!expr->type)
        panic(
            "LocalChecker::implicit_full_dereference: Expression has no type. "
            "Was it type checked?"
        );

    auto initial_type = expr->type;

    while (auto i_pointer_type =
               std::dynamic_pointer_cast<Type::IPointer>(expr->type)) {
        if (PTR_INSTANCEOF(i_pointer_type, Type::Nullptr)) {
            panic(
                "LocalChecker::implicit_full_dereference: Called on a nullptr "
                "pointer unexpectedly."
            );
        }
        if (PTR_INSTANCEOF(i_pointer_type, Type::RawPointer)) {
            if (!is_in_unsafe_context()) {
                Logger::inst().log_error(
                    Err::PtrDerefOutsideUnsafeBlock,
                    expr->location,
                    "Cannot implicitly dereference `" +
                        initial_type->to_string() +
                        "` outside of unsafe context."
                );
                return nullptr;
            }
        }
        expr = std::make_shared<Expr::Deref>(
            std::make_shared<Token>(Tok::Star, *expr->location),
            expr
        );
        expr->type = i_pointer_type->base;
    }

    return expr->type;
}

bool LocalChecker::check_pointer_cast(
    std::shared_ptr<Type> expr_type,
    std::shared_ptr<Type> target_type,
    std::shared_ptr<Token> as_token
) {
    auto expr_ptr_type = std::dynamic_pointer_cast<Type::IPointer>(expr_type);
    auto target_ptr_type =
        std::dynamic_pointer_cast<Type::IPointer>(target_type);
    if (!expr_ptr_type || !target_ptr_type) {
        panic(
            "LocalChecker::check_pointer_cast: Both types must be pointer "
            "types."
        );
    }

    // Beyond this point, both types must be raw pointer types.
    auto expr_raw_ptr_type =
        std::dynamic_pointer_cast<Type::RawPointer>(expr_ptr_type);
    auto target_raw_ptr_type =
        std::dynamic_pointer_cast<Type::RawPointer>(target_ptr_type);
    if (!expr_raw_ptr_type || !target_raw_ptr_type) {
        Logger::inst().log_error(
            Err::InvalidCastOperation,
            as_token->location,
            "Invalid pointer cast from `" + expr_type->to_string() + "` to `" +
                target_type->to_string() + "`."
        );
        return false;
    }

    // Nullptr cast.
    if (PTR_INSTANCEOF(expr_raw_ptr_type, Type::Nullptr)) {
        // Nullptr can be cast to any raw pointer type.
        return true;
    }

    // Multi-level pointer cast.
    if (PTR_INSTANCEOF(expr_raw_ptr_type->base, Type::IPointer) ||
        PTR_INSTANCEOF(target_raw_ptr_type->base, Type::IPointer)) {
        if (!PTR_INSTANCEOF(target_raw_ptr_type->base, Type::IPointer)) {
            Logger::inst().log_error(
                Err::InvalidCastOperation,
                as_token->location,
                "Cannot cast pointer type `" +
                    expr_raw_ptr_type->base->to_string() +
                    "` to non-pointer "
                    "type `" +
                    target_raw_ptr_type->base->to_string() + "`."
            );
            return false;
        }
        else if (!PTR_INSTANCEOF(expr_raw_ptr_type->base, Type::IPointer)) {
            Logger::inst().log_error(
                Err::InvalidCastOperation,
                as_token->location,
                "Cannot cast non-pointer type `" +
                    expr_raw_ptr_type->base->to_string() +
                    "` to pointer "
                    "type `" +
                    target_raw_ptr_type->base->to_string() + "`."
            );
            return false;
        }

        // Recursively check the inner pointer cast.
        return check_pointer_cast(
            expr_raw_ptr_type->base,
            target_raw_ptr_type->base,
            as_token
        );
    }

    // Array pointer cast.
    if (auto target_array_type =
            std::dynamic_pointer_cast<Type::Array>(target_raw_ptr_type->base)) {
        // The base type of the expression pointer type must also be an array
        // type.
        auto expr_array_type =
            std::dynamic_pointer_cast<Type::Array>(expr_raw_ptr_type->base);
        if (!expr_array_type) {
            Logger::inst().log_error(
                Err::InvalidCastOperation,
                as_token->location,
                "Cannot cast pointer of type `" + expr_type->to_string() +
                    "` to array pointer type `" + target_type->to_string() +
                    "`."
            );
            return false;
        }
        // The element types must be the same, not just compatible.
        if (*expr_array_type->base != *target_array_type->base) {
            Logger::inst().log_error(
                Err::InvalidCastOperation,
                as_token->location,
                "Array pointer element types `" +
                    expr_array_type->base->to_string() + "` and `" +
                    target_array_type->base->to_string() +
                    "` must be equivalent."
            );
            return false;
        }
        // The target array type must be unsized for this kind of cast.
        if (target_array_type->size.has_value()) {
            Logger::inst().log_error(
                Err::InvalidCastOperation,
                as_token->location,
                "Array pointer cast is only valid when target array type is "
                "unsized; array type `" +
                    target_array_type->to_string() + "` is sized."
            );
            return false;
        }
        // Cast is OK.
        return true;
    }

    // Cast is invalid.
    Logger::inst().log_error(
        Err::InvalidCastOperation,
        as_token->location,
        "Invalid pointer cast from `" + expr_type->to_string() + "` to `" +
            target_type->to_string() + "`."
    );

    return false;
}

std::optional<Dictionary<std::string, std::weak_ptr<Expr>>>
LocalChecker::try_match_args_to_params(
    std::shared_ptr<Type::Function> func_type,
    const std::vector<std::shared_ptr<Expr>>& pos_args,
    const Dictionary<std::string, std::shared_ptr<Expr>>& named_args
) {
    Dictionary<std::string, std::weak_ptr<Expr>> arg_mapping;
    std::unordered_set<std::string> assigned_params;

    // First, apply default arguments.
    for (const auto& [param_string, param_field] : func_type->parameters) {
        arg_mapping.insert(
            param_string,
            param_field.default_expr.value_or(std::weak_ptr<Expr>())
        );
        // We insert an empty pointer if there is no default expression.
        // This ensures the dictionary has an entry for every parameter in the
        // correct order.
    }
    // Next, apply positional arguments.
    if (pos_args.size() > func_type->parameters.size()) {
        // Too many positional arguments.
        return std::nullopt;
    }
    for (size_t i = 0; i < pos_args.size(); i++) {
        const auto param_optional = func_type->parameters.get_pair_at(i);
        const auto [param_name, param_field] = *param_optional;
        if (!pos_args[i]->type->is_assignable_to(*param_field.type)) {
            // Positional argument type does not match parameter type.
            return std::nullopt;
        }
        arg_mapping.insert(param_name, pos_args[i]);
        assigned_params.insert(param_name);
    }
    // Next, apply named arguments.
    for (const auto& [arg_name, arg_expr] : named_args) {
        if (!func_type->parameters.contains(arg_name)) {
            // Named argument does not match any parameter.
            return std::nullopt;
        }
        if (!arg_expr->type->is_assignable_to(
                *func_type->parameters.at(arg_name)->type
            )) {
            // Named argument type does not match parameter type.
            return std::nullopt;
        }
        // if (assigned_params.find(arg_name) != assigned_params.end()) {
        // Parameter already assigned.
        // We *could* issue a warning here, but we'll just ignore for now.
        // }
        arg_mapping.insert(arg_name, arg_expr);
        assigned_params.insert(arg_name);
    }
    // Finally, check that all parameters have been assigned.
    for (const auto& [param_string, _] : func_type->parameters) {
        if (arg_mapping.at(param_string)->lock() == nullptr) {
            // Parameter not assigned
            return std::nullopt;
        }
    }

    return arg_mapping;
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
    else if (!stmt->has_var) {
        // If there is no initializer, the statement must have var.
        Logger::inst().log_error(
            Err::ImmutableWithoutInitializer,
            stmt->identifier->location,
            "Immutable variable `" + std::string(stmt->identifier->lexeme) +
                "` must have an initializer."
        );
        return std::any();
    }
    // If the initializer is not present, the annotation will be (this is
    // checked in the parser).

    // If the type annotation is present, check that it matches the initializer.
    if (stmt->annotation.has_value()) {
        auto anno_any = stmt->annotation.value()->accept(&annotation_checker);
        if (!anno_any.has_value())
            return std::any();
        auto annotation_type = std::any_cast<std::shared_ptr<Type>>(anno_any);
        if (expr_type != nullptr &&
            !expr_type->is_assignable_to(*annotation_type)) {
            Logger::inst().log_error(
                Err::LetTypeMismatch,
                stmt->expression.value()->location,
                std::string("Type `") + expr_type->to_string() +
                    "` is not compatible with type `" +
                    annotation_type->to_string() + "`."
            );
            return std::any();
        }
        // On occassion, two different types are compatible with each other.
        // E.g., the annotation is @i32, but the expression is nullptr.
        // In such cases, the annotated type takes precedence.
        expr_type = annotation_type;
    }

    if (!expr_type->is_sized_type()) {
        Logger::inst().log_error(
            Err::UnsizedTypeAllocation,
            stmt->identifier->location,
            "Cannot allocate variable `" +
                std::string(stmt->identifier->lexeme) + "` of unsized type `" +
                expr_type->to_string() + "`."
        );
        return std::any();
    }

    // Create the field entry.
    Field field(
        stmt->has_var,
        stmt->identifier->lexeme,
        &stmt->identifier->location,
        expr_type
    );

    auto [node, err] = symbol_tree->add_field_entry(field);
    if (err != Err::Null) {
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

std::any LocalChecker::visit(Stmt::Func* stmt) {
    // Get the function's type
    auto func_type = std::dynamic_pointer_cast<Type::Function>(
        stmt->field_entry.lock()->field.type
    );
    if (!func_type) {
        panic(
            "LocalChecker::visit(Stmt::Func*): Field entry does not have a "
            "function type."
        );
    }

    symbol_tree->add_local_scope(stmt->body);

    bool has_error = false;
    // Check the parameters.
    for (auto& param : stmt->parameters) {
        auto param_field =
            func_type->parameters.at(std::string(param.identifier->lexeme))
                .value();

        // If the parameter has a default expression, check it.
        if (param_field.default_expr.has_value()) {
            auto default_expr_ptr = param_field.default_expr.value().lock();
            auto default_expr_type = expr_check(default_expr_ptr, false);
            if (!default_expr_type) {
                has_error = true;
            }
            else if (!default_expr_type->is_assignable_to(*param_field.type)) {
                Logger::inst().log_error(
                    Err::DefaultArgTypeMismatch,
                    default_expr_ptr->location,
                    std::string("Type `") + default_expr_type->to_string() +
                        "` is not compatible with parameter type `" +
                        param_field.type->to_string() + "`."
                );
                has_error = true;
            }
        }
        auto [node, err] = symbol_tree->add_field_entry(param_field);
        if (err != Err::Null) {
            has_error = true;
        }
        else {
            param.field_entry =
                std::dynamic_pointer_cast<Node::FieldEntry>(node);
        }
    }
    // If there was an error in the parameters, avoid checking the body.
    if (has_error) {
        symbol_tree->exit_scope();
        return std::any();
    }

    // Check the body.
    auto body_type = expr_check(stmt->body, false);
    if (!body_type) {
        // Ignore error, already reported.
    }
    // Body type must be assignable to the return type.
    else if (!body_type->is_assignable_to(*func_type->return_type)) {
        Logger::inst().log_error(
            Err::FunctionReturnTypeMismatch,
            stmt->body->location,
            std::string("Function body type `") + body_type->to_string() +
                "` is not compatible with declared return type `" +
                func_type->return_type->to_string() + "`."
        );
        return std::any();
    }
    // Exit the parameter local scope.
    symbol_tree->exit_scope();

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

    stmt->target_block = local_scope->block;

    // Visit the expression in the yield statement.
    auto expr_type = expr_check(stmt->expression, false);
    if (!expr_type)
        return std::any();

    if (!local_scope->yield_type) {
        // If this local scope does not currently have a yield type...
        local_scope->yield_type = expr_type;
    }
    else if (*local_scope->yield_type.value() != *expr_type) {
        // If this local scope has a yield type, check that the new yield
        // expression is compatible with it.
        Logger::inst().log_error(
            Err::YieldTypeMismatch,
            stmt->expression->location,
            std::string("Type `") + stmt->expression->type->to_string() +
                "` is not compatible with previously yielded type `" +
                local_scope->yield_type.value()->to_string() + "`."
        );
    }

    return std::any();
}

std::any LocalChecker::visit(Stmt::Continue* stmt) {
    auto target_scope =
        symbol_tree->get_local_scope_of_kind(Expr::Block::Kind::Loop);
    if (!target_scope) {
        Logger::inst().log_error(
            Err::ContinueOutsideLoop,
            stmt->continue_token->location,
            "Cannot use continue outside of a loop."
        );
        return std::any();
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

std::any LocalChecker::visit(Stmt::Dealloc* stmt) {
    auto expr_type = expr_check(stmt->expression, false);
    if (!expr_type)
        return std::any();

    if (!PTR_INSTANCEOF(expr_type, Type::RawPointer)) {
        Logger::inst().log_error(
            Err::DeallocNonRawPointer,
            stmt->expression->location,
            "Cannot deallocate non-raw pointer type `" +
                expr_type->to_string() + "`."
        );
        return std::any();
    }
    else if (PTR_INSTANCEOF(expr_type, Type::Nullptr)) {
        Logger::inst().log_error(
            Err::DeallocNullptr,
            stmt->expression->location,
            "Cannot deallocate pointer of nullptr type."
        );
        return std::any();
    }
    else if (!is_in_unsafe_context()) {
        Logger::inst().log_error(
            Err::DeallocOutsideUnsafeBlock,
            stmt->expression->location,
            "Cannot deallocate outside of unsafe context."
        );
        return std::any();
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
            expr->left->location,
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
    // if (*l_type != *r_type) {
    if (!r_type->is_assignable_to(*l_type)) {
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
    case Tok::Percent:
        // Types must inherit from `Type::INumeric`.
        if (!PTR_INSTANCEOF(l_type, Type::INumeric)) {
            Logger::inst().log_error(
                Err::NoOperatorOverload,
                expr->op->location,
                "Operands must be of a numeric type."
            );
            return std::any();
        }
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
        expr->type = l_type; // The result type is the same as the operand type.
        break;
    case Tok::EqEq:
    case Tok::BangEq:
        // Types must inherit from `Type::INumeric` or be `Type::Bool` or
        // `Type::Pointer`.
        if (!PTR_INSTANCEOF(l_type, Type::INumeric) &&
            !PTR_INSTANCEOF(l_type, Type::Bool) &&
            !PTR_INSTANCEOF(l_type, Type::RawPointer)) {
            Logger::inst().log_error(
                Err::NoOperatorOverload,
                expr->op->location,
                "Operands must be of a numeric, boolean, or pointer type."
            );
            return std::any();
        }
        // Both operands must be of the same type, OR both operands must be
        // pointers.
        // NOT (A == B OR (isptr(A) AND isptr(B)))
        if (!(*l_type == *r_type ||
              (PTR_INSTANCEOF(l_type, Type::RawPointer) &&
               PTR_INSTANCEOF(r_type, Type::RawPointer)))) {
            Logger::inst().log_error(
                Err::NoOperatorOverload,
                expr->op->location,
                std::string("Type `") + r_type->to_string() +
                    "` is not compatible with type `" + l_type->to_string() +
                    "`."
            );
            return std::any();
        }
        expr->type = std::make_shared<Type::Bool>(); // The result type is Bool.
        break;
    case Tok::Gt:
    case Tok::GtEq:
    case Tok::Lt:
    case Tok::LtEq:
        // Types must inherit from `Type::INumeric`.
        if (!PTR_INSTANCEOF(l_type, Type::INumeric)) {
            Logger::inst().log_error(
                Err::NoOperatorOverload,
                expr->op->location,
                "Operands must be of a numeric type."
            );
            return std::any();
        }
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
        expr->type = std::make_shared<Type::Bool>(); // The result type is Bool.
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
    case Tok::Negative:
        // Types must inherit from `Type::INumeric`.
        if (!PTR_INSTANCEOF(r_type, Type::INumeric)) {
            Logger::inst().log_error(
                Err::NoOperatorOverload,
                expr->op->location,
                "Operand must be of a numeric type."
            );
            return std::any();
        }
        // Type cannot be an unsigned integer.
        if (auto int_type = std::dynamic_pointer_cast<Type::Int>(r_type)) {
            if (!int_type->is_signed) {
                Logger::inst().log_error(
                    Err::NegativeOnUnsignedType,
                    expr->op->location,
                    "Cannot use unary '-' on unsigned integer type."
                );
                return std::any();
            }
        }
        expr->type = r_type;
        return std::any();
    case Tok::KwNot:
    case Tok::Bang:
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

std::any LocalChecker::visit(Expr::Address* expr, bool as_lvalue) {
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            expr->op->location,
            "Address-of expression cannot be an lvalue."
        );
        return std::any();
    }

    auto r_type = expr_check(expr->right, true);
    auto r_lvalue = std::dynamic_pointer_cast<Expr::IPLValue>(expr->right);
    if (!r_type || !r_lvalue)
        return std::any();

    if (expr->op->tok_type == Tok::At) {
        expr->type = std::make_shared<Type::RawPointer>(r_type, expr->has_var);
    }
    else if (expr->op->tok_type == Tok::Amp) {
        // expr->type = std::make_shared<Type::Reference>(r_type,
        // expr->has_var);

        // Note: References have a lot of rules that we can't enforce yet, so we
        // disallow them for now.
        panic(
            "LocalChecker::visit(Expr::Address*): References are not supported "
            "yet."
        );
    }
    else {
        panic(
            "LocalChecker::visit(Expr::Address*): Unknown address-of operator."
        );
    }

    if (expr->has_var && !r_lvalue->assignable) {
        Logger::inst().log_error(
            Err::AddressOfImmutable,
            expr->op->location,
            "Cannot create a mutable pointer/reference to an immutable value."
        );
        if (r_lvalue->error_location) {
            Logger::inst().log_note(
                *r_lvalue->error_location,
                "This is not mutable."
            );
        }
        return std::any();
    }

    return std::any();
}

std::any LocalChecker::visit(Expr::Deref* expr, bool as_lvalue) {
    // Dereference expressions *are* possible lvalues.
    auto r_type = expr_check(expr->right, false);
    if (!r_type)
        return std::any();

    if (PTR_INSTANCEOF(r_type, Type::Nullptr)) {
        Logger::inst().log_error(
            Err::DereferenceNullptr,
            expr->op->location,
            "Cannot dereference a pointer of type `nullptr`."
        );
        return std::any();
    }
    if (auto ptr_type = std::dynamic_pointer_cast<Type::RawPointer>(r_type)) {
        expr->type = ptr_type->base;
        // Remember: pointers are not possible lvalues.
        // For pointer dereference, the assignability is carried over from the
        // mutability of the pointer.
        expr->assignable = ptr_type->is_mutable;
        expr->error_location = expr->right->location;

        if (!is_in_unsafe_context()) {
            Logger::inst().log_error(
                Err::PtrDerefOutsideUnsafeBlock,
                expr->op->location,
                "Cannot dereference raw pointer outside of an unsafe block."
            );
            return std::any();
        }

        return std::any();
    }
    else {
        Logger::inst().log_error(
            Err::DereferenceNonPointer,
            expr->op->location,
            "Dereference operator is not valid for this kind of expression."
        );
        return std::any();
    }
}

std::any LocalChecker::visit(Expr::Cast* expr, bool as_lvalue) {
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            expr->as_token->location,
            "Cast expression cannot be an lvalue."
        );
        return std::any();
    }

    auto expr_type = expr_check(expr->expression, false);
    if (!expr_type)
        return std::any();

    auto anno_any = expr->annotation->accept(&annotation_checker);
    if (!anno_any.has_value())
        return std::any();
    auto target_type = std::any_cast<std::shared_ptr<Type>>(anno_any);
    expr->target_type = target_type;

    // Check that the cast is valid.
    if (*expr_type == *target_type) {
        // The types are the same; no cast needed.
        // We *could* emit a warning here, but we'll just allow it for now.
        expr->operation = Expr::Cast::Operation::NoOp;
    }
    else if (PTR_INSTANCEOF(expr_type, Type::IPointer) &&
             PTR_INSTANCEOF(target_type, Type::IPointer)) {
        // Pointer cast
        if (!check_pointer_cast(expr_type, target_type, expr->as_token)) {
            return std::any();
        }
        // A pointer cast is a NoOp cast.
        expr->operation = Expr::Cast::Operation::NoOp;
    }
    else if (PTR_INSTANCEOF(target_type, Type::Bool)) {
        auto target_bool_type =
            std::dynamic_pointer_cast<Type::Bool>(target_type);
        // Could be IntToBool, FPToBool
        if (PTR_INSTANCEOF(expr_type, Type::Int)) {
            // Must be IntToBool
            expr->operation = Expr::Cast::Operation::IntToBool;
        }
        else if (PTR_INSTANCEOF(expr_type, Type::Float)) {
            // Must be FPToBool
            expr->operation = Expr::Cast::Operation::FPToBool;
        }
    }
    else if (PTR_INSTANCEOF(expr_type, Type::Int)) {
        auto expr_int_type = std::dynamic_pointer_cast<Type::Int>(expr_type);
        // Could be SignExt, ZeroExt, IntTrunc, SIntToFP, UIntToFP
        if (PTR_INSTANCEOF(target_type, Type::Int)) {
            auto target_int_type =
                std::dynamic_pointer_cast<Type::Int>(target_type);
            // Could be SignExt, ZeroExt, IntTrunc
            if (expr_int_type->width < target_int_type->width) {
                // Could be SignExt or ZeroExt
                if (target_int_type->is_signed && expr_int_type->is_signed) {
                    // Must be SignExt
                    expr->operation = Expr::Cast::Operation::SignExt;
                }
                else {
                    // Must be ZeroExt
                    expr->operation = Expr::Cast::Operation::ZeroExt;
                }
            }
            else if (expr_int_type->width > target_int_type->width) {
                // Must be IntTrunc
                expr->operation = Expr::Cast::Operation::IntTrunc;
            }
            else {
                // Same bit width but different signedness; NoOp
                expr->operation = Expr::Cast::Operation::NoOp;
            }
        }
        else if (PTR_INSTANCEOF(target_type, Type::Float)) {
            auto target_float_type =
                std::dynamic_pointer_cast<Type::Float>(target_type);
            // Could be SIntToFP or UIntToFP
            if (expr_int_type->is_signed) {
                // Must be SIntToFP
                expr->operation = Expr::Cast::Operation::SIntToFP;
            }
            else {
                // Must be UIntToFP
                expr->operation = Expr::Cast::Operation::UIntToFP;
            }
        }
    }
    else if (PTR_INSTANCEOF(expr_type, Type::Float)) {
        auto expr_float_type =
            std::dynamic_pointer_cast<Type::Float>(expr_type);
        // Could be FPExt, FPTrunc, FPToSInt, FPToUInt
        if (PTR_INSTANCEOF(target_type, Type::Float)) {
            auto target_float_type =
                std::dynamic_pointer_cast<Type::Float>(target_type);
            // Could be FPExt or FPTrunc
            if (expr_float_type->width < target_float_type->width) {
                // Must be FPExt
                expr->operation = Expr::Cast::Operation::FPExt;
            }
            else {
                // Must be FPTrunc
                expr->operation = Expr::Cast::Operation::FPTrunc;
            }
        }
        else if (PTR_INSTANCEOF(target_type, Type::Int)) {
            auto target_int_type =
                std::dynamic_pointer_cast<Type::Int>(target_type);
            // Could be FPToSInt or FPToUInt
            if (target_int_type->is_signed) {
                // Must be FPToSInt
                expr->operation = Expr::Cast::Operation::FPToSInt;
            }
            else {
                // Must be FPToUInt
                expr->operation = Expr::Cast::Operation::FPToUInt;
            }
        }
    }

    if (expr->operation == Expr::Cast::Operation::Null) {
        // No possible cast operation was found.
        Logger::inst().log_error(
            Err::InvalidCastOperation,
            expr->as_token->location,
            std::string("Cannot cast from type `") + expr_type->to_string() +
                "` to type `" + target_type->to_string() + "`."
        );
        return std::any();
    }

    expr->type = target_type;
    return std::any();
}

std::any LocalChecker::visit(Expr::Access* expr, bool as_lvalue) {
    if (!expr_check(expr->left, true))
        return std::any();
    auto l_type = implicit_full_dereference(expr->left);
    auto l_lvalue = std::dynamic_pointer_cast<Expr::IPLValue>(expr->left);
    if (!l_type || !l_lvalue)
        return std::any();

    if (!l_type->is_sized_type()) {
        Logger::inst().log_error(
            Err::UnsizedTypeMemberAccess,
            expr->left->location,
            "Cannot access members of unsized type `" + l_type->to_string() +
                "`."
        );
        Logger::inst().log_note(
            "Aggregate type members must be sized to calculate member offsets."
        );
        return std::any();
    }

    if (auto tuple_l_type = std::dynamic_pointer_cast<Type::Tuple>(l_type)) {
        if (expr->right_token->tok_type == Tok::TupleIndex) {
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
                    expr->left->location,
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
            expr->left->location,
            "Dot operator is not valid for this kind of expression."
        );
        return std::any();
    }
}

std::any LocalChecker::visit(Expr::Subscript* expr, bool as_lvalue) {
    if (!expr_check(expr->left, true))
        return std::any();
    auto l_type = implicit_full_dereference(expr->left);
    auto l_lvalue = std::dynamic_pointer_cast<Expr::IPLValue>(expr->left);
    if (!l_type || !l_lvalue)
        return std::any();

    auto index_type = expr_check(expr->index, false);
    if (!index_type)
        return std::any();

    if (auto array_l_type = std::dynamic_pointer_cast<Type::Array>(l_type)) {
        // Array element type must be a sized type.
        if (!array_l_type->base->is_sized_type()) {
            Logger::inst().log_error(
                Err::UnsizedTypeArrayAccess,
                expr->left->location,
                "Cannot access array elements of unsized type `" +
                    array_l_type->base->to_string() + "`."
            );
            Logger::inst().log_note(
                "Array element types must be sized to calculate element "
                "offsets."
            );
            return std::any();
        }
        // Index must be an integer type.
        if (!PTR_INSTANCEOF(index_type, Type::Int)) {
            Logger::inst().log_error(
                Err::ArrayIndexNotInteger,
                expr->index->location,
                "Array index must be of an integer type."
            );
            return std::any();
        }
        expr->type = array_l_type->base;

        // For array subscript, the assignability and possible error location
        // is carried over from the left side.
        expr->assignable = l_lvalue->assignable;
        expr->error_location = l_lvalue->error_location;
    }
    else {
        Logger::inst().log_error(
            Err::OperatorNotValidForExpr,
            expr->left->location,
            "Subscript operator is not valid for this kind of expression."
        );
    }

    return std::any();
}

std::any LocalChecker::visit(Expr::Call* expr, bool as_lvalue) {
    std::vector<std::shared_ptr<Type::Function>> candidate_funcs;
    std::vector<std::shared_ptr<Node::FieldEntry>> overload_field_entries;

    auto callee_type = expr_check(expr->callee, false);
    if (!callee_type)
        return std::any();
    // If the callee is an exact function, add it to the candidate list.
    if (auto func_type =
            std::dynamic_pointer_cast<Type::Function>(callee_type)) {
        candidate_funcs.push_back(func_type);
    }
    // If the callee is an overloaded function, add all overloads to the
    // candidate list.
    else if (auto overload_type =
                 std::dynamic_pointer_cast<Type::OverloadedFn>(callee_type)) {
        for (auto overload : overload_type->overload_group.lock()->overloads) {
            auto func_type =
                std::dynamic_pointer_cast<Type::Function>(overload->field.type);
            candidate_funcs.push_back(func_type);
            overload_field_entries.push_back(overload);
        }
    }
    else {
        Logger::inst().log_error(
            Err::NotACallable,
            expr->callee->location,
            "Callee expression is not callable."
        );
        Logger::inst().log_note(expr->location, "Call occurs here.");
        return std::any();
    }

    // Check each argument once
    bool has_error = false;
    for (auto& arg : expr->provided_pos_args) {
        has_error |= expr_check(arg, false) == nullptr;
    }
    for (auto& [_, arg] : expr->provided_named_args) {
        has_error |= expr_check(arg, false) == nullptr;
    }
    if (has_error)
        return std::any();
    // Don't worry about the types right now; we'll check them during matching.

    std::optional<Dictionary<std::string, std::weak_ptr<Expr>>> matched_args;
    std::vector<size_t> matched_candidate_indices;
    for (size_t i = 0; i < candidate_funcs.size(); i++) {
        // Try to match the arguments to the parameters.
        auto match_attempt = try_match_args_to_params(
            candidate_funcs[i],
            expr->provided_pos_args,
            expr->provided_named_args
        );

        if (match_attempt.has_value()) {
            matched_candidate_indices.push_back(i);
            matched_args = match_attempt;
        }
    }

    if (matched_candidate_indices.size() == 1) {
        // Exactly one candidate matched.
        auto matched_func = candidate_funcs[matched_candidate_indices[0]];
        expr->type = matched_func->return_type;
        expr->actual_args = matched_args.value();
        // If the callee is an overloaded function...
        if (!overload_field_entries.empty()) {
            expr->callee->type = matched_func;
            auto name_ref =
                std::dynamic_pointer_cast<Expr::NameRef>(expr->callee);
            // Due to the semantics of the overloadedfn type, the callee must be
            // a name reference.
            if (name_ref) {
                name_ref->field_entry =
                    overload_field_entries[matched_candidate_indices[0]];
            }
        }
        return std::any();
    }
    else if (matched_candidate_indices.size() > 1) {
        // More than one candidate matched.
        Logger::inst().log_error(
            Err::MultipleMatchingFunctionOverloads,
            expr->callee->location,
            "Function call matched multiple overloads."
        );
        std::string note_msg = "Matched candidates:\n";
        for (auto index : matched_candidate_indices) {
            note_msg += " - " + candidate_funcs[index]->to_string() + "\n";
        }
        Logger::inst().log_note(note_msg);
        return std::any();
    }
    else {
        // No candidates matched.
        Logger::inst().log_error(
            Err::NoMatchingFunctionOverload,
            expr->callee->location,
            "No matching function overload found for the provided arguments."
        );
        std::string note_msg = "Possible candidates:\n";
        for (auto& candidate_func : candidate_funcs) {
            note_msg += " - " + candidate_func->to_string() + "\n";
        }
        Logger::inst().log_note(note_msg);
        return std::any();
    }

    return std::any();
}

std::any LocalChecker::visit(Expr::SizeOf* expr, bool as_lvalue) {
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            expr->location,
            "Sizeof expression cannot be an lvalue."
        );
        return std::any();
    }

    auto type_any = expr->annotation->accept(&annotation_checker);
    if (!type_any.has_value())
        return std::any();
    auto type = std::any_cast<std::shared_ptr<Type>>(type_any);
    if (!type->is_sized_type()) {
        Logger::inst().log_error(
            Err::SizeOfUnsizedType,
            expr->location,
            "Cannot measure size of unsized type `" + type->to_string() + "`."
        );
        return std::any();
    }
    expr->inner_type = type;
    expr->type = std::make_shared<Type::Int>(false, 64); // Sizeof returns u64.

    return std::any();
}

std::any LocalChecker::visit(Expr::Alloc* expr, bool as_lvalue) {
    // As a reminder: pointers are not possible lvalues.
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            expr->location,
            "Alloc expression cannot be an lvalue."
        );
        return std::any();
    }
    // You can still do `(alloc expr).property` since it contains a dereference.

    std::shared_ptr<Type> alloc_type = nullptr;

    if (expr->amount_expr.has_value()) {
        // `alloc for <amount_expr> of <type_annotation>`
        auto amount_type = expr_check(expr->amount_expr.value(), false);
        if (!amount_type)
            return std::any();
        if (!PTR_INSTANCEOF(amount_type, Type::Int)) {
            Logger::inst().log_error(
                Err::AllocAmountNotInteger,
                expr->amount_expr.value()->location,
                "Amount expression for alloc must be of an integer type."
            );
            return std::any();
        }
        auto anno_any =
            expr->type_annotation.value()->accept(&annotation_checker);
        if (!anno_any.has_value())
            return std::any();
        auto alloc_inner_type = std::any_cast<std::shared_ptr<Type>>(anno_any);
        // alloc inner type must be sized.
        if (!alloc_inner_type->is_sized_type()) {
            Logger::inst().log_error(
                Err::UnsizedTypeAllocation,
                expr->type_annotation.value()->location,
                "Cannot allocate memory for unsized type `" +
                    alloc_inner_type->to_string() + "`."
            );
            Logger::inst().log_note(
                "Allocated types must be sized to calculate memory layout."
            );
            return std::any();
        }

        alloc_type = std::make_shared<Type::Array>(alloc_inner_type);
    }
    else if (!expr->type_annotation.has_value()) {
        // `alloc with <init_expr>`
        auto init_type = expr_check(expr->expression.value(), false);
        if (!init_type)
            return std::any();
        alloc_type = init_type;
    }
    else {
        // `alloc <type_annotation> [with <init_expr>]`
        auto anno_any =
            expr->type_annotation.value()->accept(&annotation_checker);
        if (!anno_any.has_value())
            return std::any();
        auto alloc_inner_type = std::any_cast<std::shared_ptr<Type>>(anno_any);
        // alloc inner type must be sized.
        if (!alloc_inner_type->is_sized_type()) {
            Logger::inst().log_error(
                Err::UnsizedTypeAllocation,
                expr->type_annotation.value()->location,
                "Cannot allocate memory for unsized type `" +
                    alloc_inner_type->to_string() + "`."
            );
            Logger::inst().log_note(
                "Allocated types must be sized to calculate memory layout."
            );
            return std::any();
        }

        if (expr->expression.has_value()) {
            auto init_type = expr_check(expr->expression.value(), false);
            if (!init_type)
                return std::any();
            if (!init_type->is_assignable_to(*alloc_inner_type)) {
                Logger::inst().log_error(
                    Err::AllocInitTypeMismatch,
                    expr->expression.value()->location,
                    std::string("Initializer expression of type `") +
                        init_type->to_string() +
                        "` is not compatible with allocated type `" +
                        alloc_inner_type->to_string() + "`."
                );
                return std::any();
            }
            // alloc_inner_type takes precedence over init_type, so we don't do
            // anything else with init_type here.
        }

        alloc_type = alloc_inner_type;
    }

    // Alloc always returns a mutable raw pointer.
    expr->type = std::make_shared<Type::RawPointer>(alloc_type, true);
    return std::any();
}

std::any LocalChecker::visit(Expr::NameRef* expr, bool as_lvalue) {
    auto node = symbol_tree->search_name(expr->name);

    if (!node) {
        Logger::inst().log_error(
            Err::UndeclaredName,
            expr->name.parts.back().token->location,
            "Name `" + expr->name.to_string() + "` was not declared."
        );
        if (repl_mode) {
            static std::unordered_set<std::string> possible_commands = {
                "help",
                "h",
                "version",
                "license",
                "discard",
                "reset",
                "exit",
                "quit",
                "q"
            };
            auto it = possible_commands.find(expr->name.to_string());
            if (it != possible_commands.end()) {
                Logger::inst().log_note("Did you mean `:" + *it + "`?");
            }
        }
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
        expr->error_location = field_entry->field.location;
    }

    expr->type = field_entry->field.type;
    expr->field_entry = field_entry;
    return std::any();
}

std::any LocalChecker::visit(Expr::Literal* expr, bool as_lvalue) {
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            expr->location,
            "Literal expression cannot be an lvalue."
        );
        return std::any();
    }
    switch (expr->token->tok_type) {
    case Tok::Int8:
        expr->type = std::make_shared<Type::Int>(true, 8);
        break;
    case Tok::Int16:
        expr->type = std::make_shared<Type::Int>(true, 16);
        break;
    case Tok::Int32:
        expr->type = std::make_shared<Type::Int>(true, 32);
        break;
    case Tok::Int64:
        expr->type = std::make_shared<Type::Int>(true, 64);
        break;
    case Tok::UInt8:
        expr->type = std::make_shared<Type::Int>(false, 8);
        break;
    case Tok::UInt16:
        expr->type = std::make_shared<Type::Int>(false, 16);
        break;
    case Tok::UInt32:
        expr->type = std::make_shared<Type::Int>(false, 32);
        break;
    case Tok::UInt64:
        expr->type = std::make_shared<Type::Int>(false, 64);
        break;
    case Tok::Float32:
        expr->type = std::make_shared<Type::Float>(32);
        break;
    case Tok::Float64:
        expr->type = std::make_shared<Type::Float>(64);
        break;
    case Tok::Bool:
        expr->type = std::make_shared<Type::Bool>();
        break;
    case Tok::Str:
        expr->type = std::make_shared<Type::Str>();
        break;
    case Tok::Nullptr:
        expr->type = std::make_shared<Type::Nullptr>();
        break;
    default:
        panic(
            "LocalChecker::visit(Expr::Literal*): Could not handle case "
            "for token type " +
            std::to_string(static_cast<int>(expr->token->tok_type))
        );
    }
    return std::any();
}

std::any LocalChecker::visit(Expr::Tuple* expr, bool as_lvalue) {
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            expr->location,
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
    if (element_types.empty()) {
        expr->type = std::make_shared<Type::Unit>();
    }
    else {
        expr->type = std::make_shared<Type::Tuple>(element_types);
    }
    return std::any();
}

std::any LocalChecker::visit(Expr::Array* expr, bool as_lvalue) {
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            expr->location,
            "Array expression cannot be an lvalue."
        );
        return std::any();
    }
    bool has_error = false;
    if (expr->elements.empty()) {
        expr->type = std::make_shared<Type::EmptyArray>();
        return std::any();
    }
    // Visit every element once.
    for (auto& element : expr->elements) {
        expr_check(element, false);
        if (!element->type)
            has_error = true;
    }
    if (has_error)
        return std::any();

    auto first_elem_type = expr->elements[0]->type;
    // Ensure all elements have the same type.
    for (size_t i = 1; i < expr->elements.size(); i++) {
        if (*expr->elements[i]->type != *first_elem_type) {
            Logger::inst().log_error(
                Err::ArrayElementTypeMismatch,
                expr->elements[i]->location,
                "Array element type inconsistent with first element."
            );
            Logger::inst().log_note(
                "Expected type `" + first_elem_type->to_string() + "`."
            );
            has_error = true;
        }
    }
    if (has_error)
        return std::any();
    expr->type =
        std::make_shared<Type::Array>(first_elem_type, expr->elements.size());

    return std::any();
}

std::any LocalChecker::visit(Expr::Block* expr, bool as_lvalue) {
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            expr->location,
            "Block expression cannot be an lvalue."
        );
        return std::any();
    }
    auto [local_scope, err] = symbol_tree->add_local_scope(
        std::dynamic_pointer_cast<Expr::Block>(expr->shared_from_this())
    );
    if (err != Err::Null) {
        panic("LocalChecker::visit(Expr::Block*): Could not add local scope.");
    }
    for (auto& stmt : expr->statements) {
        stmt->accept(this);
        // If the statement has an error, we continue as normal.
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
            expr->location,
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
            expr->condition->location,
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
            expr->location,
            "Yielded expression types of conditional branches do not match."
        );
        Logger::inst().log_note(
            expr->then_branch->location,
            "Then branch has type `" + then_type->to_string() + "`."
        );
        if (expr->implicit_else) {
            Logger::inst().log_note(
                "Implicit else branch yields a unit value with type `()`."
            );
        }
        else {
            Logger::inst().log_note(
                expr->else_branch->location,
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
    if (as_lvalue) {
        Logger::inst().log_error(
            Err::NotAPossibleLValue,
            expr->location,
            "Loop expression cannot be an lvalue."
        );
        return std::any();
    }

    bool has_error = false;

    if (expr->condition.has_value()) {
        auto cond_type = expr_check(expr->condition.value(), false);
        if (!cond_type)
            has_error = true;
        if (!has_error && !PTR_INSTANCEOF(cond_type, Type::Bool)) {
            Logger::inst().log_error(
                Err::ConditionNotBool,
                expr->condition.value()->location,
                "Condition expression must be of type `bool`, not `" +
                    cond_type->to_string() + "`."
            );
            has_error = true;
        }
    }

    // // If this is a conditional loop that does not have a block body...
    // if (!PTR_INSTANCEOF(expr->body, Expr::Block) &&
    //     expr->condition.has_value()) {
    //     // Wrap the body in a block.
    //     expr->body = std::make_shared<Expr::Block>(
    //         expr->loop_kw,
    //         std::vector<std::shared_ptr<Stmt>>{
    //             std::make_shared<Stmt::Expression>(expr->body)
    //         },
    //         Expr::Block::Kind::Loop
    //     );
    //     // This allows users to write single-expression while loops without
    //     // restriction.
    // }

    auto body_type = expr_check(expr->body, false);
    if (!body_type)
        has_error = true;
    if (!has_error && expr->condition.has_value()) {
        // If the loop has a condition and its body does not yield unit...
        if (!PTR_INSTANCEOF(body_type, Type::Unit)) {
            Logger::inst().log_error(
                Err::WhileLoopYieldingNonUnit,
                expr->body->location,
                "Body of while loop must yield type `()`, not `" +
                    body_type->to_string() + "`."
            );
            has_error = true;
        }
    }

    if (!has_error) {
        expr->type = body_type;
    }

    return std::any();
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
