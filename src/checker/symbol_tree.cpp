#include "symbol_tree.h"

#define PTR_INSTANCEOF(ptr, type) \
    (std::dynamic_pointer_cast<type>(ptr) != nullptr)

std::expected<std::shared_ptr<Node::Namespace>, Err> SymbolTree::add_namespace(const std::string& name) {
    // Namespaces cannot be added in a local scope
    if (PTR_INSTANCEOF(current_scope, Node::LocalScope)) {
        return std::unexpected(Err::NamespaceInLocalScope);
    }
    // Namespaces cannot be added in a struct definition
    if (PTR_INSTANCEOF(current_scope, Node::StructDef)) {
        return std::unexpected(Err::NamespaceInStructDef);
    }

    auto new_namespace = std::make_shared<Node::Namespace>(current_scope, name);
    current_scope = new_namespace;
    return new_namespace;
}

std::expected<std::shared_ptr<Node::StructDef>, Err> SymbolTree::add_struct_def(const std::string& name, bool is_class) {
    // Structs cannot be added in a local scope
    if (PTR_INSTANCEOF(current_scope, Node::LocalScope)) {
        return std::unexpected(Err::StructInLocalScope);
    }
    // Check if the struct already exists in the current scope
    if (current_scope->children.at(name)) {
        return std::unexpected(Err::StructAlreadyDeclared);
    }

    auto new_struct = std::make_shared<Node::StructDef>(current_scope, name, is_class);
    current_scope = new_struct;
    return new_struct;
}

std::expected<std::shared_ptr<Node::LocalScope>, Err> SymbolTree::add_local_scope() {
    auto new_local_scope = std::make_shared<Node::LocalScope>(current_scope);
    current_scope = new_local_scope;
    return new_local_scope;
}

std::optional<std::shared_ptr<Node::IScope>> SymbolTree::exit_scope() {
    if (current_scope->parent.expired()) {
        return std::nullopt; // Cannot exit root scope
    }
    current_scope = current_scope->parent.lock();
    return current_scope;
}
