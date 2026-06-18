#include "nico/frontend/utils/annotation_checker.h"

#include "nico/frontend/utils/symbol_node.h"

namespace nico {

std::any AnnotationChecker::visit(Annotation::NameRef* annotation) {
    std::shared_ptr<Type> type = nullptr;
    if (symbol_tree->try_resolve_name(annotation->name)) {
        auto node = annotation->name->node.lock();
        if (auto type_node = std::dynamic_pointer_cast<Node::ITypeNode>(node)) {
            type = type_node->type;
            return type;
        }
        Diagnostics::inst().emit_error(
            Err::NameNotAType,
            annotation->name->identifier->location,
            "Name reference `" + annotation->name->to_string() +
                "` does not "
                "refer to a type."
        );
        return std::any();
    }
    else if (allow_unresolved_named_types) {
        auto node = symbol_tree->add_unresolved_type(annotation->name);
        type = node->referencing_type_object;
        return type;
    }
    Diagnostics::inst().emit_error(
        Err::NameNotFound,
        annotation->name->identifier->location,
        "Could not resolve name `" + annotation->name->to_string() + "`."
    );
    return std::any();
}

std::any AnnotationChecker::visit(Annotation::Pointer* annotation) {
    std::shared_ptr<Type> type = nullptr;
    auto base_opt = annotation_check(annotation->base);
    if (!base_opt.has_value())
        return std::any();
    type = std::make_shared<Type::RawTypedPtr>(
        base_opt.value(),
        annotation->is_mutable
    );
    return type;
}

std::any AnnotationChecker::visit(Annotation::Nullptr* /*annotation*/) {
    std::shared_ptr<Type> type = std::make_shared<Type::Nullptr>();
    return type;
}

std::any AnnotationChecker::visit(Annotation::Void* /*annotation*/) {
    std::shared_ptr<Type> type = std::make_shared<Type::Void>();
    return type;
}

std::any AnnotationChecker::visit(Annotation::Reference* annotation) {
    std::shared_ptr<Type> type = nullptr;
    auto base_opt = annotation_check(annotation->base);
    if (!base_opt.has_value())
        return std::any();
    type = std::make_shared<Type::Reference>(
        base_opt.value(),
        annotation->is_mutable
    );
    return type;
}

std::any AnnotationChecker::visit(Annotation::Array* annotation) {
    std::shared_ptr<Type> type = nullptr;
    if (!annotation->base.has_value()) {
        type = std::make_shared<Type::EmptyArray>();
        return type;
    }
    auto base_opt = annotation_check(annotation->base.value());
    if (!base_opt.has_value())
        return std::any();
    if (annotation->size.has_value()) {
        type = std::make_shared<Type::Array>(
            base_opt.value(),
            annotation->size.value()
        );
    }
    else {
        type = std::make_shared<Type::Array>(base_opt.value());
    }
    return type;
}

std::any AnnotationChecker::visit(Annotation::Object* annotation) {
    std::shared_ptr<Type> type = nullptr;

    Dictionary<std::string, Binding> fields_dict;
    bool has_error = false;

    for (const auto& field : annotation->fields) {
        auto field_name = std::string(field.identifier->lexeme);
        if (auto existing_binding = fields_dict.at(field_name)) {
            Diagnostics::inst().emit_error(
                Err::DuplicateObjectAnnotationFieldName,
                field.identifier->location,
                "Duplicate field name in object annotation: `" + field_name +
                    "`."
            );
            Diagnostics::inst().emit_note(
                existing_binding->location,
                "Previous field with the same name declared here."
            );
            has_error = true;
            continue;
        }

        auto field_type_opt = annotation_check(field.annotation);
        if (!field_type_opt.has_value())
            return std::any();

        fields_dict.insert(
            field_name,
            Binding(
                field.mutability,
                field_name,
                &field.identifier->location,
                field_type_opt.value()
            )
        );
    }

    if (has_error)
        return std::any();

    type = std::make_shared<Type::Object>(std::move(fields_dict));
    return type;
}

std::any AnnotationChecker::visit(Annotation::Tuple* annotation) {
    std::shared_ptr<Type> type = nullptr;
    std::vector<std::shared_ptr<Type>> element_types;
    for (const auto& element : annotation->elements) {
        auto elem_opt = annotation_check(element);
        if (!elem_opt.has_value())
            return std::any();
        element_types.push_back(elem_opt.value());
    }
    if (element_types.empty()) {
        type = std::make_shared<Type::Unit>();
    }
    else {
        type = std::make_shared<Type::Tuple>(element_types);
    }

    return type;
}

std::any AnnotationChecker::visit(Annotation::TypeOf* annotation) {
    if (expr_checker.expired()) {
        Diagnostics::inst().emit_error(
            Err::UncheckableTypeofAnnotation,
            annotation->location,
            "Cannot use `typeof` annotation in declaration space."
        );
        return std::any();
    }
    annotation->expression->accept(expr_checker.lock().get(), false);
    if (!annotation->expression->type) {
        panic(
            "Annotation::TypeOf::visit: expression has no type after checking."
        );
        return std::any();
    }
    return annotation->expression->type;
}

std::shared_ptr<AnnotationChecker> AnnotationChecker::create(
    std::shared_ptr<SymbolTree> symbol_tree,
    bool allow_unresolved_named_types,
    std::weak_ptr<ExpressionChecker> expr_checker
) {
    auto checker = std::make_shared<AnnotationChecker>(Private());
    checker->symbol_tree = symbol_tree;
    checker->allow_unresolved_named_types = allow_unresolved_named_types;
    checker->expr_checker = expr_checker;
    return checker;
}

std::optional<std::shared_ptr<Type>>
AnnotationChecker::annotation_check(std::shared_ptr<Annotation> annotation) {
    auto type_any = annotation->accept(this);
    if (!type_any.has_value())
        return std::nullopt;
    return std::any_cast<std::shared_ptr<Type>>(type_any);
}

} // namespace nico
