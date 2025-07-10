#include "symbol_tree.h"
#include "../common/utils.h"

void SymbolTree::install_primitive_types() {
    std::shared_ptr<Node> new_node;

    new_node = std::make_shared<Node::PrimitiveType>(root_scope, "i32", std::make_shared<Type::Int>(true, 32));
    new_node->initialize_node();

    new_node = std::make_shared<Node::PrimitiveType>(root_scope, "f64", std::make_shared<Type::Float>(64));
    new_node->initialize_node();

    new_node = std::make_shared<Node::PrimitiveType>(root_scope, "bool", std::make_shared<Type::Bool>());
    new_node->initialize_node();
}

std::pair<std::shared_ptr<Node::Namespace>, Err> SymbolTree::add_namespace(const std::string& name) {
    // Namespaces cannot be added in a local scope
    if (PTR_INSTANCEOF(current_scope, Node::LocalScope)) {
        return std::make_pair(nullptr, Err::NamespaceInLocalScope);
    }
    // Namespaces cannot be added in a struct definition
    if (PTR_INSTANCEOF(current_scope, Node::StructDef)) {
        return std::make_pair(nullptr, Err::NamespaceInStructDef);
    }

    auto new_namespace = std::make_shared<Node::Namespace>(current_scope, name);
    new_namespace->initialize_node(); // Add the namespace to its parent scope's children
    current_scope = new_namespace;
    return std::make_pair(new_namespace, Err::Null);
}

std::pair<std::shared_ptr<Node::StructDef>, Err> SymbolTree::add_struct_def(const std::string& name, bool is_class) {
    // Structs cannot be added in a local scope
    if (PTR_INSTANCEOF(current_scope, Node::LocalScope)) {
        return std::make_pair(nullptr, Err::StructInLocalScope);
    }
    // Check if the struct already exists in the current scope
    if (current_scope->children.at(name)) {
        return std::make_pair(nullptr, Err::StructAlreadyDeclared);
    }

    auto new_struct = std::make_shared<Node::StructDef>(current_scope, name, is_class);
    new_struct->initialize_node(); // Add the struct to its parent scope's children
    current_scope = new_struct;
    return std::make_pair(new_struct, Err::Null);
}

std::pair<std::shared_ptr<Node::LocalScope>, Err> SymbolTree::add_local_scope() {
    auto new_local_scope = std::make_shared<Node::LocalScope>(current_scope);
    new_local_scope->initialize_node(); // Add the local scope to its parent scope's children
    current_scope = new_local_scope;
    return std::make_pair(new_local_scope, Err::Null);
}

std::optional<std::shared_ptr<Node::IScope>> SymbolTree::exit_scope() {
    if (current_scope->parent.expired()) {
        return std::nullopt; // Cannot exit root scope
    }

    current_scope = current_scope->parent.lock();
    return current_scope;
}

std::optional<std::shared_ptr<Node>> SymbolTree::search_ident(const Ident& ident) const {
    auto& parts = ident.parts;

    // Upward search: Search from the current scope upward until the first part of the Ident matches.
    auto scope = current_scope;

    while (scope) {
        auto it = scope->children.at(std::string(parts[0].token->lexeme));
        if (it) {
            // Found a match for the first part, now do downward search for the remaining parts.
            auto node = it.value();
            bool found = true;

            // Downward search: Search from the matched scope downward for the remaining parts of the Ident.
            for (size_t i = 1; i < parts.size(); ++i) {
                if (PTR_INSTANCEOF(node, Node::IScope)) {
                    auto scope_node = std::dynamic_pointer_cast<Node::IScope>(node);
                    auto child_it = scope_node->children.at(std::string(parts[i].token->lexeme));
                    if (child_it) {
                        node = child_it.value();
                    } else {
                        found = false;
                        break;
                    }
                } else {
                    found = false;
                    break;
                }
            }

            if (found) {
                return node; // Successfully found the full Ident
            }
        }
        // Move up to the parent scope for the next iteration
        scope = scope->parent.lock();
        // If the current scope is the root scope, the `scope->parent.lock()` will return an empty shared_ptr, causing the loop to terminate.
    }

    return std::nullopt; // Not found
}

std::optional<std::shared_ptr<Node::FieldEntry>> SymbolTree::add_field_entry(const Field& field) {
    if (current_scope->children.contains(std::string(field.token->lexeme))) {
        return std::nullopt; // Field already exists
    }

    auto new_field_entry = std::make_shared<Node::FieldEntry>(current_scope, field);
    new_field_entry->initialize_node(); // Add the field entry to its parent scope's children
    return new_field_entry;
}
