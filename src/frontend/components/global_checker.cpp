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

    // Check the type annotation.
    if (stmt->annotation.has_value()) {
        // auto anno_any =
        // stmt->annotation.value()->accept(&annotation_checker);
        auto anno_type =
            expression_checker.annotation_check(stmt->annotation.value());
        if (!anno_type)
            return std::any();
        expr_type = anno_type;
    }
    else {
        Logger::inst().log_error(
            Err::StaticVarWithoutType,
            stmt->identifier->location,
            "Static variable declarations must have a type annotation."
        );
        return std::any();
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
        expr_type,
        stmt->expression
    );

    auto [ok, node] = symbol_tree->add_field_entry(field);
    if (!ok) {
        return std::any();
    }
    else if (auto field_entry =
                 std::dynamic_pointer_cast<Node::FieldEntry>(node)) {
        stmt->field_entry = field_entry;
        return std::any();
    }
    else {
        panic(
            "GlobalChecker::visit(Stmt::Static*): Symbol tree returned a "
            "non-field entry for a field entry."
        );
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
        auto annotation_type =
            expression_checker.annotation_check(param.annotation);
        if (!annotation_type)
            return std::any();

        Field param_field(
            param.has_var,
            param.identifier->lexeme,
            &param.identifier->location,
            annotation_type,
            param.expression
        );
        parameter_fields.insert(param_string, param_field);
    }
    // Next, get the return type.
    std::shared_ptr<Type> return_type = nullptr;
    if (stmt->annotation.has_value()) {
        auto return_anno_type =
            expression_checker.annotation_check(stmt->annotation.value());
        if (!return_anno_type)
            return std::any();
        return_type = return_anno_type;
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

    auto [ok, node] = symbol_tree->add_overloadable_func(field);
    if (!ok) {
        return std::any();
    }
    else if (auto field_entry =
                 std::dynamic_pointer_cast<Node::FieldEntry>(node)) {
        stmt->field_entry = field_entry;
        return std::any();
    }
    else {
        panic(
            "GlobalChecker::visit(Stmt::Func*): Symbol tree returned a "
            "non-field entry for a field entry."
        );
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
    auto [ok, node] = symbol_tree->add_namespace(stmt->identifier);
    if (!ok) {
        return std::any();
    }

    stmt->namespace_node = node;

    for (auto& stmt : stmt->stmts) {
        stmt->accept(this);
    }
    // Errors may occur, but we can still continue processing the rest of the
    // statements in the namespace.

    symbol_tree->exit_scope();
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Extern*) {
    // TODO: Implement global checking for extern blocks.
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
