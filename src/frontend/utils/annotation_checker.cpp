#include "nico/frontend/utils/annotation_checker.h"

namespace nico {

std::any AnnotationChecker::visit(Annotation::NameRef* annotation) {
    std::shared_ptr<Type> type = nullptr;
    if (symbol_tree->try_resolve_name(annotation->name)) {
        auto node = annotation->name->node.lock();
        if (auto type_node = std::dynamic_pointer_cast<Node::ITypeNode>(node)) {
            type = type_node->type;
            return type;
        }
    }

    Logger::inst().log_error(
        Err::NameNotFound,
        annotation->name->identifier->location,
        "Could not resolve name `" + annotation->name->to_string() + "`."
    );
    return std::any();
}

std::any AnnotationChecker::visit(Annotation::Pointer* annotation) {
    std::shared_ptr<Type> type = nullptr;
    auto base_any = annotation->base->accept(this);
    if (!base_any.has_value())
        return std::any();
    auto base_type = std::any_cast<std::shared_ptr<Type>>(base_any);
    type =
        std::make_shared<Type::RawTypedPtr>(base_type, annotation->is_mutable);
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
    auto base_any = annotation->base->accept(this);
    if (!base_any.has_value())
        return std::any();
    auto base_type = std::any_cast<std::shared_ptr<Type>>(base_any);
    type = std::make_shared<Type::Reference>(base_type, annotation->is_mutable);
    return type;
}

std::any AnnotationChecker::visit(Annotation::Array* annotation) {
    std::shared_ptr<Type> type = nullptr;
    if (!annotation->base.has_value()) {
        type = std::make_shared<Type::EmptyArray>();
        return type;
    }
    auto base_any = annotation->base.value()->accept(this);
    if (!base_any.has_value())
        return std::any();
    auto base_type = std::any_cast<std::shared_ptr<Type>>(base_any);
    if (annotation->size.has_value()) {
        type =
            std::make_shared<Type::Array>(base_type, annotation->size.value());
    }
    else {
        type = std::make_shared<Type::Array>(base_type);
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
            Logger::inst().log_error(
                Err::DuplicateObjectAnnotationFieldName,
                field.identifier->location,
                "Duplicate field name in object annotation: `" + field_name +
                    "`."
            );
            Logger::inst().log_note(
                existing_binding->location,
                "Previous field with the same name declared here."
            );
            has_error = true;
            continue;
        }

        auto field_type_any = field.annotation->accept(this);
        if (!field_type_any.has_value())
            return std::any();
        auto field_type = std::any_cast<std::shared_ptr<Type>>(field_type_any);

        fields_dict.insert(
            field_name,
            Binding(
                field.mutability,
                field_name,
                &field.identifier->location,
                field_type
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
        auto elem_any = element->accept(this);
        if (!elem_any.has_value())
            return std::any();
        element_types.push_back(std::any_cast<std::shared_ptr<Type>>(elem_any));
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
        Logger::inst().log_error(
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
    std::weak_ptr<ExpressionChecker> expr_checker
) {
    auto checker = std::make_shared<AnnotationChecker>(Private());
    checker->symbol_tree = symbol_tree;
    checker->expr_checker = expr_checker;
    return checker;
}

std::optional<std::shared_ptr<Type>>
AnnotationChecker::annotation_check(std::shared_ptr<Annotation> annotation) {
    auto type_any = annotation->accept(this);
    if (!type_any.has_value())
        return std::nullopt;
    auto type = std::any_cast<std::shared_ptr<Type>>(type_any);
    return type;
}

} // namespace nico
