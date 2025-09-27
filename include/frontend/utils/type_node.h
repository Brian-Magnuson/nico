#ifndef NICO_TYPE_NODE_H
#define NICO_TYPE_NODE_H

#include <utility>

#include "frontend/utils/nodes.h"
#include "frontend/utils/symbol_node.h"
#include "shared/dictionary.h"
#include "shared/utils.h"

// MARK: Numeric types

/**
 * @brief A base class for all numeric types.
 *
 * Includes `Type::Int` and `Type::Float`.
 */
class Type::INumeric : public Type {};

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
    // The width of the integer in bits. Can be any number, but should be 8, 16,
    // 32, or 64.
    const uint8_t width;

    Int(bool is_signed, uint8_t width)
        : is_signed(is_signed), width(width) {}

    std::string to_string() const override {
        return (is_signed ? "i" : "u") + std::to_string(width);
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_int = dynamic_cast<const Int*>(&other)) {
            return is_signed == other_int->is_signed &&
                   width == other_int->width;
        }
        return false;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        return llvm::IntegerType::get(builder->getContext(), width);
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

    Float(uint8_t width)
        : width(width) {
        if (width != 32 && width != 64) {
            panic(
                "Type::Float: Invalid width " + std::to_string(width) +
                ". Must be 32 or 64."
            );
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

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        switch (width) {
        case 32:
            return llvm::Type::getFloatTy(builder->getContext());
        case 64:
            return llvm::Type::getDoubleTy(builder->getContext());
        default:
            panic(
                "Type::Float: Invalid width " + std::to_string(width) +
                ". Must be 32 or 64."
            );
        }
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
    std::string to_string() const override { return "bool"; }

    bool operator==(const Type& other) const override {
        return dynamic_cast<const Bool*>(&other) != nullptr;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        return llvm::Type::getInt1Ty(builder->getContext());
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

    Pointer(std::shared_ptr<Type> base, bool is_mutable)
        : is_mutable(is_mutable), base(base) {}

    std::string to_string() const override {
        return std::string(is_mutable ? "var" : "") + "*" + base->to_string();
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_pointer = dynamic_cast<const Pointer*>(&other)) {
            return *base == *other_pointer->base;
        }
        return false;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        return llvm::PointerType::get(builder->getContext(), 0);
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

    Reference(std::shared_ptr<Type> base, bool is_mutable)
        : is_mutable(is_mutable), base(base) {}

    std::string to_string() const override {
        return std::string(is_mutable ? "var" : "") + "&" + base->to_string();
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_reference =
                dynamic_cast<const Reference*>(&other)) {
            return *base == *other_reference->base;
        }
        return false;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        return llvm::PointerType::get(builder->getContext(), 0);
    }
};

/**
 * @brief A primitive string type.
 *
 * The primitive string type is a pointer to a sequence of characters in static
 * memory. Primitive strings are immutable and live for as long as the program
 * runs.
 *
 * It is similar to the `char *` type in C, but is kept a separate type for
 * safety purposes such as to prevent pointer casting.
 */
class Type::Str : public Type {
public:
    std::string to_string() const override { return "str"; }

    bool operator==(const Type& other) const override {
        return dynamic_cast<const Str*>(&other) != nullptr;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        return llvm::PointerType::get(builder->getContext(), 0);
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

    Array(std::shared_ptr<Type> base)
        : base(base), size(std::nullopt) {}

    Array(std::shared_ptr<Type> base, size_t size)
        : base(base), size(size) {}

    std::string to_string() const override {
        return "[" + base->to_string() + "; " +
               (size ? std::to_string(*size) : "unknown") + "]";
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_array = dynamic_cast<const Array*>(&other)) {
            return *base == *other_array->base && size == other_array->size;
        }
        return false;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        if (size) {
            return llvm::ArrayType::get(base->get_llvm_type(builder), *size);
        }
        else {
            return llvm::PointerType::get(builder->getContext(), 0);
        }
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

    Tuple(std::vector<std::shared_ptr<Type>> elements)
        : elements(std::move(elements)) {}

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
            if (elements.size() != other_tuple->elements.size()) {
                return false;
            }
            for (size_t i = 0; i < elements.size(); ++i) {
                if (*(elements[i]) != *(other_tuple->elements[i])) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        std::vector<llvm::Type*> element_types;
        for (const auto& element : elements) {
            element_types.push_back(element->get_llvm_type(builder));
        }
        return llvm::StructType::get(builder->getContext(), element_types);
    }
};

/**
 * @brief A unit type.
 *
 * A unit type is a special tuple type that has no elements and is equivalent to
 * a tuple type with no elements. It is written as `()` and named "unit" because
 * it has only one possible value, which is `()`. It may be used to represent
 * the absence of a type, like `void` in other languages.
 *
 * This class does not override `Type::Tuple::operator==` and, thus, will appear
 * equal to other instances of `Type::Tuple` that have no elements. That is to
 * say, `Type::Unit` may be used interchangably with `Type::Tuple` with no
 * elements.
 */
class Type::Unit : public Type::Tuple {
public:
    Unit()
        : Tuple({}) {}

    std::string to_string() const override { return "()"; }

    llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        return llvm::StructType::get(builder->getContext());
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

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        std::vector<llvm::Type*> field_types;
        for (const auto& [key, value] : properties) {
            field_types.push_back(value.type->get_llvm_type(builder));
        }
        return llvm::StructType::get(builder->getContext(), field_types);
    }
};

// MARK: Special types

/**
 * @brief A named type.
 *
 * Used to represent types that have a name, such as complex types and aliased
 * types.
 *
 * Named types must point to a node in the symbol tree that is an instance of
 * Node::ITypeNode to be considered resolved. When converted to a string, the
 * unique name of the node is used.
 */
class Type::Named : public Type {
public:
    // The node associated with this named type; uses a weak pointer to avoid
    // circular references.
    const std::weak_ptr<Node::ITypeNode> node;

    Named(std::weak_ptr<Node::ITypeNode> node)
        : node(node) {
        if (node.expired()) {
            panic("Type::Named: Node cannot be null.");
        }
    }

    std::string to_string() const override {
        if (auto node_ptr = node.lock()) {
            return node_ptr->symbol;
        }
        return "<expired>";
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_named = dynamic_cast<const Named*>(&other)) {
            return node.lock() == other_named->node.lock();
        }
        return false;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        if (auto node_ptr = node.lock()) {
            llvm::StructType* struct_ty = llvm::StructType::getTypeByName(
                builder->getContext(),
                node_ptr->symbol
            );
            if (!struct_ty) {
                struct_ty = llvm::StructType::create(
                    builder->getContext(),
                    node_ptr->symbol
                );
            }
            return struct_ty;
        }
        panic("Type::Named: Node is expired; LLVM type cannot be generated.");
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
        : parameters(std::move(parameters)),
          return_type(std::move(return_type)) {}

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
        if (const auto* other_function =
                dynamic_cast<const Function*>(&other)) {
            return parameters == other_function->parameters &&
                   *return_type == *other_function->return_type;
        }
        return false;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        std::vector<llvm::Type*> param_types;
        for (const auto& param : parameters) {
            param_types.push_back(param.type->get_llvm_type(builder));
        }
        llvm::Type* return_llvm_type = return_type->get_llvm_type(builder);
        return llvm::FunctionType::get(return_llvm_type, param_types, false);
    }
};

#endif // NICO_TYPE_NODE_H
