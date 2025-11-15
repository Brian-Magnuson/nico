#include "nico/frontend/utils/annotation_checker.h"

#include "nico/frontend/utils/frontend_context.h"
#include "nico/shared/logger.h"
#include "nico/shared/status.h"

namespace nico {

std::any AnnotationChecker::visit(Annotation::NameRef* annotation) {
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
    else {
        Logger::inst().log_error(
            Err::UnknownAnnotationName,
            *annotation->location,
            "Unknown type annotation: `" + annotation->name.to_string() + "`."
        );
    }

    return type;
}

std::any AnnotationChecker::visit(Annotation::Pointer* annotation) {
    std::shared_ptr<Type> type = nullptr;
    auto base_any = annotation->base->accept(this);
    if (!base_any.has_value())
        return std::any();
    auto base_type = std::any_cast<std::shared_ptr<Type>>(base_any);
    type = std::make_shared<Type::Pointer>(base_type, annotation->is_mutable);
    return type;
}

std::any AnnotationChecker::visit(Annotation::Nullptr* /*annotation*/) {
    std::shared_ptr<Type> type = std::make_shared<Type::Nullptr>();
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
    return std::any();
}

std::any AnnotationChecker::visit(Annotation::Object* annotation) {
    return std::any();
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
    if (!expr_visitor) {
        Logger::inst().log_error(
            Err::UncheckableTypeofAnnotation,
            *annotation->location,
            "Expression after 'typeof' cannot be checked here."
        );
        return nullptr;
    }
    annotation->expression->accept(expr_visitor, false);
    return annotation->expression->type;
}

} // namespace nico
