#ifndef NICO_TYPE_H
#define NICO_TYPE_H

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../common/dictionary.h"
#include "../common/utils.h"
#include "../lexer/token.h"

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

    // TODO: Remove this declaration.
    // Non-declarable types
    // class StructDef;

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
 *
 * Fields use type objects, and thus, must have their types properly resolved before constructed.
 */
class Field {
public:
    // Whether the field is declared with `var` or not.
    const bool is_var;
    // The name of the field.
    const std::shared_ptr<Token> token;
    // The type of the field.
    const std::shared_ptr<Type> type;

    virtual ~Field() = default;

    Field(bool is_var, std::shared_ptr<Token> token, std::shared_ptr<Type> type)
        : is_var(is_var), token(token), type(type) {
        if (type == nullptr) {
            panic("Field::Field: Type cannot be null.");
        }
    }

    /**
     * @brief Returns a string representation of the field.
     *
     * @return std::string A string representation of the field.
     */
    virtual std::string to_string() const {
        return (is_var ? "var " : "") + std::string(token->lexeme) + ": " + type->to_string();
    }

    bool operator==(const Field& other) const {
        return is_var == other.is_var && token->lexeme == other.token->lexeme && *type == *other.type;
    }
};

/**
 * @brief The base class for all nodes in the symbol tree.
 *
 * All nodes in the symbol tree have a unique name to identify them.
 * Most subclasses of Node inherit from Node::IScope, meaning they have other nodes as children.
 *
 * Do not extend this class directly; use Node::IBasicNode instead.
 */
class Node : public std::enable_shared_from_this<Node> {
public:
    class IBasicNode;

    class IScope;
    class IGlobalScope;
    class ITypeNode;

    class RootScope;
    class Namespace;
    class PrimitiveType;
    class StructDef;
    class LocalScope;
    class FieldEntry;

    // This node's parent scope, if it exists.
    std::weak_ptr<Node::IScope> parent;
    // This node's unique name, assigned upon construction.
    const std::string unique_name;
    // A short name for this node, used for adding this node to the parent node's children.
    const std::string short_name;

    virtual ~Node() = default;

protected:
    Node(std::weak_ptr<Node::IScope> parent_scope, const std::string& name);

public:
    /**
     * @brief Adds this node to its parent scope's children.
     *
     * If this node is an instance of Node::RootScope, this function does nothing.
     *
     * If this node is an instance of Node::ITypeNode, it will also set the type of the node to a Named type that references this node.
     *
     * Should be called immediately after constructing a node that is part of a scope.
     */
    void initialize_node();
};

// MARK: Nodes

/**
 * @brief A basic node in the symbol tree.
 *
 * This class adds no additional members to Node.
 * Its purpose is to be the sole subclass of Node, thereby preventing Node from
 * being a diamond inheritance base class, which would otherwise result in
 * ambiguity when using `shared_from_this()`.
 */
class Node::IBasicNode : public Node {
protected:
    IBasicNode(std::weak_ptr<Node::IScope> parent_scope, const std::string& name)
        : Node(parent_scope, name) {}

public:
    virtual ~IBasicNode() = default;
};

/**
 * @brief A scope interface for nodes in the symbol tree.
 *
 * A scope may contain other nodes as children, allowing for a hierarchical structure.
 */
class Node::IScope : public virtual Node::IBasicNode {
public:
    // A dictionary of child nodes, indexed by their name parts.
    Dictionary<std::string, std::shared_ptr<Node>> children;

    virtual ~IScope() = default;

protected:
    IScope(std::weak_ptr<Node::IScope> parent_scope, const std::string& name)
        : Node::IBasicNode(parent_scope, name) {}
};

/**
 * @brief A global scope interface for nodes in the symbol tree.
 *
 * Global scopes include the root scope, namespaces, and struct definitions.
 *
 * The global scope interface adds no additional members to IScope.
 * Rather, it is used to distinguish global scopes from local scopes.
 */
class Node::IGlobalScope : public virtual Node::IScope {
public:
    virtual ~IGlobalScope() = default;

protected:
    IGlobalScope()
        : Node::IBasicNode(std::weak_ptr<Node::IScope>(), ""),
          Node::IScope(std::weak_ptr<Node::IScope>(), "")
    // Note: These values won't actually be used since only derived classes will call this constructor.
    {}
};

/**
 * @brief An interface for nodes that represent types in the symbol tree.
 *
 * Note: For the purpose of avoiding diamond inheritance, this interface does not extend Node. Despite this, it is still used as a base class for type nodes.
 */
class Node::ITypeNode : public virtual Node::IBasicNode {
public:
    std::shared_ptr<Type> type;

    virtual ~ITypeNode() = default;

protected:
    ITypeNode()
        : Node::IBasicNode(std::weak_ptr<Node::IScope>(), "") {
    }
};

/**
 * @brief The root scope of the symbol tree.
 *
 * The root scope is the top-level scope that contains all other scopes.
 * Its unique identifier is always "::" and the pointer to its parent scope is empty.
 */
class Node::RootScope : public virtual Node::IGlobalScope {
public:
    virtual ~RootScope() = default;

    RootScope()
        : Node::IBasicNode(std::weak_ptr<Node::IScope>(), "::"),
          Node::IScope(std::weak_ptr<Node::IScope>(), "::"),
          Node::IGlobalScope() {}
};

/**
 * @brief A namespace scope in the symbol tree.
 *
 * Namespace scopes are used to group related symbols together and avoid naming conflicts.
 * It is a kind of global scope.
 *
 * Unlike struct definitions, namespaces may be closed and reopened in another location.
 * They may also be nested within other namespaces, including namespaces with the same name (though not recommended; name resolution will be done based on the searching algorithm).
 *
 * A namespace may not be declared within a local scope or a struct definition.
 */
class Node::Namespace : public virtual Node::IGlobalScope {
public:
    virtual ~Namespace() = default;

    Namespace(std::weak_ptr<Node::IScope> parent_scope, const std::string& name)
        : Node::IBasicNode(parent_scope, name),
          Node::IScope(parent_scope, name),
          Node::IGlobalScope() {
    }
};

/**
 * @brief A struct definition scope in the symbol tree.
 *
 * Struct definitions are used to define custom data types with fields and methods.
 * It is a kind of global scope.
 *
 * Unlike namespaces, struct definitions cannot be closed and reopened in another location. They also cannot be nested within a struct of the same name.
 *
 * A struct may not be declared within a local scope.
 */
class Node::StructDef : public virtual Node::IGlobalScope, public virtual Node::ITypeNode {
public:
    // Whether this struct is declared with `class` or not. Classes may follow different semantic rules than structs, such as memory management.
    const bool is_class = false;
    // A dictionary of properties (fields) in this struct, indexed by their names.
    Dictionary<std::string, Field> properties;
    // A dictionary of methods in this struct, indexed by their names. Methods are also stored as fields, but are never `var` and always have a type of `Function`.
    Dictionary<std::string, Field> methods;

    virtual ~StructDef() = default;

    StructDef(std::weak_ptr<Node::IScope> parent_scope, const std::string& name, bool is_class = false)
        : Node::IBasicNode(parent_scope, name),
          Node::IScope(parent_scope, name),
          Node::IGlobalScope(),
          Node::ITypeNode(),
          is_class(is_class) {}
};

/**
 * @brief A local scope node in the symbol tree.
 *
 * Local scopes are used to define variables and functions that are only accessible within a specific block of code.
 * They do not have names; their unique identifiers are generated using numbers, which increment with each new local scope created.
 * They are not global scopes and cannot contain other global scopes.
 *
 * As a side effect of having only numbers as identifiers, it is impossible to reference a variable declared in a local scope from outside that scope (since an identifier expression cannot start with a number).
 */
class Node::LocalScope : public virtual Node::IScope {
public:
    static int next_scope_id;

    LocalScope(std::weak_ptr<Node::IScope> parent_scope)
        : Node::IBasicNode(parent_scope, std::to_string(next_scope_id++)),
          Node::IScope(parent_scope, std::to_string(next_scope_id++)) {}
};

/**
 * @brief A field entry in the symbol tree.
 *
 * Field entries are any variable declared with `let` or any function declared with `func`.
 *
 * Field objects carry a type object, and must therefore have their types resolved before being constructed.
 */
class Node::FieldEntry : public virtual Node::IBasicNode {
public:
    // The field object that this entry represents.
    Field field;

    FieldEntry(std::weak_ptr<Node::IScope> parent_scope, const Field& field)
        : Node::IBasicNode(parent_scope, std::string(field.token->lexeme)), field(field) {}
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
            panic("Type::Float: Invalid width " + std::to_string(width) + ". Must be 32 or 64.");
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

    Pointer(std::shared_ptr<Type> base, bool is_mutable) : is_mutable(is_mutable), base(base) {}

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

    Reference(std::shared_ptr<Type> base, bool is_mutable) : is_mutable(is_mutable), base(base) {}

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
    // The node associated with this named type; uses a weak pointer to avoid circular references.
    const std::weak_ptr<Node::ITypeNode> node;

    Named(std::weak_ptr<Node::ITypeNode> node)
        : node(node) {
        if (node.expired()) {
            panic("Type::Named: Node cannot be null.");
        }
    }

    std::string to_string() const override {
        if (auto node_ptr = node.lock()) {
            return node_ptr->unique_name;
        }
        return "<expired>";
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_named = dynamic_cast<const Named*>(&other)) {
            return node.lock() == other_named->node.lock();
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

#endif // NICO_TYPE_H
