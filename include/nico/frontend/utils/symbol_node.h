#ifndef NICO_SYMBOL_NODE_H
#define NICO_SYMBOL_NODE_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/nodes.h"
#include "nico/frontend/utils/symbol_tree.h"
#include "nico/shared/dictionary.h"
#include "nico/shared/error_code.h"
#include "nico/shared/utils.h"

namespace nico {

/**
 * @brief A scope interface for nodes in the symbol tree.
 *
 * A scope may contain other nodes as children, allowing for a hierarchical
 * structure.
 */
class Node::IScope : public virtual Node {
public:
    // A dictionary of child nodes, indexed by their name parts.
    Dictionary<std::string, std::shared_ptr<Node>> children;
    // A list of local scopes. Binding entries cannot be accessed from outside
    // local scopes, so this is kept separate from children.
    std::vector<std::shared_ptr<Node::LocalScope>> local_scopes;

    virtual ~IScope() = default;

protected:
    IScope(Private)
        : Node(Private()) {}

public:
    virtual std::string to_tree_string(size_t indent = 0) const override;

    /**
     * @brief Adds a child node to this scope.
     *
     * If successful, the child node will be added to this scope and registered
     * in the provided symbol tree.
     *
     * This function will check if the child node's name is reserved, if its
     * symbol is reserved, and if its name already exists in this scope. If any
     * of these checks fail, the child node will not be added and an error will
     * be logged.
     *
     * @param symbol_tree A reference to the symbol tree, used for registering
     * the child's symbol.
     * @param child The child node to add.
     * @param custom_symbol An optional custom symbol name to use for
     * registration.
     * @return True if the child was successfully added, false otherwise.
     */
    virtual bool add_child(
        SymbolTree& symbol_tree,
        std::shared_ptr<Node::ILocatable> child,
        std::optional<std::string> custom_symbol = std::nullopt
    );
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
    IGlobalScope(Private)
        : Node(Private()) {}
};

/**
 * @brief An interface for nodes that represent types in the symbol tree.
 *
 * Note: the node is not ready to use until `initialize_node()` is called.
 * This is because the type needs to be constructed from a weak pointer to this
 * node, which cannot be safely created in the constructor.
 */
class Node::ITypeNode : public virtual Node {
public:
    std::shared_ptr<Type> type;

    virtual ~ITypeNode() = default;

protected:
    ITypeNode(Private)
        : Node(Private()) {}
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
class Node::ILocatable : public virtual Node {
public:
    // The location in the source code where this node is introduced.
    const Location* location;

    virtual ~ILocatable() = default;

protected:
    ILocatable(Private, const Location* location)
        : Node(Private()), location(location) {}

    ILocatable(Private)
        : Node(Private()) {}
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

    RootScope(Private)
        : Node(Private()),
          Node::IScope(Private()),
          Node::IGlobalScope(Private()) {}

    /**
     * @brief Creates a new root scope node.
     *
     * @param short_name The short name of the root scope. Defaults to an empty
     * string.
     * @return A shared pointer to the newly created root scope node.
     */
    static std::shared_ptr<RootScope> create(std::string_view short_name = "") {
        auto node = std::make_shared<RootScope>(Private());
        node->short_name = std::string(short_name);
        return node;
    }

    virtual std::string to_string() const override {
        return "ROOT \"" + symbol + "\"";
    }
};

/**
 * @brief A namespace scope in the symbol tree.
 *
 * Namespace scopes are used to group related symbols together and avoid
 * naming conflicts. It is a kind of global scope.
 *
 * Unlike struct definitions, namespaces may be closed and reopened in
 * another location. They may also be nested within other namespaces,
 * including namespaces with the same name (though not recommended; name
 * resolution will be done based on the searching algorithm).
 *
 * A namespace may not be declared within a local scope or a struct
 * definition.
 */
class Node::Namespace : public virtual Node::IGlobalScope,
                        public virtual Node::ILocatable {
public:
    virtual ~Namespace() = default;

    Namespace(Private)
        : Node(Private()),
          Node::IScope(Private()),
          Node::IGlobalScope(Private()),
          Node::ILocatable(Private()) {}

    /**
     * @brief Creates a new namespace node and adds it to the parent scope.
     *
     * @param parent The parent scope in which to add the namespace.
     * @param token The token representing the namespace identifier.
     * @return A shared pointer to the newly created namespace node.
     */
    static std::shared_ptr<Namespace>
    create(std::shared_ptr<Node::IScope> parent, std::shared_ptr<Token> token);

    virtual std::string to_string() const override {
        return "NS \"" + symbol + "\"";
    }
};

/**
 * @brief An extern block scope in the symbol tree.
 *
 * Extern blocks are used to group together symbols that are defined in an
 * external library. They are similar to namespaces, but with some restrictions:
 * They are only allowed to contain entries for functions and static variables.
 *
 * An extern block may not be declared within a local scope or a struct
 * definition.
 */
class Node::ExternBlock : public virtual Node::Namespace {
public:
    virtual ~ExternBlock() = default;

    ExternBlock(Private)
        : Node(Private()),
          Node::IScope(Private()),
          Node::IGlobalScope(Private()),
          Node::ILocatable(Private()),
          Node::Namespace(Private()) {}

    static std::shared_ptr<ExternBlock>
    create(std::shared_ptr<Node::IScope> parent, std::shared_ptr<Token> token);

    virtual std::string to_string() const override {
        return "EXTERN \"" + symbol + "\"";
    }

    virtual bool add_child(
        SymbolTree& symbol_tree,
        std::shared_ptr<Node::ILocatable> child,
        std::optional<std::string> custom_symbol = std::nullopt
    ) override;
};

/**
 * @brief A primitive type in the symbol tree.
 *
 * A primitive type node references a basic type object instead of a custom
 * type. This allows the type checker to look up basic types as if they were
 * any other named type.
 *
 * Unlike Node::StructDef, the type object is constructed *before* the node
 * rather than after. This is possible since the basic types do not need to
 * reference any nodes in the symbol tree.
 *
 * Ideally, the symbol tree should install primitive types in the root
 * scope.
 */
class Node::PrimitiveType : public virtual Node::ITypeNode {
public:
    virtual ~PrimitiveType() = default;

    PrimitiveType(Private)
        : Node(Private()), Node::ITypeNode(Private()) {}

    /**
     * @brief Creates a new primitive type node and adds it to the parent scope.
     *
     * Primitive types are usually added to the reserved scope, a root scope
     * separate from the main tree.
     *
     * @param parent The parent scope in which to add the primitive type.
     * @param short_name The short name of the primitive type.
     * @param type The type object that this primitive type node references.
     * @return A shared pointer to the newly created primitive type node.
     */
    static std::shared_ptr<PrimitiveType> create(
        std::shared_ptr<Node::IScope> parent,
        std::string_view short_name,
        std::shared_ptr<Type> type
    );

    virtual std::string to_string() const override {
        return "PTYPE \"" + symbol + "\" : " + type->to_string();
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
    // Whether this struct is declared with `class` or not. Classes may
    // follow different semantic rules than structs, such as memory
    // management.
    bool is_class = false;
    // A dictionary of properties (bindings) in this struct, indexed by their
    // names.
    Dictionary<std::string, Binding> properties;
    // A dictionary of methods in this struct, indexed by their names.
    // Methods are also stored as bindings, but are never `var` and always
    // have a type of `Function`.
    Dictionary<std::string, Binding> methods;

    virtual ~StructDef() = default;

    StructDef(Private)
        : Node(Private()),
          Node::IScope(Private()),
          Node::IGlobalScope(Private()),
          Node::ITypeNode(Private()),
          Node::ILocatable(Private()) {}

    /**
     * @brief Creates a new struct definition node and adds it to the parent
     * scope.
     *
     * The struct and its corresponding named type are also set up to reference
     * each other.
     *
     * @param parent The parent scope in which to add the struct definition.
     * @param token The token representing the struct identifier.
     * @param is_class Whether this struct is declared with `class` or not.
     * @return A shared pointer to the newly created struct definition node.
     */
    static std::shared_ptr<StructDef> create(
        std::shared_ptr<Node::IScope> parent,
        std::shared_ptr<Token> token,
        bool is_class = false
    );

    virtual bool add_child(
        SymbolTree& symbol_tree,
        std::shared_ptr<Node::ILocatable> child,
        std::optional<std::string> custom_symbol = std::nullopt
    ) override;

    virtual std::string to_string() const override {
        return "STRUCT \"" + symbol + "\"";
    }
};

/**
 * @brief A local scope node in the symbol tree.
 *
 * Local scopes are used to define variables and functions that are only
 * accessible within a specific block of code. They do not have names; their
 * unique identifiers are generated using numbers, which increment with each
 * new local scope created. They are not global scopes and cannot contain
 * other global scopes.
 *
 * As a side effect of having only numbers as identifiers, it is impossible
 * to reference a variable declared in a local scope from outside that scope
 * (since an identifier expression cannot start with a number).
 */
class Node::LocalScope : public virtual Node::IScope {
public:
    // A static counter to generate unique identifiers for local scopes.
    static int next_scope_id;
    // The block expression that this local scope represents.
    std::shared_ptr<Expr::Block> block;
    // The type of the expressions yielded within this local scope.
    // We track the type here until we set the type of the block expression.
    std::optional<std::shared_ptr<Type>> yield_type;

    virtual ~LocalScope() = default;

    LocalScope(Private)
        : Node(Private()), Node::IScope(Private()) {}

    /**
     * @brief Creates a new local scope and adds it to the current scope's list
     * of local scopes.
     *
     * Local scopes do not have real names and are kept alive by their parent's
     * list of local scopes.
     *
     * Their names and symbols are based on a static counter that increments
     * with each new local scope created.
     *
     * @param parent The parent scope in which to add the local scope.
     * @param block The block expression that this local scope represents.
     * @return A shared pointer to the newly created local scope node.
     */
    static std::shared_ptr<LocalScope> create(
        std::shared_ptr<Node::IScope> parent, std::shared_ptr<Expr::Block> block
    );

    virtual bool add_child(
        SymbolTree& symbol_tree,
        std::shared_ptr<Node::ILocatable> child,
        std::optional<std::string> custom_symbol = std::nullopt
    ) override;

    virtual std::string to_string() const override {
        return "LSCOPE \"" + symbol + "\"";
    }
};

/**
 * @brief A binding entry in the symbol tree.
 *
 * Binding entries are any variable declared with `let`.
 *
 * Binding objects carry a type object, and must therefore have their types
 * resolved before being constructed.
 */
class Node::BindingEntry : public virtual Node::ILocatable {
public:
    // Whether this binding entry is declared in a global scope or not.
    // Global binding entries are declared using LLVM global variables, while
    // local binding entries are declared using LLVM alloca instructions.
    bool is_global;
    // Whether this binding entry is a global variable that has been
    // initialized. If this is true, the variable should not be initialized
    // again.
    bool is_initialized;

    // The binding object that this entry represents.
    Binding binding;
    // The linkage of this binding entry.
    Linkage linkage;
    // If this binding is a local variable, the pointer to the LLVM
    // allocation.
    llvm::AllocaInst* llvm_ptr = nullptr;

    virtual ~BindingEntry() = default;

    BindingEntry(Private, const Binding& binding, Linkage linkage)
        : Node(Private()),
          Node::ILocatable(Private()),
          binding(binding),
          linkage(linkage) {}

    /**
     * @brief Creates a new binding entry node and adds it to the parent scope.
     *
     * A binding entry node represents a new variable or function in the symbol
     * tree.
     *
     * @param parent The parent scope in which to add the binding entry.
     * @param binding The binding object that this entry represents.
     * @param linkage The linkage of this binding entry.
     * @return A shared pointer to the newly created binding entry node.
     */
    static std::shared_ptr<BindingEntry> create(
        std::shared_ptr<Node::IScope> parent,
        const Binding& binding,
        Linkage linkage
    );

    /**
     * @brief Gets the LLVM allocation for this binding entry.
     *
     * If the binding is global, this function will attempt to get the global
     * variable. If the binding is local, it will return the LLVM pointer
     * stored in the node.
     *
     * If the binding is a function type, a special suffix is added to the
     * symbol name (the symbol is not modified) to differentiate it from the
     * function with the same name.
     *
     * @param builder The IRBuilder used to create the allocation if needed.
     */
    virtual llvm::Value*
    get_llvm_allocation(std::unique_ptr<llvm::IRBuilder<>>& builder) {
        llvm::Value* ptr;
        std::string suffix =
            PTR_INSTANCEOF(binding.type, Type::Function) ? "$var" : "";

        if (is_global) {
            auto ir_module = builder->GetInsertBlock()->getModule();
            // Attempt to get the global variable.
            ptr = ir_module->getGlobalVariable(symbol + suffix, true);
            // If it doesn't exist, declare it.
            auto llvm_type = binding.type->get_llvm_type(builder);
            if (ptr == nullptr) {
                ptr = new llvm::GlobalVariable(
                    *ir_module,
                    llvm_type,
                    false, // isConstant
                    get_llvm_linkage(),
                    is_initialized ? nullptr
                                   : llvm::Constant::getNullValue(
                                         llvm_type
                                     ), // Initializer
                    symbol + suffix
                );

                // If the variable was not initialized before, it should be
                // initialized now.
                is_initialized = true;

                /*
                Note: Using `nullptr` is very different from using
                `getNullValue`. `nullptr` means that the global variable is
                declared but not defined. `getNullValue` means that the global
                variable is defined and initialized to zero.

                This is important, because global variables with external
                linkage should be defined first, and then only declared in other
                modules. If we define a variable multiple times, we may get
                unusual errors involving missing symbols. If we declare a
                variable multiple times, but never define it, we will also get
                errors. The trick is to initialize the variable to zero on the
                first definition, and then declare it without an initializer on
                subsequent definitions.
                */
            }
        }
        else {
            ptr = llvm_ptr;
            if (ptr == nullptr) {
                panic(
                    "Node::BindingEntry::get_llvm_allocation: Local variable "
                    "has "
                    "no LLVM allocation."
                );
            }
        }
        return ptr;
    }

    /**
     * @brief Returns the appropriate LLVM linkage type for this binding entry
     * based on its binding information.
     *
     * @return The LLVM linkage type.
     */
    llvm::GlobalValue::LinkageTypes get_llvm_linkage() const {
        switch (linkage) {
        case Linkage::Internal:
            return llvm::GlobalValue::InternalLinkage;
        case Linkage::External:
            return llvm::GlobalValue::ExternalLinkage;
        default:
            panic(
                "Node::BindingEntry::get_llvm_linkage: Unsupported linkage "
                "type."
            );
        }
    }

    virtual std::string to_string() const override {
        return "ENTRY \"" + symbol + "\" : " + binding.type->to_string();
    }
};

/**
 * @brief An overload group in the symbol tree.
 *
 * Overload groups are used to group related function overloads together
 * under a single name. They are represented as binding entries with a type of
 * `Type::OverloadedFn`.
 *
 *
 * Since they are binding entries, they are also locatable nodes.
 * The location token should be set to the first overload's token.
 */
class Node::OverloadGroup : public virtual Node::BindingEntry {
public:
    // A list of overloads in this group.
    std::vector<std::shared_ptr<Node::BindingEntry>> overloads;

    virtual ~OverloadGroup() = default;

    OverloadGroup(
        Private,
        std::string_view overload_name,
        const Location* first_overload_location
    )
        : Node(Private()),
          Node::ILocatable(Private()),
          Node::BindingEntry(
              Private(),
              Binding(
                  Binding::Mutability::None,
                  overload_name,
                  first_overload_location,
                  std::dynamic_pointer_cast<Type>(
                      std::make_shared<Type::OverloadedFn>()
                  )
              ),
              Linkage::Internal
          ) {}

    /**
     * @brief Creates a new overload group and adds it to the parent scope.
     *
     * Additionally, an instance of `Type::OverloadedFn` is created and assigned
     * to the overload group's binding entry.
     *
     * @param parent The parent scope to which the overload group will be
     * added.
     * @param overload_name The name of the overload group.
     * @param first_overload_location The location of the first overload.
     * @return A shared pointer to the newly created overload group node.
     */
    static std::shared_ptr<OverloadGroup> create(
        std::shared_ptr<Node::IScope> parent,
        std::string_view overload_name,
        const Location* first_overload_location
    );

    virtual llvm::Value*
    get_llvm_allocation(std::unique_ptr<llvm::IRBuilder<>>& builder) override {
        return llvm::UndefValue::get(
            llvm::PointerType::get(builder->getContext(), 0)
        );
        // When the code generator encounters a name reference, it requires
        // the LLVM allocation of the binding entry. Overload groups do not
        // have a meaningful LLVM allocation, but we need to return
        // something.

        // This value will never actually be used. This is because when you
        // "use" a name reference to an overload group, the type checker
        // replaces the binding entry with the correct function overload
        // before code generation.
    }

    virtual std::string to_string() const override {
        return "FUNC \"" + symbol + "\"";
    }

    virtual std::string to_tree_string(size_t indent = 0) const override {
        std::string indent_str(indent, ' ');
        std::string result = indent_str + to_string() + "\n";
        for (const auto& overload : overloads) {
            result += overload->to_tree_string(indent + 2);
        }
        return result;
    }
};

} // namespace nico

#endif // NICO_SYMBOL_NODE_H
