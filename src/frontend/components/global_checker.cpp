#include "nico/frontend/components/global_checker.h"

#include "nico/shared/dictionary.h"
#include "nico/shared/logger.h"
#include "nico/shared/status.h"

namespace nico {

std::any GlobalChecker::visit(Stmt::Expression*) {
    // Do nothing.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Let*) {
    // Do nothing.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Static* stmt) {
    std::shared_ptr<Type> expr_type = nullptr;

    // Visit the initializer (if present).
    if (stmt->expression.has_value()) {
        auto expr_type_opt =
            expression_checker.expr_check(stmt->expression.value(), false);
        if (!expr_type_opt.has_value())
            return std::any();
        expr_type = expr_type_opt.value();
    }

    // If the initializer is not present, the annotation will be (this is
    // checked in the parser).

    // Check the type annotation.
    if (stmt->annotation.has_value()) {
        auto anno_type_opt =
            expression_checker.annotation_check(stmt->annotation.value());
        if (!anno_type_opt.has_value())
            return std::any();
        auto anno_type = anno_type_opt.value();

        // If an initializer is present, check that the annotation type and
        // initializer type are compatible.
        if (stmt->expression.has_value() &&
            !expr_type->is_assignable_to(*anno_type)) {
            Logger::inst().log_error(
                Err::StaticTypeMismatch,
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

    std::optional<std::string> custom_symbol = std::nullopt;
    bool is_extern = false;

    if (PTR_INSTANCEOF(symbol_tree->current_scope, Node::ExternBlock)) {
        if (stmt->expression.has_value()) {
            Logger::inst().log_error(
                Err::ExternStaticWithInitializer,
                stmt->identifier->location,
                "Static variable declared in extern block cannot have an "
                "initializer."
            );
            return std::any();
        }

        // Extern statics have a custom symbol based on their identifier name.
        custom_symbol = std::string(stmt->identifier->lexeme);
        // Currently, we do not allow users to manually specify a custom symbol,
        // but if we did, it would override this symbol.

        is_extern = true;
    }

    // Create the field entry.
    Field field(
        stmt->has_var,
        stmt->identifier->lexeme,
        &stmt->identifier->location,
        expr_type,
        stmt->expression
    );

    auto node_opt = symbol_tree->add_field_entry(field, custom_symbol);
    if (!node_opt.has_value()) {
        return std::any();
    }
    else if (auto field_entry = std::dynamic_pointer_cast<Node::FieldEntry>(
                 node_opt.value()
             )) {
        stmt->field_entry = field_entry;
    }
    else {
        panic(
            "GlobalChecker::visit(Stmt::Static*): Symbol tree returned a "
            "non-field entry for a field entry."
        );
    }

    if (is_extern) {
        // Extern statics are initialized elsewhere, so we mark them as
        // initialized to avoid multiple definition issues.
        stmt->field_entry.lock()->extern_initialized = true;
    }

    return std::any();
}

std::any GlobalChecker::visit(Stmt::Func* stmt) {
    // Start with the parameters.
    Dictionary<std::string, Field> parameter_fields;

    for (auto& param : stmt->parameters) {
        auto param_string = std::string(param.identifier->lexeme);
        // Check for duplicate parameter names.
        if (auto it = parameter_fields.find(param_string);
            it != parameter_fields.end()) {
            Logger::inst().log_error(
                Err::DuplicateFunctionParameterName,
                param.identifier->location,
                "Duplicate parameter name `" + param_string + "`."
            );
            Logger::inst().log_note(
                *it->second.location,
                "Previous declaration of parameter `" + param_string + "` here."
            );
            return std::any();
        }
        // Get the type from the annotation (which is always present).
        auto annotation_type_opt =
            expression_checker.annotation_check(param.annotation);
        if (!annotation_type_opt.has_value())
            return std::any();

        Field param_field(
            param.has_var,
            param.identifier->lexeme,
            &param.identifier->location,
            annotation_type_opt.value(),
            param.expression
        );
        parameter_fields.insert(param_string, param_field);
    }
    // Next, get the return type.
    std::shared_ptr<Type> return_type = nullptr;
    if (stmt->annotation.has_value()) {
        auto return_anno_type_opt =
            expression_checker.annotation_check(stmt->annotation.value());
        if (!return_anno_type_opt.has_value())
            return std::any();
        return_type = return_anno_type_opt.value();
    }
    else {
        // If no return annotation is present, the return type is Unit.
        return_type = std::make_shared<Type::Unit>();
    }
    // Create the function type.
    auto func_type =
        std::make_shared<Type::Function>(parameter_fields, return_type);

    // Create the field entry.
    Field field(
        false,
        stmt->identifier->lexeme,
        &stmt->identifier->location,
        func_type
    );
    // Functions are always immutable.

    std::optional<std::string> custom_symbol = std::nullopt;
    // If the function is declared in an extern block...
    if (PTR_INSTANCEOF(symbol_tree->current_scope, Node::ExternBlock)) {
        if (stmt->body.has_value()) {
            Logger::inst().log_error(
                Err::ExternFuncWithBody,
                stmt->identifier->location,
                "Function declared in extern block cannot have a body."
            );
            return std::any();
        }

        // Extern functions have a custom symbol based on their identifier name.
        custom_symbol = std::string(stmt->identifier->lexeme);
        // Currently, we do not allow users to manually specify a custom symbol,
        // but if we did, it would override this symbol.
    }
    // If the function is not declared in an extern block, but doesn't have a
    // body...
    else if (!stmt->body.has_value()) {
        Logger::inst().log_error(
            Err::NonExternFuncWithoutBody,
            stmt->identifier->location,
            "Non-extern function is missing a body."
        );
        return std::any();
    }

    // If the function has a custom symbol...
    if (custom_symbol.has_value()) {
        // It is declared as a field and is not overloadable.
        auto node_opt = symbol_tree->add_field_entry(field, custom_symbol);
        if (!node_opt.has_value()) {
            return std::any();
        }
        stmt->field_entry = node_opt.value();
    }
    // If the function has an autogenerated symbol...
    else {
        // It is declared as an overloadable function.
        auto node_opt = symbol_tree->add_overloadable_func(field);
        if (!node_opt.has_value()) {
            return std::any();
        }
        stmt->field_entry = node_opt.value();
    }

    return std::any();
}

std::any GlobalChecker::visit(Stmt::Print*) {
    // Do nothing.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Dealloc*) {
    // Do nothing.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Pass*) {
    // Do nothing.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Yield*) {
    // Do nothing.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Continue*) {
    // Do nothing.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Namespace* stmt) {
    auto node_opt = symbol_tree->add_namespace(stmt->identifier);
    if (!node_opt.has_value()) {
        return std::any();
    }

    stmt->namespace_node = node_opt.value();

    for (auto& stmt : stmt->stmts) {
        stmt->accept(this);
    }
    // Errors may occur, but we can still continue processing the rest of the
    // statements in the namespace.

    symbol_tree->exit_scope();
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Extern* stmt) {
    auto node_opt = symbol_tree->add_extern_block(stmt->identifier);
    if (!node_opt.has_value()) {
        return std::any();
    }

    stmt->extern_block_node = node_opt.value();

    for (auto& stmt : stmt->stmts) {
        stmt->accept(this);
    }
    // Errors may occur, but we can still continue processing the rest of the
    // statements in the extern block.

    symbol_tree->exit_scope();
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Eof*) {
    // Do nothing.
    return std::any();
}

void GlobalChecker::run_check(std::unique_ptr<FrontendContext>& context) {
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

void GlobalChecker::check(
    std::unique_ptr<FrontendContext>& context, bool repl_mode
) {
    if (IS_VARIANT(context->status, Status::Error)) {
        panic("GlobalChecker::check: Context is already in an error state.");
    }

    GlobalChecker checker(context->symbol_tree, repl_mode);
    checker.run_check(context);
}

} // namespace nico
