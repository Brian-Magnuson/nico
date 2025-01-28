#ifndef NICO_TYPE_H
#define NICO_TYPE_H

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "dictionary.h"

/**
 * @brief A base class for all types.
 */
class Type {
public:
    class INumeric;

    class Int;
    class Float;

    class Bool;

    class Pointer;
    class Reference;

    class Array;

    class NamedStruct;
    class Definition;

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

/**
 * @brief A pointer type.
 *
 * Points to another type.
 * Note: Since LLVM 15, pointers do not store type information.
 * Keep this in mind before converting to the LLVM type.
 */
class Type::Pointer : public Type {
public:
    // Whether the pointer is mutable.
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
    // Whether the reference is mutable.
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
    const size_t size;
    // Whether the array has a known size.
    const bool is_sized;

    Array(std::shared_ptr<Type> base) : base(base), size(0), is_sized(false) {}

    Array(std::shared_ptr<Type> base, size_t size) : base(base), size(size), is_sized(true) {}

    std::string to_string() const override {
        return "[" + base->to_string() + "; " + std::to_string(size) + "]";
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_array = dynamic_cast<const Array*>(&other)) {
            return *base == *other_array->base && size == other_array->size;
        }
        return false;
    }
};

/**
 * @brief A named struct type.
 *
 * Named struct types are types that have a name.
 * They are usually not useful on their own, but are used to reference a struct defined elsewhere.
 *
 * This language prefers nominal typing. Therefore, two structs with the same name are considered the same type.
 */
class Type::NamedStruct : public Type {
public:
    // The name of the struct type.
    const std::string name;

    NamedStruct(const std::string& name) : name(name) {}

    std::string to_string() const override {
        return name;
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_named_struct = dynamic_cast<const NamedStruct*>(&other)) {
            return name == other_named_struct->name;
        }
        return false;
    }
};

/**
 * @brief A struct definition.
 *
 * Symbols with this type are used to define structs rather than identify values.
 */
class Type::Definition : public Type {
public:
    // The fields of the struct.
    Dictionary<std::string, std::shared_ptr<Type>> fields;

    Definition() = default;

    std::string to_string() const override {
        std::string result = "{";
        for (auto& [key, value] : fields) {
            result += key + ": " + value->to_string() + ", ";
        }
        if (fields.size() > 0) {
            result.pop_back();
            result.pop_back();
        }
        result += "}";
        return result;
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_struct = dynamic_cast<const Definition*>(&other)) {
            return fields == other_struct->fields;
        }
        return false;
    }
};

#endif // NICO_TYPE_H
