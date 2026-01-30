#ifndef NICO_NODES_H
#define NICO_NODES_H

#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>

#include "nico/shared/token.h"
#include "nico/shared/utils.h"

namespace nico {

class SymbolTree;

// MARK: Node

/**
 * @brief A node in the symbol tree.
 *
 * Symbol tree nodes are used to store information whenever a new symbol is
 * introduced in the source code. Theoretically, every declaration should result
 * in only one node in the symbol tree, so nodes may be compared directly for
 * equality.
 *
 * All nodes in the symbol tree have a unique symbol to identify them.
 * Most subclasses of Node inherit from Node::IScope, meaning they have other
 * nodes as children.
 *
 * Nodes may require additional initialization after construction to ensure
 * parent references are set up correctly. Please use `initialize_node()`
 * immediately after constructing nodes.
 */
class Node : public std::enable_shared_from_this<Node> {
public:
    class IScope;
    class IGlobalScope;
    class ITypeNode;
    class ILocatable;

    class RootScope;
    class Namespace;
    class PrimitiveType;
    class StructDef;
    class LocalScope;
    class FieldEntry;
    class OverloadGroup;

    // This node's parent scope, if it exists.
    std::weak_ptr<Node::IScope> parent;
    // A short name for this node, used for adding this node to the parent
    // node's children.
    std::string short_name;

    virtual ~Node() = default;

protected:
    // This node's unique symbol, assigned upon construction.
    std::string symbol;

    /**
     * @brief A private struct used to restrict access to constructors.
     */
    struct Private {
        explicit Private() = default;
    };

    Node(Private) {}

public:
    /**
     * @brief Returns a string representation of this node.
     *
     * @return A string representation of this node.
     */
    virtual std::string to_string() const = 0;

    /**
     * @brief Sets the symbol of this node.
     *
     * The symbol must be unique for the entire symbol tree.
     * If the symbol is already in use, this function returns false and the
     * symbol is not changed.
     *
     * @param symbol_tree The symbol tree to which this node belongs.
     * @param new_symbol The new symbol to set.
     * @return True if the symbol was set successfully. False otherwise.
     */
    bool set_symbol(const SymbolTree* symbol_tree, std::string_view new_symbol);

    /**
     * @brief Sets the symbol of this node using its parent's symbol.
     *
     * Will fail if the parent pointer is empty/expired or if the symbol is not
     * unique.
     *
     * @param symbol_tree The symbol tree to which this node belongs.
     * @return True if the symbol was set successfully. False otherwise.
     */
    bool set_symbol_using_parent(const SymbolTree* symbol_tree);

    /**
     * @brief Retrieves the unique symbol of this node.
     *
     * @return The unique symbol of this node.
     */
    const std::string& get_symbol() const { return symbol; }

    /**
     * @brief Returns a string representation of the subtree rooted at this
     * node.
     *
     * @param indent The current indentation level. Defaults to 0.
     * @return A string representation of the subtree rooted at this node.
     */
    virtual std::string to_tree_string(size_t indent = 0) const {
        std::string indent_str(indent, ' ');
        return indent_str + to_string() + "\n";
    };
};

// MARK: Type

/**
 * @brief A type object.
 *
 * This class serves as the base class for all types in the symbol tree.
 * Type objects are used to represent the resolved types of expressions and
 * variables. They should not be confused with annotation objects, which are
 * part of the AST and represent unresolved types. They should not be used in
 * the parser, except when the expression is a literal value, such as an
 * integer.
 *
 * Types can be compared for equality, converted to a unique string, and
 * converted to an equivalent LLVM type.
 *
 * Note that LLVM types may not carry less information than the type object from
 * which it was generated. Thus, care should be taken when converting between
 * the two.
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
    class IPointer;
    class IRawPtr;
    class Nullptr;
    class Anyptr;
    class ITypedPtr;
    class RawTypedPtr;
    class Reference;
    class Str;

    // Aggregate types
    class Array;
    class EmptyArray;
    class Tuple;
    class Unit;
    class Object;

    // Callable types
    class ICallable;
    class Function;
    class OverloadedFn;

    // Special types
    class Named;

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
     * This operator does not consider if one type can be implicitly converted
     * to another.
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

    /**
     * @brief Generates the corresponding LLVM type for this type object.
     *
     * If this type is a named type, only the name will be used to create the
     * type. The type definition should be written elsewhere during code
     * generation.
     *
     * @param builder The LLVM IR builder to use for generating the type.
     * @return The corresponding LLVM type for this type object.
     */
    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const = 0;

    /**
     * @brief Generate the arguments for printing a value of this type.
     *
     * This method is used to generate the format string and arguments for
     * printing a value of this type using `printf`.
     *
     * If the type does not have a specific way to be printed, "[object]" will
     * be used as the format string as a fallback.
     *
     * @param builder The LLVM IR builder to use for generating the type.
     * @param value The LLVM value to print.
     * @param include_quotes Whether or not to include quotes around string
     * values.
     * @return A pair containing the format string and a vector of LLVM values
     * to use as arguments for the format string.
     */
    virtual std::pair<std::string, std::vector<llvm::Value*>> to_print_args(
        std::unique_ptr<llvm::IRBuilder<>>& builder,
        llvm::Value* value,
        bool include_quotes = false
    ) const {
        return {"[object]", {}};
    }

    /**
     * @brief Check if this type is assignable a binding of the target type.
     *
     * For clarification, for the assignment `a = b`, this checks if the type of
     * `b` (this) is assignable to the type of `a` (other).
     *
     * This function is assymmetric; that is, `a.is_assignable_to(b)` may not
     * return the same result as `b.is_assignable_to(a)`.
     *
     * For most cases, this is equivalent to checking for equality.
     * For pointer types, assigning a mutable pointer to an immutable pointer is
     * allowed.
     *
     * @param target_type The target type to check assignability against.
     * @return True if this type is assignable to the target type. False
     * otherwise.
     */
    virtual bool is_assignable_to(const Type& target_type) const {
        return *this == target_type;
    }

    /**
     * @brief Check if this type is sized, i.e., has a known size at compile
     * time.
     *
     * Sized types are required for memory allocation.
     *
     * Most types are sized. An example of an unsized type is `[T; ?]`.
     *
     * @return True if the type is sized. False otherwise.
     */
    virtual bool is_sized_type() const { return true; }

    /**
     * @brief Get the size of the LLVM type in bytes corresponding to this type.
     *
     * Internally, this function calls `get_llvm_type` and uses the data layout
     * of the module.
     *
     * Note: This is NOT the size of the type object itself, but the size of the
     * LLVM type that this object represents. For example, if this type object
     * represents the `i32` LLVM type, this function will return 4.
     *
     * @param builder The LLVM IR builder to use for generating the type.
     * @return The size of the LLVM type in bytes.
     */
    size_t
    get_llvm_type_size(std::unique_ptr<llvm::IRBuilder<>>& builder) const {
        llvm::Type* llvm_type = get_llvm_type(builder);
        llvm::DataLayout data_layout =
            builder->GetInsertBlock()->getModule()->getDataLayout();
        return data_layout.getTypeAllocSize(llvm_type);
    }
};

// MARK: Stmt

/**
 * @brief A statement AST node.
 *
 * Statements are pieces of code that do not evaluate to a value.
 * Includes the expression statement, declarations, and non-declaring
 * statements.
 */
class Stmt : public std::enable_shared_from_this<Stmt> {
public:
    class Expression;

    class Let;
    class Func;

    class Print;
    class Dealloc;

    class Pass;
    class Yield;
    class Continue;

    class Eof;

    virtual ~Stmt() {}

    /**
     * @brief A visitor class for statements.
     */
    class Visitor {
    public:
        virtual std::any visit(Expression* stmt) = 0;
        virtual std::any visit(Let* stmt) = 0;
        virtual std::any visit(Func* stmt) = 0;
        virtual std::any visit(Print* stmt) = 0;
        virtual std::any visit(Dealloc* stmt) = 0;
        virtual std::any visit(Pass* stmt) = 0;
        virtual std::any visit(Yield* stmt) = 0;
        virtual std::any visit(Continue* stmt) = 0;
        virtual std::any visit(Eof* stmt) = 0;
    };

    /**
     * @brief Accept a visitor.
     *
     * @param visitor The visitor to accept.
     * @return The return value from the visitor.
     */
    virtual std::any accept(Visitor* visitor) = 0;
};

// MARK: Expr

/**
 * @brief An expression AST node.
 *
 * Expressions evaluate to a value.
 */
class Expr : public std::enable_shared_from_this<Expr> {
public:
    class IPLValue;

    class Assign;
    class Logical;
    class Binary;
    class Unary;
    class Address;
    class Deref;
    class Cast;
    class Access;
    class Subscript;
    class Call;
    class SizeOf;
    class Alloc;
    class NameRef;
    class Literal;

    class Tuple;
    class Unit;
    class Array;

    class Block;
    class Conditional;
    class Loop;

    virtual ~Expr() {}

    /**
     * @brief A visitor class for expressions.
     */
    class Visitor {
    public:
        virtual std::any visit(Assign* expr, bool as_lvalue) = 0;
        virtual std::any visit(Logical* expr, bool as_lvalue) = 0;
        virtual std::any visit(Binary* expr, bool as_lvalue) = 0;
        virtual std::any visit(Unary* expr, bool as_lvalue) = 0;
        virtual std::any visit(Address* expr, bool as_lvalue) = 0;
        virtual std::any visit(Deref* expr, bool as_lvalue) = 0;
        virtual std::any visit(Cast* expr, bool as_lvalue) = 0;
        virtual std::any visit(Access* expr, bool as_lvalue) = 0;
        virtual std::any visit(Subscript* expr, bool as_lvalue) = 0;
        virtual std::any visit(Call* expr, bool as_lvalue) = 0;
        virtual std::any visit(SizeOf* expr, bool as_lvalue) = 0;
        virtual std::any visit(Alloc* expr, bool as_lvalue) = 0;
        virtual std::any visit(NameRef* expr, bool as_lvalue) = 0;
        virtual std::any visit(Literal* expr, bool as_lvalue) = 0;
        virtual std::any visit(Tuple* expr, bool as_lvalue) = 0;
        virtual std::any visit(Array* expr, bool as_lvalue) = 0;
        virtual std::any visit(Block* expr, bool as_lvalue) = 0;
        virtual std::any visit(Conditional* expr, bool as_lvalue) = 0;
        virtual std::any visit(Loop* expr, bool as_lvalue) = 0;
    };

    // The type of the expression.
    std::shared_ptr<Type> type;
    // The location of the expression.
    const Location* location;

    /**
     * @brief Accept a visitor.
     *
     * @param visitor The visitor to accept.
     * @param as_lvalue Whether or not the expression should be treated as an
     * lvalue.
     * @return The return value from the visitor.
     */
    virtual std::any accept(Visitor* visitor, bool as_lvalue) = 0;
};

// MARK: Annotation

/**
 * @brief An annotation AST node.
 *
 * An annotation object is used in the AST to organize parts of the type
 * annotation. Annotations are effectively unresolved types, which can be
 * resolved to proper type objects in the type checker. It should not be
 * confused with a type object, which represents the resolved type of an
 * expression.
 *
 * Type annotations are not designed to be compared with each other; comparing
 * types should only be done after resolution.
 */
class Annotation {
public:
    class NameRef;

    class Pointer;
    class Nullptr;
    class Reference;

    class Array;
    class Object;
    class Tuple;

    class TypeOf;

    virtual ~Annotation() = default;

    /**
     * @brief A visitor class for annotations.
     */
    class Visitor {
    public:
        virtual std::any visit(NameRef* annotation) = 0;
        virtual std::any visit(Pointer* annotation) = 0;
        virtual std::any visit(Nullptr* annotation) = 0;
        virtual std::any visit(Reference* annotation) = 0;
        virtual std::any visit(Array* annotation) = 0;
        virtual std::any visit(Object* annotation) = 0;
        virtual std::any visit(Tuple* annotation) = 0;
        virtual std::any visit(TypeOf* annotation) = 0;
    };

    // The location of the expression.
    const Location* location;

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
     * The string representation is not unique and should not be used to compare
     * types.
     *
     * @return A string representation of the annotation.
     */
    virtual std::string to_string() const { return "[unknown]"; }
};

// MARK: Name

/**
 * @brief A name class used to represent names with multiple parts.
 *
 * Name should only be used where multi-part names are allowed.
 * Multi-part names are not allowed in declarations, but are in name expressions
 * and annotations.
 *
 * Names should not be compared directly as different names may refer to the
 * same thing and similar names may refer to different things. Instead, search
 * for the name in the symbol tree and resolve it to a node.
 */
class Name {
public:
    /**
     * @brief A part of a name.
     *
     * Consists of the token representing the part and a vector of arguments.
     *
     * E.g. `example::object<with, args>` would have two parts:
     * - The first part would be `example` with no arguments.
     * - The second part would be `object` with two arguments: `with` and
     * `args`.
     */
    struct Part {
        // The token representing this part of the name.
        std::shared_ptr<Token> token;
        // The arguments for this part of the name, if any.
        std::vector<std::shared_ptr<Name>> args;
    };

    // The parts of the name.
    std::vector<Part> parts;

    Name(std::shared_ptr<Token> token)
        : parts({{token, {}}}) {}

    Name(std::vector<Part> elements)
        : parts(elements) {
        if (parts.empty()) {
            panic("Name::Name: parts cannot be empty");
        }
    }

    /**
     * @brief Converts this name to a string representation.
     *
     * @return std::string The string representation of the name.
     */
    std::string to_string() const {
        std::string result = "";

        // example::object<with, args>
        for (size_t i = 0; i < parts.size(); ++i) {
            const auto& part = parts[i];
            result += part.token->lexeme;
            if (!part.args.empty()) {
                result += "<";
                for (size_t j = 0; j < part.args.size(); ++j) {
                    result += part.args[j]->to_string();
                    if (j < part.args.size() - 1) {
                        result += ", ";
                    }
                }
                result += ">";
            }
            if (i < parts.size() - 1) {
                result += "::";
            }
        }
        return result;
    }
};

// MARK: Field

/**
 * @brief A multi-purpose field class.
 *
 * Used to represent properties or shared variables in complex types,
 * properties in objects, and parameters in functions.
 *
 * Fields use type objects, and thus, must have their types properly resolved
 * before constructed.
 */
class Field {
public:
    // Whether the field is declared with `var` or not.
    bool is_var;
    // The name of the field.
    // std::shared_ptr<Token> token;
    // The name of the field.
    std::string name;
    // The location where the field is introduced.
    const Location* location;
    // The type of the field.
    std::shared_ptr<Type> type;
    // The default expression for the field, if any.
    std::optional<std::weak_ptr<Expr>> default_expr;

    virtual ~Field() = default;

    Field(
        bool is_var,
        std::string_view name,
        const Location* location,
        std::shared_ptr<Type> type,
        std::optional<std::weak_ptr<Expr>> default_expr = std::nullopt
    )
        : is_var(is_var),
          name(name),
          location(location),
          type(type),
          default_expr(default_expr) {
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
        return (is_var ? "var " : "") + name + ": " + type->to_string();
    }

    /**
     * @brief Checks if this field is equivalent to another field.
     *
     * Fields are considered equivalent if they have the same `is_var` status,
     * the same name, and the same type. The token does not necessarily have to
     * be the same; just the lexeme.
     *
     * @param other The other field to compare.
     * @return True if the fields are equivalent, false otherwise.
     */
    bool operator==(const Field& other) const {
        return is_var == other.is_var && name == other.name &&
               *type == *other.type;
    }
};

} // namespace nico

#endif // NICO_NODES_H
