#ifndef NICO_SYMBOL_TREE_H
#define NICO_SYMBOL_TREE_H

#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>

#include "../logger/error_code.h"
#include "../parser/ident.h"
#include "dictionary.h"
#include "type.h"

/**
 * @brief The base class for all nodes in the symbol tree.
 */
class Node {
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

class Node::IScope : public Node {
public:
    Dictionary<std::string, std::shared_ptr<Node>> children;

    IScope(std::weak_ptr<Node::IScope> parent_scope, const std::string& name)
        : Node(std::move(parent_scope), name) {}
};

class Node::IGlobalScope : public Node::IScope {
public:
    IGlobalScope(std::weak_ptr<Node::IScope> parent_scope, const std::string& name)
        : Node::IScope(std::move(parent_scope), name) {}
};

class Node::RootScope : public Node::IGlobalScope {
public:
    RootScope()
        : Node::IGlobalScope(std::weak_ptr<Node::IScope>(), "::") {}
};

class Node::Namespace : public Node::IGlobalScope {
public:
    Namespace(std::weak_ptr<Node::IScope> parent_scope, const std::string& name)
        : Node::IGlobalScope(std::move(parent_scope), parent_scope.lock()->unique_name + "::" + name) {}
};

class Node::StructDef : public Node::IGlobalScope {
public:
    const bool is_class = false;
    Dictionary<std::string, Field> properties;
    Dictionary<std::string, std::shared_ptr<Type::Function>> methods;

    StructDef(std::weak_ptr<Node::IScope> parent_scope, const std::string& name, bool is_class = false)
        : Node::IGlobalScope(std::move(parent_scope), parent_scope.lock()->unique_name + "::" + name), is_class(is_class) {}
};

class Node::LocalScope : public Node::IScope {
public:
    static int next_scope_id;

    std::unordered_set<std::string> declared_locals;

    LocalScope(std::weak_ptr<Node::IScope> parent_scope)
        : Node::IScope(std::move(parent_scope), std::to_string(next_scope_id++)) {}
};

int Node::LocalScope::next_scope_id = 0;

class Node::FieldEntry : public Node {
public:
    Field field;

    FieldEntry(std::weak_ptr<Node::IScope> parent_scope, const Field& field)
        : Node(std::move(parent_scope), parent_scope.lock()->unique_name + "::" + field.name), field(field) {}
};

class SymbolTree {

    std::shared_ptr<Node::RootScope> root_scope;

    std::shared_ptr<Node::IScope> current_scope;

public:
    /**
     * @brief Adds a namespace to the symbol tree, then enters the namespace scope.
     * @param name The name of the namespace.
     * @return std::expected<std::shared_ptr<Node::Namespace>, Err> The namespace if added successfully.
     */
    std::expected<std::shared_ptr<Node::Namespace>, Err> add_namespace(const std::string& name);

    /**
     * @brief Adds a struct definition to the symbol tree, then enters the struct definition scope.
     * @param name The name of the struct.
     * @return std::expected<std::shared_ptr<Node::StructDef>, Err> The struct definition if added successfully.
     */
    std::expected<std::shared_ptr<Node::StructDef>, Err> add_struct_def(const std::string& name, bool is_class = false);

    /**
     * @brief Adds a new local scope to the symbol tree, then enters the local scope.
     * @return std::expected<std::shared_ptr<Node::LocalScope>, Err> The local scope if added successfully.
     */
    std::expected<std::shared_ptr<Node::LocalScope>, Err> add_local_scope();

    /**
     * @brief Searches the symbol tree for a node with the matching identifier.
     *
     * @param ident The identifier to search for.
     * @return std::optional<std::shared_ptr<Node>> The node if found, or
     * std::nullopt if not found.
     */
    std::optional<std::shared_ptr<Node>> search_ident(Ident ident) const;
};

#endif // NICO_SYMBOL_TREE_H
