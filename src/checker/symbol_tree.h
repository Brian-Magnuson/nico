#ifndef NICO_SYMBOL_TREE_H
#define NICO_SYMBOL_TREE_H

#include <cstdlib>
#include <expected>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include "../logger/error_code.h"
#include "../parser/annotation.h"
#include "../parser/ident.h"
#include "dictionary.h"
#include "type.h"

/**
 * @brief The base class for all nodes in the symbol tree.
 *
 * All nodes in the symbol tree have a unique name to identify them.
 * Most subclasses of Node inherit from Node::IScope, meaning they have other nodes as children.
 */
class Node : public std::enable_shared_from_this<Node> {
public:
    class IScope;
    class IGlobalScope;

    class RootScope;
    class Namespace;
    class StructDef;
    class LocalScope;
    class FieldEntry;

    // This node's parent scope, if it exists.
    std::weak_ptr<Node::IScope> parent;
    // This node's unique name, assigned upon construction.
    const std::string unique_name;

    virtual ~Node() = default;

    Node(std::weak_ptr<Node::IScope> parent_scope, const std::string& name)
        : parent(std::move(parent_scope)), unique_name(name) {}
};

/**
 * @brief A scope interface for nodes in the symbol tree.
 *
 * A scope may contain other nodes as children, allowing for a hierarchical structure.
 */
class Node::IScope : public Node {
public:
    // A dictionary of child nodes, indexed by their name parts.
    Dictionary<std::string, std::shared_ptr<Node>> children;

    IScope(std::weak_ptr<Node::IScope> parent_scope, const std::string& name)
        : Node(std::move(parent_scope), name) {
        parent_scope.lock()->children[name] = shared_from_this();
    }
};

/**
 * @brief A global scope interface for nodes in the symbol tree.
 *
 * Global scopes include the root scope, namespaces, and struct definitions.
 *
 * The global scope interface adds no additional members to IScope.
 * Rather, it is used to distinguish global scopes from local scopes.
 */
class Node::IGlobalScope : public Node::IScope {
public:
    IGlobalScope(std::weak_ptr<Node::IScope> parent_scope, const std::string& name)
        : Node::IScope(std::move(parent_scope), name) {}
};

/**
 * @brief The root scope of the symbol tree.
 *
 * The root scope is the top-level scope that contains all other scopes.
 * Its unique identifier is always "::" and the pointer to its parent scope is empty.
 */
class Node::RootScope : public Node::IGlobalScope {
public:
    RootScope()
        : Node::IGlobalScope(std::weak_ptr<Node::IScope>(), "::") {}
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
class Node::Namespace : public Node::IGlobalScope {
public:
    Namespace(std::weak_ptr<Node::IScope> parent_scope, const std::string& name)
        : Node::IGlobalScope(std::move(parent_scope), parent_scope.lock()->unique_name + "::" + name) {}
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
class Node::StructDef : public Node::IGlobalScope {
public:
    // Whether this struct is declared with `class` or not. Classes may follow different semantic rules than structs, such as memory management.
    const bool is_class = false;
    // A dictionary of properties (fields) in this struct, indexed by their names.
    Dictionary<std::string, Field> properties;
    // A dictionary of methods in this struct, indexed by their names. Methods are also stored as fields, but are never `var` and always have a type of `Function`.
    Dictionary<std::string, Field> methods;

    StructDef(std::weak_ptr<Node::IScope> parent_scope, const std::string& name, bool is_class = false)
        : Node::IGlobalScope(std::move(parent_scope), parent_scope.lock()->unique_name + "::" + name), is_class(is_class) {}
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
class Node::LocalScope : public Node::IScope {
public:
    static int next_scope_id;

    LocalScope(std::weak_ptr<Node::IScope> parent_scope)
        : Node::IScope(std::move(parent_scope), std::to_string(next_scope_id++)) {}
};

int Node::LocalScope::next_scope_id = 0;

/**
 * @brief A field entry in the symbol tree.
 *
 * Field entries are any variable declared with `let` or any function declared with `func`.
 *
 * Field objects carry a type object, and must therefore have their types resolved before being constructed.
 */
class Node::FieldEntry : public Node {
public:
    // The field object that this entry represents.
    Field field;

    FieldEntry(std::weak_ptr<Node::IScope> parent_scope, const Field& field)
        : Node(std::move(parent_scope), parent_scope.lock()->unique_name + "::" + field.name), field(field) {}
};

/**
 * @brief A symbol tree that represents the structure of the program's symbols.
 */
class SymbolTree {
    // The root scope of the symbol tree, which is the top-level scope that contains all other scopes.
    std::shared_ptr<Node::RootScope> root_scope;
    // The current scope in the symbol tree, which is the scope that is currently being modified or accessed.
    std::shared_ptr<Node::IScope> current_scope;

public:
    SymbolTree()
        : root_scope(std::make_shared<Node::RootScope>()), current_scope(root_scope) {}

    /**
     * @brief Adds a namespace to the symbol tree, then enters the namespace scope.
     *
     * If the current scope does not allow namespaces, this function does not add the namespace and returns an error.
     *
     * @param name The name of the namespace.
     * @return std::expected<std::shared_ptr<Node::Namespace>, Err> The namespace if added successfully.
     */
    std::expected<std::shared_ptr<Node::Namespace>, Err> add_namespace(const std::string& name);

    /**
     * @brief Adds a struct definition to the symbol tree, then enters the struct definition scope.
     *
     * If the struct definition already exists, or the current scope does not allow structs, this function does not add the struct and returns an error.
     *
     * @param name The name of the struct.
     * @return std::expected<std::shared_ptr<Node::StructDef>, Err> The struct definition if added successfully.
     */
    std::expected<std::shared_ptr<Node::StructDef>, Err> add_struct_def(const std::string& name, bool is_class = false);

    /**
     * @brief Adds a new local scope to the symbol tree, then enters the local scope.
     *
     * Currently, this function has no restrictions on where local scopes can be added.
     *
     * @return std::expected<std::shared_ptr<Node::LocalScope>, Err> The local scope if added successfully.
     */
    std::expected<std::shared_ptr<Node::LocalScope>, Err> add_local_scope();

    /**
     * @brief Exits the current scope and returns to the parent scope.
     *
     * If the current scope is the root scope, this function does nothing and returns std::nullopt.
     *
     * If the current scope is a local scope, its set of declared variables is cleared upon exit.
     *
     * @return std::optional<std::shared_ptr<Node::IScope>> The parent scope if it exists, or std::nullopt if there is no parent scope (i.e., if the current scope is the root scope).
     */
    std::optional<std::shared_ptr<Node::IScope>> exit_scope();

    /**
     * @brief Searches the symbol tree for a node with the matching identifier.
     *
     * The search algorithm comes in two parts: upward search and downward search.
     *
     * Upward search: Search from the current scope upward until the first part of the Ident matches.
     * Downward search: Search from the matched scope downward for the remaining parts of the Ident.
     * If downward search fails, resume upward search until the next match is found or the root scope is reached.
     *
     * Note: If the desired node is a FieldEntry, this function does not reveal whether the field entry is currently declared. Consider adding a check in the type checker.
     *
     * @param ident The identifier to search for.
     * @return std::optional<std::shared_ptr<Node>> The node if found, or
     * std::nullopt if not found.
     */
    std::optional<std::shared_ptr<Node>> search_ident(const Ident& ident) const;

    /**
     * @brief Adds a field entry to the symbol tree in the current scope.
     *
     * If the field name already exists in the current scope, this function does not add the field and returns std::nullopt.
     *
     * Because a field entry carries a type object, the field's type must be resolved before being added to the symbol tree.
     *
     * @param field The field to add.
     * @return std::optional<std::shared_ptr<Node::FieldEntry>> The field entry if added successfully, or std::nullopt if the field name already exists in the current scope.
     */
    std::optional<std::shared_ptr<Node::FieldEntry>> add_field_entry(const Field& field);
};

#endif // NICO_SYMBOL_TREE_H
