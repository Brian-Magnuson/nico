#include "nico/frontend/components/local_checker.h"

#include "nico/shared/diagnostics.h"
#include "nico/shared/error_code.h"
#include "nico/shared/status.h"
#include "nico/shared/utils.h"

namespace nico {

// MARK: Statements

std::any LocalChecker::visit(Stmt::Expression* stmt) {
    // Visit the expression.
    expression_checker->expr_check(stmt->expression, false, true);
    return std::any();
}

std::any LocalChecker::visit(Stmt::Let* stmt) {
    std::shared_ptr<Type> expr_type = nullptr;

    // Visit the initializer (if present).
    if (stmt->expression.has_value()) {
        auto expr_type_opt =
            expression_checker->expr_check(stmt->expression.value(), false);
        if (!expr_type_opt.has_value())
            return std::any();
        expr_type = expr_type_opt.value();
    }

    // If the initializer is not present, the annotation will be (this is
    // checked in the parser).

    // Check the type annotation.
    if (stmt->annotation.has_value()) {
        auto anno_type_opt =
            annotation_checker->annotation_check(stmt->annotation.value());
        if (!anno_type_opt.has_value())
            return std::any();
        auto anno_type = anno_type_opt.value();

        // If an initializer is present, check that the annotation type and
        // initializer type are compatible.
        if (expr_type != nullptr && !expr_type->is_assignable_to(anno_type)) {
            Diagnostics::inst().emit_error(
                Err::LetTypeMismatch,
                stmt->expression.value()->location,
                "Type of initializer expression `" + expr_type->to_string() +
                    "` is not assignable to type in annotation `" +
                    anno_type->to_string() + "`."
            );
            return std::any();
        }
        // On occassion, two different types are compatible with each other.
        // E.g., the annotation is @i32, but the expression is nullptr.
        // In such cases, the annotated type takes precedence.
        expr_type = anno_type;
    }

    // At this point, expr_type is not null.

    if (!expr_type->is_definitely_sized()) {
        Diagnostics::inst().emit_error(
            Err::UnsizedTypeAllocation,
            stmt->identifier->location,
            "Cannot allocate variable `" +
                std::string(stmt->identifier->lexeme) + "` of unsized type `" +
                expr_type->to_string() + "`."
        );
        return std::any();
    }

    if (!stmt->has_var && !stmt->expression.has_value()) {
        Diagnostics::inst().emit_error(
            Err::ImmutableWithoutInitializer,
            stmt->identifier->location,
            "Immutable variable declaration must have an initializer."
        );
        return std::any();
    }

    // Create the binding entry.
    Binding binding(
        stmt->has_var ? Binding::Mutability::Var : Binding::Mutability::None,
        stmt->identifier->lexeme,
        &stmt->identifier->location,
        expr_type
    );

    auto node_opt = symbol_tree->add_binding_entry(
        binding,
        repl_mode ? Linkage::External : Linkage::Internal
    );
    if (!node_opt.has_value()) {
        return std::any();
    }
    else if (
        auto binding_node =
            std::dynamic_pointer_cast<Node::BindingEntry>(node_opt.value())
    ) {
        stmt->binding_entry = binding_node;
        return std::any();
    }
    else {
        panic(
            "LocalChecker::visit(Stmt::Let*): Symbol tree returned a "
            "non-binding "
            "entry for a binding entry."
        );
    }

    return std::any();
}

std::any LocalChecker::visit(Stmt::Static* stmt) {
    // Binding has already been created.
    // We only need to check if the initializer expression.

    if (!stmt->expression.has_value()) {
        // No initializer; do nothing.
        return std::any();
    }

    auto expr_type_opt =
        expression_checker->expr_check(stmt->expression.value(), false);
    if (!expr_type_opt.has_value())
        return std::any();
    auto expr_type = expr_type_opt.value();
    auto binding_type = stmt->binding_entry.lock()->binding.type;

    // If the type of the initializer expression is not assignable to the type
    // of the binding...
    if (!expr_type->is_assignable_to(binding_type)) {
        Diagnostics::inst().emit_error(
            Err::StaticTypeMismatch,
            stmt->expression.value()->location,
            std::string("Type of initializer expression `") +
                expr_type->to_string() +
                "` is not assignable to type in annotation `" +
                binding_type->to_string() + "`."
        );
    }
    // If the type of the initializer expression is not sized...
    else if (!expr_type->is_definitely_sized()) {
        Diagnostics::inst().emit_error(
            Err::UnsizedTypeAllocation,
            stmt->identifier->location,
            "Cannot allocate variable `" +
                std::string(stmt->identifier->lexeme) + "` of unsized type `" +
                expr_type->to_string() + "`."
        );
    }
    // Otherwise, the initializer expression is valid.
    else {
        // Update the binding's default expression to the initializer
        // expression.
        stmt->binding_entry.lock()->binding.default_expr =
            stmt->expression.value();
    }

    return std::any();
}

std::any LocalChecker::visit(Stmt::Func* stmt) {
    // Get the function's type
    auto func_type_opt =
        Type::as_a<Type::Function>(stmt->binding_entry.lock()->binding.type);
    if (!func_type_opt) {
        panic(
            "LocalChecker::visit(Stmt::Func*): Binding entry does not have a "
            "function type."
        );
    }
    auto func_type = func_type_opt.value();

    // The body is always present here.
    // The body is only absent in extern blocks, which are not checked by this
    // type checker.

    symbol_tree->add_local_scope(stmt->body.value());

    bool has_error = false;
    // Check the parameters.
    for (auto& param : stmt->parameters) {
        auto param_binding =
            func_type->parameters.at(std::string(param.identifier->lexeme))
                .value();

        // If the parameter has a default expression, check it.
        if (param_binding.default_expr.has_value()) {
            auto default_expr_ptr = param_binding.default_expr.value().lock();
            auto default_expr_type_opt =
                expression_checker->expr_check(default_expr_ptr, false);
            if (!default_expr_type_opt.has_value()) {
                has_error = true;
            }
            else if (!default_expr_type_opt.value()->is_assignable_to(
                         param_binding.type
                     )) {
                Diagnostics::inst().emit_error(
                    Err::DefaultArgTypeMismatch,
                    default_expr_ptr->location,
                    std::string("Type `") +
                        default_expr_type_opt.value()->to_string() +
                        "` is not compatible with parameter type `" +
                        param_binding.type->to_string() + "`."
                );
                has_error = true;
            }
        }
        auto node_opt =
            symbol_tree->add_binding_entry(param_binding, Linkage::Internal);
        if (!node_opt.has_value()) {
            has_error = true;
        }
        else {
            param.binding_entry =
                std::dynamic_pointer_cast<Node::BindingEntry>(node_opt.value());
        }
    }
    // If there was an error in the parameters, avoid checking the body.
    if (has_error) {
        symbol_tree->exit_scope();
        return std::any();
    }

    // Check the body.
    auto body_type_opt =
        expression_checker->expr_check(stmt->body.value(), false);
    if (!body_type_opt.has_value()) {
        // Ignore error, already reported.
    }
    // Body type must be assignable to the return type.
    else if (!body_type_opt.value()->is_assignable_to(func_type->return_type)) {
        Diagnostics::inst().emit_error(
            Err::FunctionReturnTypeMismatch,
            stmt->body.value()->location,
            std::string("Function body type `") +
                body_type_opt.value()->to_string() +
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
            Diagnostics::inst().emit_error(
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
            Diagnostics::inst().emit_error(
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
            Diagnostics::inst().emit_error(
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
    auto expr_type_opt =
        expression_checker->expr_check(stmt->expression, false);
    if (!expr_type_opt.has_value())
        return std::any();
    auto expr_type = expr_type_opt.value();

    if (!local_scope->yield_type) {
        // If this local scope does not currently have a yield type...
        local_scope->yield_type = expr_type;
    }
    else if (!expr_type->is_assignable_to(local_scope->yield_type.value())) {
        // If this local scope has a yield type, check that the new yield
        // expression is compatible with it.
        Diagnostics::inst().emit_error(
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
        Diagnostics::inst().emit_error(
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
        expression_checker->expr_check(expr, false);
    }
    return std::any();
}

std::any LocalChecker::visit(Stmt::Dealloc* stmt) {
    auto expr_type_opt =
        expression_checker->expr_check(stmt->expression, false);
    if (!expr_type_opt.has_value())
        return std::any();
    auto expr_type = expr_type_opt.value();

    if (Type::is_a<Type::Nullptr>(expr_type)) {
        Diagnostics::inst().emit_error(
            Err::DeallocNullptr,
            stmt->expression->location,
            "Cannot deallocate pointer of nullptr type."
        );
        return std::any();
    }
    else if (!Type::is_a<Type::IRawPtr>(expr_type)) {
        Diagnostics::inst().emit_error(
            Err::DeallocNonRawPointer,
            stmt->expression->location,
            "Cannot deallocate non-raw pointer type `" +
                expr_type->to_string() + "`."
        );
        return std::any();
    }
    else if (!symbol_tree->is_context_unsafe()) {
        Diagnostics::inst().emit_error(
            Err::DeallocOutsideUnsafeBlock,
            stmt->expression->location,
            "Cannot deallocate outside of unsafe context."
        );
        return std::any();
    }

    return std::any();
}

std::any LocalChecker::visit(Stmt::Namespace* stmt) {
    auto previous_scope = symbol_tree->current_scope;
    symbol_tree->current_scope = stmt->namespace_node.lock();

    for (auto& stmt : stmt->stmts) {
        stmt->accept(this);
    }
    // Namespaces may contain functions.
    // Functions may contain execution-space statements.
    // Therefore, namespaces may contain execution-space statements.
    // Therefore, we need to check namespaces.
    // Non-execution-space statements will still be ignored.

    symbol_tree->current_scope = previous_scope;
    // We could probably do `symbol_tree->exit_scope()` here and that would be
    // fine and possibly just as safe, but this is more predictable and saves us
    // the trouble of proving that `exit_scope()` works.

    return std::any();
}

std::any LocalChecker::visit(Stmt::ExternBlock* /*stmt*/) {
    // Extern blocks do not contain execution-space statements.
    return std::any();
}

std::any LocalChecker::visit(Stmt::TypeDef* /*stmt*/) {
    // Type definitions do not contain execution-space statements.
    return std::any();
}

std::any LocalChecker::visit(Stmt::StructDef* stmt) {
    auto previous_scope = symbol_tree->current_scope;
    symbol_tree->current_scope = stmt->struct_def_node.lock();

    // Get the struct type from the struct_def_node.
    auto struct_type_opt =
        Type::as_a<Type::Struct>(stmt->struct_def_node.lock()->type);
    if (!struct_type_opt) {
        panic(
            "LocalChecker::visit(Stmt::StructDef*): Struct definition node "
            "does not have a struct type."
        );
    }
    auto struct_type = struct_type_opt.value();

    for (auto& inner_stmt : stmt->stmts) {
        if (auto field_stmt =
                std::dynamic_pointer_cast<Stmt::Field>(inner_stmt)) {
            auto field_binding =
                struct_type->fields
                    .at(std::string(field_stmt->identifier->lexeme))
                    .value();
            // If the field has an initializer expression, check it.
            if (field_binding.default_expr.has_value()) {
                auto default_expr_ptr =
                    field_binding.default_expr.value().lock();
                auto default_expr_type_opt =
                    expression_checker->expr_check(default_expr_ptr, false);
                if (default_expr_type_opt.has_value() &&
                    !default_expr_type_opt.value()->is_assignable_to(
                        field_binding.type
                    )) {
                    Diagnostics::inst().emit_error(
                        Err::DefaultFieldTypeMismatch,
                        default_expr_ptr->location,
                        std::string("Type `") +
                            default_expr_type_opt.value()->to_string() +
                            "` is not compatible with field type `" +
                            field_binding.type->to_string() + "`."
                    );
                }
            }
        }
        else {
            inner_stmt->accept(this);
        }
    }

    symbol_tree->current_scope = previous_scope;

    return std::any();
}

std::any LocalChecker::visit(Stmt::Field* /*stmt*/) {
    // Field definitions do not contain execution-space statements.
    return std::any();
}

std::any LocalChecker::visit(Stmt::Eof* stmt) {
    return std::any();
}

void LocalChecker::run_check(std::unique_ptr<FrontendContext>& context) {
    symbol_tree->clear_modified();

    for (size_t i = context->stmts_processed; i < context->stmts.size(); ++i) {
        context->stmts[i]->accept(this);
    }

    if (Diagnostics::inst().get_errors().empty()) {
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
