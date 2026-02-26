#include "nico/frontend/components/local_checker.h"

#include <unordered_set>

#include "nico/shared/error_code.h"
#include "nico/shared/logger.h"
#include "nico/shared/status.h"
#include "nico/shared/utils.h"

namespace nico {

// MARK: Statements

std::any LocalChecker::visit(Stmt::Expression* stmt) {
    // Visit the expression.
    expression_checker.expr_check(stmt->expression, false, true);
    return std::any();
}

std::any LocalChecker::visit(Stmt::Let* stmt) {
    std::shared_ptr<Type> expr_type = nullptr;

    // Visit the initializer (if present).
    if (stmt->expression.has_value()) {
        expr_type =
            expression_checker.expr_check(stmt->expression.value(), false);
        if (!expr_type)
            return std::any();
    }

    // If the initializer is not present, the annotation will be (this is
    // checked in the parser).

    // If the type annotation is present, check that it matches the initializer.
    if (stmt->annotation.has_value()) {
        auto annotation_type =
            expression_checker.annotation_check(stmt->annotation.value());
        if (!annotation_type)
            return std::any();
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

    auto [ok, node] = symbol_tree->add_field_entry(field);
    if (!ok) {
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

std::any LocalChecker::visit(Stmt::Static* /*stmt*/) {
    // Do nothing. Static variables are handled by the global checker.
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

    symbol_tree->add_local_scope(stmt->body.value());

    bool has_error = false;
    // Check the parameters.
    for (auto& param : stmt->parameters) {
        auto param_field =
            func_type->parameters.at(std::string(param.identifier->lexeme))
                .value();

        // If the parameter has a default expression, check it.
        if (param_field.default_expr.has_value()) {
            auto default_expr_ptr = param_field.default_expr.value().lock();
            auto default_expr_type =
                expression_checker.expr_check(default_expr_ptr, false);
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
        auto [ok, node] = symbol_tree->add_field_entry(param_field);
        if (!ok) {
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
    auto body_type = expression_checker.expr_check(stmt->body.value(), false);
    if (!body_type) {
        // Ignore error, already reported.
    }
    // Body type must be assignable to the return type.
    else if (!body_type->is_assignable_to(*func_type->return_type)) {
        Logger::inst().log_error(
            Err::FunctionReturnTypeMismatch,
            stmt->body.value()->location,
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
    auto expr_type = expression_checker.expr_check(stmt->expression, false);
    if (!expr_type)
        return std::any();

    if (!local_scope->yield_type) {
        // If this local scope does not currently have a yield type...
        local_scope->yield_type = expr_type;
    }
    else if (!expr_type->is_assignable_to(*local_scope->yield_type.value())) {
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
        expression_checker.expr_check(expr, false);
    }
    return std::any();
}

std::any LocalChecker::visit(Stmt::Dealloc* stmt) {
    auto expr_type = expression_checker.expr_check(stmt->expression, false);
    if (!expr_type)
        return std::any();

    if (PTR_INSTANCEOF(expr_type, Type::Nullptr)) {
        Logger::inst().log_error(
            Err::DeallocNullptr,
            stmt->expression->location,
            "Cannot deallocate pointer of nullptr type."
        );
        return std::any();
    }
    else if (!PTR_INSTANCEOF(expr_type, Type::IRawPtr)) {
        Logger::inst().log_error(
            Err::DeallocNonRawPointer,
            stmt->expression->location,
            "Cannot deallocate non-raw pointer type `" +
                expr_type->to_string() + "`."
        );
        return std::any();
    }
    else if (!symbol_tree->is_context_unsafe()) {
        Logger::inst().log_error(
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

std::any LocalChecker::visit(Stmt::Extern* /*stmt*/) {
    // TODO: Implement local checking for extern blocks. For now, we'll just
    // ignore them.
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
