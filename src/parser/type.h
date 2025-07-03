#ifndef NICO_TYPE_H
#define NICO_TYPE_H

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "dictionary.h"

/**
 * @brief A base class for all types.
 */
class Type {
public:
    // Numeric types
    class INumeric;
    class Int;
    class Float;

    // Boolean type
    class Bool;

    // Pointer types
    class Pointer;
    class Reference;

    // Aggregate types
    class Array;
    class Tuple;
    class Object;

    // Special types
    class Named;
    class Function;

    // Non-declarable types
    class StructDef;

    Type() = default;
    virtual ~Type() = default;

    /**
     * @brief Converts this type to a string.
     *
     * In theory, the string representation should be unique for the type.
     *
     * @return A string representation of the type.
     */
    virtual std::string to_string() const = 0;

    /**
     * @brief Check if two types are equivalent.
     *
     * Note: The types must match exactly.
     * This operator does not consider if one type can be implicitly converted to another.
     *
     * @param other The other type to compare.
     * @return True if the types are equivalent. False otherwise.
     */
    virtual bool operator==(const Type& other) const = 0;

    /**
     * @brief Check if two types are not equivalent.
     *
     * Types must match exactly to be considered equivalent.
     * Effectively the negation of the `==` operator.
     *
     * @param other The other type to compare.
     * @return True if the types are not equivalent. False otherwise.
     */
    virtual bool operator!=(const Type& other) const {
        return !(*this == other);
    }
};

/**
 * @brief A multi-purpose field class.
 *
 * Used to represent properties or shared variables in complex types,
 * properties in objects, and parameters in functions.
 */
class Field {
public:
    // Whether the field is declared with `var` or not.
    const bool is_var;
    // The name of the field.
    const std::string name;
    // The type of the field.
    const std::shared_ptr<Type> type;

    Field(bool is_var, const std::string& name, std::shared_ptr<Type> type)
        : is_var(is_var), name(name), type(type) {}

    std::string to_string() const {
        return (is_var ? "var " : "") + name + ": " + type->to_string();
    }

    bool operator==(const Field& other) const {
        return is_var == other.is_var && name == other.name && *type == *other.type;
    }
};

// MARK: Numeric types

/**
 * @brief A base class for all numeric types.
 *
 * Includes `Type::Int` and `Type::Float`.
 */
class Type::INumeric : public Type {
};

/**
 * @brief An integer type.
 *
 * Can be signed or unsigned, and can have any width.
 * To save space, the width is stored as a uint8_t.
 * Additionally, it is recommended only widths of 8, 16, 32, or 64 are used.
 */
class Type::Int : public Type::INumeric {
public:
    // Whether the integer is signed or unsigned.
    const bool is_signed;
    // The width of the integer in bits. Can be any number, but should be 8, 16, 32, or 64.
    const uint8_t width;

    Int(bool is_signed, uint8_t width) : is_signed(is_signed), width(width) {}

    std::string to_string() const override {
        return (is_signed ? "i" : "u") + std::to_string(width);
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_int = dynamic_cast<const Int*>(&other)) {
            return is_signed == other_int->is_signed && width == other_int->width;
        }
        return false;
    }
};

/**
 * @brief A floating-point type.
 *
 * Can be 32 or 64 bits wide.
 */
class Type::Float : public Type::INumeric {
public:
    // The width of the float in bits. Can be 32 or 64.
    const uint8_t width;

    Float(uint8_t width) : width(width) {
        if (width != 32 && width != 64) {
            std::cerr << "Type::Float: Invalid width " << width << ". Must be 32 or 64." << std::endl;
            std::abort();
        }
    }

    std::string to_string() const override {
        return "f" + std::to_string(width);
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_float = dynamic_cast<const Float*>(&other)) {
            return width == other_float->width;
        }
        return false;
    }
};

// MARK: Boolean type

/**
 * @brief A boolean type.
 *
 * Boolean types have no additional state as there is no need;
 * All boolean types are the same.
 * In LLVM, booleans may be represented as an integer 1 bit wide (`i1`).
 */
class Type::Bool : public Type {
public:
    std::string to_string() const override {
        return "bool";
    }

    bool operator==(const Type& other) const override {
        return dynamic_cast<const Bool*>(&other) != nullptr;
    }
};

// MARK: Pointer types

/**
 * @brief A pointer type.
 *
 * Points to another type.
 * Note: Since LLVM 15, pointers do not store type information.
 * Keep this in mind before converting to the LLVM type.
 */
class Type::Pointer : public Type {
public:
    // Whether object pointed to by this pointer is mutable.
    bool is_mutable;
    // The type that the pointer points to.
    const std::shared_ptr<Type> base;

    Pointer(std::shared_ptr<Type> base) : base(base) {}

    std::string to_string() const override {
        return std::string(is_mutable ? "var" : "") + "*" + base->to_string();
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_pointer = dynamic_cast<const Pointer*>(&other)) {
            return *base == *other_pointer->base;
        }
        return false;
    }
};

/**
 * @brief A reference type.
 *
 * References are pointers with special semantics.
 * Note: Since LLVM 15, pointers do not store type information.
 * Keep this in mind before converting to the LLVM type.
 */
class Type::Reference : public Type {
public:
    // Whether object pointed to by this reference is mutable.
    bool is_mutable;
    // The type that the reference points to.
    const std::shared_ptr<Type> base;

    Reference(std::shared_ptr<Type> base) : base(base) {}

    std::string to_string() const override {
        return std::string(is_mutable ? "var" : "") + "&" + base->to_string();
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_reference = dynamic_cast<const Reference*>(&other)) {
            return *base == *other_reference->base;
        }
        return false;
    }
};

// MARK: Aggregate types

/**
 * @brief An array type.
 *
 * Contains a base type and a size.
 */
class Type::Array : public Type {
public:
    // The type of the elements in the array.
    const std::shared_ptr<Type> base;
    // The number of elements in the array.
    const std::optional<size_t> size;

    Array(std::shared_ptr<Type> base) : base(base), size(std::nullopt) {}

    Array(std::shared_ptr<Type> base, size_t size) : base(base), size(size) {}

    std::string to_string() const override {
        return "[" + base->to_string() + "; " + (size ? std::to_string(*size) : "unknown") + "]";
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_array = dynamic_cast<const Array*>(&other)) {
            return *base == *other_array->base && size == other_array->size;
        }
        return false;
    }
};

/**
 * @brief A tuple type.
 *
 * Used to represent a fixed-size collection of types.
 */
class Type::Tuple : public Type {
public:
    // The types of the elements in the tuple.
    const std::vector<std::shared_ptr<Type>> elements;

    Tuple(std::vector<std::shared_ptr<Type>> elements) : elements(std::move(elements)) {}

    std::string to_string() const override {
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

    bool operator==(const Type& other) const override {
        if (const auto* other_tuple = dynamic_cast<const Tuple*>(&other)) {
            return elements == other_tuple->elements;
        }
        return false;
    }
};

/**
 * @brief An object type.
 *
 * Used to represent objects with properties.
 */
class Type::Object : public Type {
public:
    // The fields of the object.
    Dictionary<std::string, Field> properties;

    Object() = default;

    std::string to_string() const override {
        std::string result = "{";
        for (const auto& [key, value] : properties) {
            result += value.to_string() + ", ";
        }
        if (properties.size() > 0) {
            result.pop_back();
            result.pop_back();
        }
        result += "}";
        return result;
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_object = dynamic_cast<const Object*>(&other)) {
            return properties == other_object->properties;
        }
        return false;
    }
};

// MARK: Special types

/**
 * @brief A named type.
 *
 * Used to represent types that have a name, such as complex types and aliased types.
 */
class Type::Named : public Type {
public:
    // The name of the type.
    const std::string name;

    Named(const std::string& name) : name(name) {}

    std::string to_string() const override {
        return name;
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_named = dynamic_cast<const Named*>(&other)) {
            return name == other_named->name;
        }
        return false;
    }
};

/**
 * @brief A function type.
 *
 * Used to represent functions with parameters and return types.
 */
class Type::Function : public Type {
public:
    // The parameters of the function.
    std::vector<Field> parameters;
    // The return type of the function.
    const std::shared_ptr<Type> return_type;

    Function(std::vector<Field> parameters, std::shared_ptr<Type> return_type)
        : parameters(std::move(parameters)), return_type(std::move(return_type)) {}

    std::string to_string() const override {
        std::string result = "func(";
        for (const auto& param : parameters) {
            result += param.to_string() + ", ";
        }
        if (!parameters.empty()) {
            result.pop_back();
            result.pop_back();
        }
        result += ") -> " + return_type->to_string();
        return result;
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_function = dynamic_cast<const Function*>(&other)) {
            return parameters == other_function->parameters && *return_type == *other_function->return_type;
        }
        return false;
    }
};

// MARK: Non-declarable types

class Type::StructDef : public Type {
public:
    bool is_class = false;

    Dictionary<std::string, Field> properties;

    Dictionary<std::string, std::shared_ptr<Type::Function>> methods;

    StructDef() = default;
    StructDef(bool is_class) : is_class(is_class) {}

    std::string to_string() const override {
        std::string result = (is_class ? "class " : "struct ") + std::string("{");
        for (const auto& [key, value] : properties) {
            result += value.to_string() + ", ";
        }
        if (properties.size() > 0) {
            result.pop_back();
            result.pop_back();
        }
        result += "}";

        if (!methods.empty()) {
            result += " impl {";
            for (const auto& [key, value] : methods) {
                result += key + ": " + value->to_string() + ", ";
            }
            if (methods.size() > 0) {
                result.pop_back();
                result.pop_back();
            }
            result += "}";
        }

        return result;
    }
};

#endif // NICO_TYPE_H
