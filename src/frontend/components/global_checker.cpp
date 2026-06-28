#include "nico/frontend/components/global_checker.h"

#include "nico/shared/diagnostics.h"
#include "nico/shared/dictionary.h"
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

    // Check the type annotation. Type annotation is required for static
    // variables.
    if (stmt->annotation.has_value()) {
        auto anno_type_opt =
            annotation_checker->annotation_check(stmt->annotation.value());
        if (!anno_type_opt.has_value())
            return std::any();
        expr_type = anno_type_opt.value();
    }
    else {
        Diagnostics::inst().emit_error(
            Err::StaticMissingTypeAnnotation,
            stmt->identifier->location,
            "Static variable declaration must have a type annotation."
        );
        return std::any();
    }

    if (PTR_INSTANCEOF(symbol_tree->current_scope, Node::ExternBlock)) {
        if (stmt->expression.has_value()) {
            Diagnostics::inst().emit_error(
                Err::ExternStaticWithInitializer,
                stmt->identifier->location,
                "Static variable declared in extern block cannot have an "
                "initializer."
            );
        }
        if (stmt->linkage_opt == Linkage::Internal) {
            Diagnostics::inst().emit_error(
                Err::ExternBindingWithInternalLinkage,
                stmt->identifier->location,
                "Static variable declared in extern block cannot have internal "
                "linkage."
            );
        }

        stmt->linkage_opt = Linkage::External;
    }
    else if (!stmt->has_var && !stmt->expression.has_value()) {
        Diagnostics::inst().emit_error(
            Err::ImmutableWithoutInitializer,
            stmt->identifier->location,
            "Immutable variable declaration must have an initializer."
        );
    }

    // If the linkage was set to external, but custom symbol is not set, set the
    // custom symbol to the variable name.
    if (stmt->linkage_opt == Linkage::External &&
        !stmt->custom_symbol_opt.has_value()) {
        stmt->custom_symbol_opt = std::string(stmt->identifier->lexeme);
    }
    // If, at this point, linkage does not have a value...
    else if (!stmt->linkage_opt.has_value()) {
        // Linkage is set based on whether we are in repl mode or not.
        stmt->linkage_opt = repl_mode ? Linkage::External : Linkage::Internal;
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
        stmt->linkage_opt.value(),
        stmt->custom_symbol_opt
    );
    if (!node_opt.has_value()) {
        return std::any();
    }
    else if (
        auto binding_entry =
            std::dynamic_pointer_cast<Node::BindingEntry>(node_opt.value())
    ) {
        stmt->binding_entry = binding_entry;
    }
    else {
        panic(
            "GlobalChecker::visit(Stmt::Static*): Symbol tree returned a "
            "non-binding entry for a binding entry."
        );
    }

    return std::any();
}

std::any GlobalChecker::visit(Stmt::Func* stmt) {
    // Start with the parameters.
    Dictionary<std::string, Binding> param_bindings;

    for (auto& param : stmt->parameters) {
        auto param_string = std::string(param.identifier->lexeme);
        // Check for duplicate parameter names.
        if (auto it = param_bindings.find(param_string);
            it != param_bindings.end()) {
            Diagnostics::inst().emit_error(
                Err::DuplicateFunctionParameterName,
                param.identifier->location,
                "Duplicate parameter name `" + param_string + "`."
            );
            Diagnostics::inst().emit_note(
                *it->second.location,
                "Previous declaration of parameter `" + param_string + "` here."
            );
            return std::any();
        }
        // Get the type from the annotation (which is always present).
        auto annotation_type_opt =
            annotation_checker->annotation_check(param.annotation);
        if (!annotation_type_opt.has_value())
            return std::any();

        Binding param_binding(
            param.has_var ? Binding::Mutability::Var
                          : Binding::Mutability::None,
            param.identifier->lexeme,
            &param.identifier->location,
            annotation_type_opt.value(),
            param.expression
        );
        param_bindings.insert(param_string, param_binding);
    }
    // Next, get the return type.
    std::shared_ptr<Type> return_type = nullptr;
    if (stmt->annotation.has_value()) {
        auto return_anno_type_opt =
            annotation_checker->annotation_check(stmt->annotation.value());
        if (!return_anno_type_opt.has_value())
            return std::any();
        return_type = return_anno_type_opt.value();
    }
    else {
        // If no return annotation is present, the return type is Void.
        return_type = std::make_shared<Type::Void>();
    }

    // If the function is declared in an extern block...
    if (PTR_INSTANCEOF(symbol_tree->current_scope, Node::ExternBlock)) {
        if (stmt->body.has_value()) {
            Diagnostics::inst().emit_error(
                Err::ExternBlockFuncWithBody,
                stmt->identifier->location,
                "Function declared in extern block cannot have a body."
            );
        }
        if (stmt->linkage_opt == Linkage::Internal) {
            Diagnostics::inst().emit_error(
                Err::ExternBindingWithInternalLinkage,
                stmt->identifier->location,
                "Function declared in extern block cannot have internal "
                "linkage."
            );
        }

        // Currently, we do not allow users to manually specify a custom symbol,
        // but if we did, it would override this symbol.
        stmt->linkage_opt = Linkage::External;
    }
    // If the function is not declared in an extern block...
    else {
        if (!stmt->body.has_value()) {
            Diagnostics::inst().emit_error(
                Err::FuncWithoutBody,
                stmt->identifier->location,
                "Function is missing a body."
            );
            // Not the most dangerous error, so we can continue checking the
            // rest of the statement.
        }
        if (stmt->is_variadic) {
            Diagnostics::inst().emit_error(
                Err::NonExternVariadicFunc,
                stmt->identifier->location,
                "Non-extern function declaration is not allowed to be variadic."
            );
            return std::any();
        }
    }

    // If the linkage was set to external, but custom symbol is not set, set the
    // custom symbol to the function name.
    if (stmt->linkage_opt == Linkage::External &&
        !stmt->custom_symbol_opt.has_value()) {
        stmt->custom_symbol_opt = std::string(stmt->identifier->lexeme);
    }
    // If, at this point, linkage does not have a value...
    else if (!stmt->linkage_opt.has_value()) {
        // Linkage is set based on whether we are in repl mode or not.
        stmt->linkage_opt = repl_mode ? Linkage::External : Linkage::Internal;
    }

    // Create the function type.
    auto func_type = std::make_shared<Type::Function>(
        param_bindings,
        return_type,
        stmt->is_variadic
    );

    // Create the binding entry.
    Binding binding(
        Binding::Mutability::None, // Functions are always immutable.
        stmt->identifier->lexeme,
        &stmt->identifier->location,
        func_type
    );
    // Functions are always immutable.

    // If the function has a custom symbol...
    if (stmt->custom_symbol_opt.has_value() ||
        stmt->linkage_opt == Linkage::External) {
        // It is declared as a binding and is not overloadable.
        auto node_opt = symbol_tree->add_binding_entry(
            binding,
            stmt->linkage_opt.value(),
            stmt->custom_symbol_opt
        );
        if (!node_opt.has_value()) {
            return std::any();
        }
        stmt->binding_entry = node_opt.value();
    }
    // If the function has an autogenerated symbol...
    else {
        // It is declared as an overloadable function.
        auto node_opt = symbol_tree->add_overloadable_func(binding);
        if (!node_opt.has_value()) {
            return std::any();
        }
        stmt->binding_entry = node_opt.value();
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

std::any GlobalChecker::visit(Stmt::ExternBlock* stmt) {
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

std::any GlobalChecker::visit(Stmt::TypeDef* stmt) {
    // First, visit the right side
    auto anno_type_opt = annotation_checker->annotation_check(stmt->annotation);
    // This checker's annotation checker will yield a type, even if the names in
    // the annotation cannot be resolved. However, if the annotation checker
    // returns no value, that means there was an error in checking the
    // annotation.

    if (!anno_type_opt.has_value()) {
        return std::any();
    }

    auto type = anno_type_opt.value();
    auto node_opt = symbol_tree->add_type_def(stmt->identifier, type);
    if (!node_opt.has_value()) {
        return std::any();
    }
    stmt->type_def_node = node_opt.value();

    return std::any();
}

std::any GlobalChecker::visit(Stmt::StructDef* stmt) {
    auto struct_node_opt = symbol_tree->add_struct_def(stmt->identifier, false);
    if (!struct_node_opt.has_value()) {
        return std::any();
    }
    auto struct_node = struct_node_opt.value();

    for (auto& stmt : stmt->stmts) {
        if (auto field_stmt = std::dynamic_pointer_cast<Stmt::Field>(stmt)) {
            auto type_opt =
                annotation_checker->annotation_check(field_stmt->annotation);
            if (!type_opt.has_value()) {
                continue;
            }
            Binding field_binding(
                field_stmt->mutability,
                std::string(field_stmt->identifier->lexeme),
                &field_stmt->identifier->location,
                type_opt.value()
            );
            if (auto existing_field =
                    struct_node->fields.at(field_binding.name)) {
                Diagnostics::inst().emit_error(
                    Err::DuplicateStructFieldName,
                    *field_binding.location,
                    "Duplicate field name `" + field_binding.name +
                        "` in "
                        "struct definition."
                );
                Diagnostics::inst().emit_note(
                    existing_field->location,
                    "Previous declaration of field `" + field_binding.name +
                        "` here."
                );
                continue;
            }

            struct_node->fields.insert(field_binding.name, field_binding);
        }
        else {
            stmt->accept(this);
        }
    }

    stmt->struct_def_node = struct_node;
    symbol_tree->exit_scope();
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Field* stmt) {
    // Do nothing. The field is processed in the StructDef visitor.
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

    symbol_tree->resolve_unresolved_types();

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
