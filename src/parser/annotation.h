#ifndef NICO_ANNOTATION_H
#define NICO_ANNOTATION_H

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "dictionary.h"
#include "ident.h"

/**
 * @brief Base class for all annotations.
 *
 * An annotation object is used in the AST to organize parts of the type annotation.
 * Annotations are effectively unresolved types, which can be resolved to proper type objects in the type checker.
 * It should not be confused with a type object, which represents the resolved type of an expression.
 *
 * Type annotations are not designed to be compared with each other; comparing types should only be done after resolution.
 */
class Annotation {
public:
    class Named;

    class Pointer;
    class Reference;

    class Array;
    class Object;
    class Tuple;

    virtual ~Annotation() = default;

    /**
     * @brief A visitor class for annotations.
     */
    class Visitor {
    public:
        virtual std::any visit(Named* annotation) = 0;
        virtual std::any visit(Pointer* annotation) = 0;
        virtual std::any visit(Reference* annotation) = 0;
        virtual std::any visit(Array* annotation) = 0;
        virtual std::any visit(Object* annotation) = 0;
        virtual std::any visit(Tuple* annotation) = 0;
    };

    /**
     * @brief Accept a visitor.
     *
     * @param visitor The visitor to accept.
     * @return The return value from the visitor.
     */
    virtual std::any accept(Visitor* visitor) = 0;

    /**
     * @brief Convert the annotation to a string representation.
     *
     * This method is used for debugging and logging purposes.
     * The string representation is not unique and should not be used to compare types.
     *
     * @return A string representation of the annotation.
     */
    virtual std::string
    to_string() const = 0;
};

/**
 * @brief An annotation consisting of an identifier.
 *
 * This annotation is used to represent named types, such as classes or interfaces.
 */
class Annotation::Named : public Annotation {
public:
    // The identifier of the named annotation.
    const Ident ident;

    Named(Ident ident) : ident(std::move(ident)) {}

    std::string to_string() const {
        return ident.to_string();
    }
};

/**
 * @brief An annotation representing a pointer type.
 *
 * This annotation is used to represent pointer types, which can be either mutable or immutable.
 */
class Annotation::Pointer : public Annotation {
public:
    // The base annotation that this pointer points to.
    const std::shared_ptr<Annotation> base;
    // Whether the object pointed to by this pointer is mutable.
    const bool is_mutable;

    Pointer(std::shared_ptr<Annotation> base, bool is_mutable = false)
        : base(std::move(base)), is_mutable(is_mutable) {}

    std::string to_string() const {
        return (is_mutable ? "var" : "") + std::string("*") + base->to_string();
    }
};

/**
 * @brief An annotation representing a reference type.
 *
 * This annotation is used to represent reference types, which can be either mutable or immutable.
 */
class Annotation::Reference : public Annotation {
public:
    // The base annotation that this reference points to.
    const std::shared_ptr<Annotation> base;
    // Whether the object pointed to by this reference is mutable.
    const bool is_mutable;

    Reference(std::shared_ptr<Annotation> base, bool is_mutable = false)
        : base(std::move(base)), is_mutable(is_mutable) {}

    std::string to_string() const {
        return (is_mutable ? "var" : "") + std::string("&") + base->to_string();
    }
};

/**
 * @brief An annotation representing an array type.
 *
 * This annotation is used to represent array types, which can be either sized or unsized.
 */
class Annotation::Array : public Annotation {
public:
    // The base annotation that this array contains.
    const std::shared_ptr<Annotation> base;
    // The number of elements in the array, if known.
    const std::optional<size_t> size;

    Array(std::shared_ptr<Annotation> base, std::optional<size_t> size = std::nullopt)
        : base(std::move(base)), size(size) {}
    std::string to_string() const {
        return "[" + base->to_string() + (size ? "; " + std::to_string(*size) : "") + "]";
    }
};

/**
 * @brief An annotation representing an object type.
 *
 * This annotation is used to represent objects with properties, similar to dictionaries.
 */
class Annotation::Object : public Annotation {
public:
    // A dictionary of properties, where keys are property names and values are annotations.
    const Dictionary<std::string, std::shared_ptr<Annotation>> properties;

    Object(Dictionary<std::string, std::shared_ptr<Annotation>> properties)
        : properties(std::move(properties)) {}

    std::string to_string() const {
        std::string result = "{";
        for (const auto& [key, value] : properties) {
            result += key + ": " + value->to_string() + ", ";
        }
        if (!properties.empty()) {
            result.pop_back();
            result.pop_back();
        }
        result += "}";

        return result;
    }
};

/**
 * @brief An annotation representing a tuple type.
 *
 * This annotation is used to represent a fixed-size collection of annotations.
 */
class Annotation::Tuple : public Annotation {
public:
    // A vector of annotations representing the elements of the tuple.
    const std::vector<std::shared_ptr<Annotation>> elements;

    Tuple(std::vector<std::shared_ptr<Annotation>> elements)
        : elements(std::move(elements)) {}

    std::string to_string() const {
        std::string result = "(";
        for (const auto& element : elements) {
            result += element->to_string() + ", ";
        }
        if (!elements.empty()) {
            result.pop_back();
            result.pop_back();
        }
        result += ")";
        return result;
    }
};

#endif // NICO_ANNOTATION_H
