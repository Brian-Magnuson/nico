#ifndef NICO_SYMBOL_NODE_H
#define NICO_SYMBOL_NODE_H

#include "nico/frontend/utils/nodes.h"
#include "nico/shared/dictionary.h"
#include "nico/shared/utils.h"

namespace nico {

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
    IBasicNode(
        std::weak_ptr<Node::IScope> parent_scope, const std::string& name
    )
        : Node(parent_scope, name) {}

public:
    virtual ~IBasicNode() = default;
};

/**
 * @brief A scope interface for nodes in the symbol tree.
 *
 * A scope may contain other nodes as children, allowing for a hierarchical
 * structure.
 */
class Node::IScope : public virtual Node::IBasicNode {
public:
    // A dictionary of child nodes, indexed by their name parts.
    Dictionary<std::string, std::shared_ptr<Node>> children;
    // A list of local scopes. Field entries cannot be accessed from outside
    // local scopes, so this is kept separate from children.
    std::vector<std::shared_ptr<Node::LocalScope>> local_scopes;

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
    // Note: These values won't actually be used since only derived classes will
    // call this constructor.
    {}
};

/**
 * @brief An interface for nodes that represent types in the symbol tree.
 *
 * Note: the node is not ready to use until `initialize_node()` is called.
 * This is because the type needs to be constructed from a weak pointer to this
 * node, which cannot be safely created in the constructor.
 */
class Node::ITypeNode : public virtual Node::IBasicNode {
public:
    std::shared_ptr<Type> type;

    virtual ~ITypeNode() = default;

protected:
    ITypeNode()
        : Node::IBasicNode(std::weak_ptr<Node::IScope>(), "") {}
};

/**
 * @brief An interfaces for nodes that reference a location in the source code.
 *
 * Locatable nodes are useful for error logging, allowing one to report where
 * a symbol was first declared.
 *
 * All nodes that have some "name" associated with them are locatable.
 * This includes all node kinds except Node::RootScope, Node::PrimitiveType, and
 * Node::LocalScope.
 */
class Node::ILocatable : public virtual Node::IBasicNode {
public:
    // The original token used to create this node.
    std::shared_ptr<Token> location_token;

    virtual ~ILocatable() = default;

protected:
    ILocatable(std::shared_ptr<Token> token)
        : Node::IBasicNode(std::weak_ptr<Node::IScope>(), ""),
          location_token(token) {}
};

/**
 * @brief The root scope of the symbol tree.
 *
 * The root scope is the top-level scope that contains all other scopes.
 * Its unique identifier is always "::" and the pointer to its parent scope is
 * empty.
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
 * Namespace scopes are used to group related symbols together and avoid naming
 * conflicts. It is a kind of global scope.
 *
 * Unlike struct definitions, namespaces may be closed and reopened in another
 * location. They may also be nested within other namespaces, including
 * namespaces with the same name (though not recommended; name resolution will
 * be done based on the searching algorithm).
 *
 * A namespace may not be declared within a local scope or a struct definition.
 */
class Node::Namespace : public virtual Node::IGlobalScope,
                        public virtual Node::ILocatable {
public:
    virtual ~Namespace() = default;

    Namespace(
        std::weak_ptr<Node::IScope> parent_scope, std::shared_ptr<Token> token
    )
        : Node::IBasicNode(parent_scope, std::string(token->lexeme)),
          Node::IScope(parent_scope, std::string(token->lexeme)),
          Node::IGlobalScope(),
          Node::ILocatable(token) {}
};

/**
 * @brief A primitive type in the symbol tree.
 *
 * A primitive type node references a basic type object instead of a custom
 * type. This allows the type checker to look up basic types as if they were any
 * other named type.
 *
 * Unlike Node::StructDef, the type object is constructed *before* the node
 * rather than after. This is possible since the basic types do not need to
 * reference any nodes in the symbol tree.
 *
 * Ideally, the symbol tree should install primitive types in the root scope.
 */
class Node::PrimitiveType : public virtual Node::ITypeNode {
public:
    virtual ~PrimitiveType() = default;

    PrimitiveType(
        std::weak_ptr<Node::IScope> parent_scope,
        const std::string& name,
        std::shared_ptr<Type> type
    )
        : Node::IBasicNode(parent_scope, name), Node::ITypeNode() {
        if (type == nullptr) {
            panic("Node::PrimitiveType: Type cannot be null.");
        }
        this->type = type;
    }
};

/**
 * @brief A struct definition scope in the symbol tree.
 *
 * Struct definitions are used to define custom data types with fields and
 * methods. It is a kind of global scope.
 *
 * Unlike namespaces, struct definitions cannot be closed and reopened in
 * another location. They also cannot be nested within a struct of the same
 * name.
 *
 * A struct may not be declared within a local scope.
 */
class Node::StructDef : public virtual Node::IGlobalScope,
                        public virtual Node::ITypeNode,
                        public virtual Node::ILocatable {
public:
    // Whether this struct is declared with `class` or not. Classes may follow
    // different semantic rules than structs, such as memory management.
    const bool is_class = false;
    // A dictionary of properties (fields) in this struct, indexed by their
    // names.
    Dictionary<std::string, Field> properties;
    // A dictionary of methods in this struct, indexed by their names. Methods
    // are also stored as fields, but are never `var` and always have a type of
    // `Function`.
    Dictionary<std::string, Field> methods;

    virtual ~StructDef() = default;

    StructDef(
        std::weak_ptr<Node::IScope> parent_scope,
        std::shared_ptr<Token> token,
        bool is_class = false
    )
        : Node::IBasicNode(parent_scope, std::string(token->lexeme)),
          Node::IScope(parent_scope, std::string(token->lexeme)),
          Node::IGlobalScope(),
          Node::ITypeNode(),
          Node::ILocatable(token),
          is_class(is_class) {}
};

/**
 * @brief A local scope node in the symbol tree.
 *
 * Local scopes are used to define variables and functions that are only
 * accessible within a specific block of code. They do not have names; their
 * unique identifiers are generated using numbers, which increment with each new
 * local scope created. They are not global scopes and cannot contain other
 * global scopes.
 *
 * As a side effect of having only numbers as identifiers, it is impossible to
 * reference a variable declared in a local scope from outside that scope (since
 * an identifier expression cannot start with a number).
 */
class Node::LocalScope : public virtual Node::IScope {
public:
    // A static counter to generate unique identifiers for local scopes.
    static int next_scope_id;
    // The type of the expressions yielded within this local scope.
    // If this is the top local scope, this is the return type of the function.
    std::optional<std::shared_ptr<Type>> yield_type;
    // The top local scope in the parent chain. Can be this. The memory for this
    // node is managed by its parent; do not manually delete it.
    Node::LocalScope* top_local_scope;

    virtual ~LocalScope() = default;

    LocalScope(std::weak_ptr<Node::IScope> parent_scope)
        : Node::IBasicNode(parent_scope, std::to_string(next_scope_id++)),
          Node::IScope(parent_scope, std::to_string(next_scope_id++)) {
        if (auto parent_local_scope =
                std::dynamic_pointer_cast<Node::LocalScope>(
                    parent_scope.lock()
                )) {
            top_local_scope = parent_local_scope->top_local_scope;
        }
        else {
            top_local_scope = this;
        }
    }
};

/**
 * @brief A function scope node in the symbol tree.
 *
 * A function scope is a special local scope created when a function is
 * declared. It stores the function's parameters and tracks the function's
 * return type.
 */
class Node::FunctionScope : public virtual Node::LocalScope,
                            public virtual Node::ILocatable {
public:
    FunctionScope(
        std::weak_ptr<Node::IScope> parent_scope, std::shared_ptr<Token> token
    )
        : Node::IBasicNode(parent_scope, std::string(token->lexeme)),
          Node::IScope(parent_scope, std::string(token->lexeme)),
          Node::LocalScope(parent_scope),
          Node::ILocatable(token) {}
};

/**
 * @brief A field entry in the symbol tree.
 *
 * Field entries are any variable declared with `let` or any function declared
 * with `func`.
 *
 * Field objects carry a type object, and must therefore have their types
 * resolved before being constructed.
 */
class Node::FieldEntry : public virtual Node::IBasicNode,
                         public virtual Node::ILocatable {
public:
    // Whether this field entry is declared in a global scope or not.
    const bool is_global;
    // The field object that this entry represents.
    Field field;
    // If this field is a local variable, the pointer to the LLVM allocation.
    llvm::AllocaInst* llvm_ptr = nullptr;

    virtual ~FieldEntry() = default;

    FieldEntry(std::weak_ptr<Node::IScope> parent_scope, const Field& field)
        : Node::IBasicNode(parent_scope, std::string(field.token->lexeme)),
          Node::ILocatable(field.token),
          is_global(PTR_INSTANCEOF(parent_scope.lock(), Node::IGlobalScope)),
          field(field) {}

    /**
     * @brief Gets the LLVM allocation for this field entry.
     *
     * If the field is global, this function will attempt to get the global
     * variable. If the field is local, it will return the LLVM pointer stored
     * in the node.
     *
     * @param builder The IRBuilder used to create the allocation if needed.
     */
    llvm::Value*
    get_llvm_allocation(std::unique_ptr<llvm::IRBuilder<>>& builder) {
        llvm::Value* ptr;
        if (is_global) {
            auto ir_module = builder->GetInsertBlock()->getModule();
            // Attempt to get the global variable.
            ptr = ir_module->getGlobalVariable(field.token->lexeme, true);
            // If it doesn't exist, declare it.
            if (ptr == nullptr) {
                ptr = new llvm::GlobalVariable(
                    *ir_module,
                    field.type->get_llvm_type(builder),
                    false, // isConstant
                    llvm::GlobalValue::ExternalLinkage,
                    nullptr, // Initializer
                    field.token->lexeme
                );
            }
        }
        else {
            ptr = llvm_ptr;
            if (ptr == nullptr) {
                panic(
                    "Node::FieldEntry::get_llvm_allocation: Local variable has "
                    "no LLVM allocation."
                );
            }
        }
        return ptr;
    }
};

} // namespace nico

#endif // NICO_SYMBOL_NODE_H
