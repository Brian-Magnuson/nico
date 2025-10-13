#include "nico/frontend/utils/symbol_tree.h"

#include "nico/shared/utils.h"

namespace nico {

void SymbolTree::install_primitive_types() {
    modified = true;
    std::shared_ptr<Node> new_node;

    new_node = std::make_shared<Node::PrimitiveType>(
        reserved_scope,
        "i32",
        std::make_shared<Type::Int>(true, 32)
    );
    new_node->initialize_node();

    new_node = std::make_shared<Node::PrimitiveType>(
        reserved_scope,
        "f64",
        std::make_shared<Type::Float>(64)
    );
    new_node->initialize_node();

    new_node = std::make_shared<Node::PrimitiveType>(
        reserved_scope,
        "bool",
        std::make_shared<Type::Bool>()
    );
    new_node->initialize_node();

    modified = true;
}

std::optional<std::shared_ptr<Node>> SymbolTree::search_name_from_scope(
    const Name& name, std::shared_ptr<Node::IScope> scope
) const {
    auto& parts = name.parts;

    // Upward search: Search from the current scope upward until the first part
    // of the Name matches.
    while (scope) {
        auto it = scope->children.at(std::string(parts[0].token->lexeme));
        if (it) {
            // Found a match for the first part, now do downward search for the
            // remaining parts.
            auto node = it.value();
            bool found = true;

            // Downward search: Search from the matched scope downward for the
            // remaining parts of the Name.
            for (size_t i = 1; i < parts.size(); ++i) {
                if (PTR_INSTANCEOF(node, Node::IScope)) {
                    auto scope_node =
                        std::dynamic_pointer_cast<Node::IScope>(node);
                    auto child_it = scope_node->children.at(
                        std::string(parts[i].token->lexeme)
                    );
                    if (child_it) {
                        node = child_it.value();
                    }
                    else {
                        found = false;
                        break;
                    }
                }
                else {
                    found = false;
                    break;
                }
            }

            if (found) {
                return node; // Successfully found the full Name
            }
        }
        // Move up to the parent scope for the next iteration
        scope = scope->parent.lock();
        // If the current scope is the root scope, the `scope->parent.lock()`
        // will return an empty shared_ptr, causing the loop to terminate.
    }

    return std::nullopt; // Not found
}

std::pair<std::shared_ptr<Node>, Err>
SymbolTree::add_namespace(std::shared_ptr<Token> token) {
    // Namespaces cannot be added in a local scope
    if (PTR_INSTANCEOF(current_scope, Node::LocalScope)) {
        return std::make_pair(nullptr, Err::NamespaceInLocalScope);
    }
    // Namespaces cannot be added in a struct definition
    if (PTR_INSTANCEOF(current_scope, Node::StructDef)) {
        return std::make_pair(nullptr, Err::NamespaceInStructDef);
    }
    // Check if the name is reserved.
    if (auto node = reserved_scope->children.at(std::string(token->lexeme))) {
        return std::make_pair(node.value(), Err::NameIsReserved);
    }

    // Check if the name already exists.
    if (auto node = current_scope->children.at(std::string(token->lexeme))) {
        if (auto ns_node =
                std::dynamic_pointer_cast<Node::Namespace>(node.value())) {
            // If existing name is a namespace...
            current_scope = ns_node;
            modified = true;
            return std::make_pair(ns_node, Err::Null);
        }
        else {
            // If existing name is not a namespace...
            return std::make_pair(node.value(), Err::NameAlreadyExists);
        }
    }
    else {
        // If name does not exist...
        // Add the namespace to its parent scope's children.
        auto new_namespace =
            std::make_shared<Node::Namespace>(current_scope, token);
        new_namespace->initialize_node();
        current_scope = new_namespace;
        modified = true;
        return std::make_pair(new_namespace, Err::Null);
    }
}

std::pair<std::shared_ptr<Node>, Err>
SymbolTree::add_struct_def(std::shared_ptr<Token> token, bool is_class) {
    // Structs cannot be added in a local scope
    if (PTR_INSTANCEOF(current_scope, Node::LocalScope)) {
        return std::make_pair(nullptr, Err::StructInLocalScope);
    }
    // Check if the name exists in the reserved scope
    if (auto node = reserved_scope->children.at(std::string(token->lexeme))) {
        return std::make_pair(node.value(), Err::NameIsReserved);
    }

    // Check if the struct already exists in the current scope
    if (auto node = current_scope->children.at(std::string(token->lexeme))) {
        return std::make_pair(node.value(), Err::NameAlreadyExists);
    }

    auto new_struct =
        std::make_shared<Node::StructDef>(current_scope, token, is_class);
    new_struct
        ->initialize_node(); // Add the struct to its parent scope's children
    current_scope = new_struct;
    modified = true;
    return std::make_pair(new_struct, Err::Null);
}

// std::pair<std::shared_ptr<Node::FunctionScope>, Err>
// SymbolTree::add_function_scope(std::shared_ptr<Token> token) {
//     // Function scopes cannot be added in a local scope
//     if (PTR_INSTANCEOF(current_scope, Node::LocalScope)) {
//         return std::make_pair(nullptr, Err::FunctionScopeInLocalScope);
//     }

//     // Add the function scope to its parent scope's children.
//     auto new_function_scope =
//         std::make_shared<Node::FunctionScope>(current_scope, token);
//     new_function_scope->initialize_node();
//     current_scope = new_function_scope;
//     modified = true;
//     return std::make_pair(new_function_scope, Err::Null);
// }

std::pair<std::shared_ptr<Node::LocalScope>, Err>
SymbolTree::add_local_scope(Expr::Block::Kind kind) {
    // Add the local scope to its parent scope's children.
    auto new_local_scope =
        std::make_shared<Node::LocalScope>(current_scope, kind);
    new_local_scope->initialize_node();
    current_scope = new_local_scope;
    modified = true;
    return std::make_pair(new_local_scope, Err::Null);
}

std::optional<std::shared_ptr<Node::IScope>> SymbolTree::exit_scope() {
    if (current_scope->parent.expired()) {
        return std::nullopt; // Cannot exit root scope
    }

    current_scope = current_scope->parent.lock();
    modified = true;
    return current_scope;
}

std::optional<std::shared_ptr<Node>>
SymbolTree::search_name(const Name& name) const {
    // First, search the reserved scope.
    if (auto node = search_name_from_scope(name, reserved_scope)) {
        return node;
    }
    // Next, search the current scope.
    return search_name_from_scope(name, current_scope);
}

std::pair<std::shared_ptr<Node>, Err>
SymbolTree::add_field_entry(const Field& field) {
    if (auto node =
            reserved_scope->children.at(std::string(field.token->lexeme))) {
        return std::make_pair(node.value(), Err::NameIsReserved);
    }

    if (auto node =
            current_scope->children.at(std::string(field.token->lexeme))) {
        return std::make_pair(node.value(), Err::NameAlreadyExists);
    }

    // Add the field entry to its parent scope's children.
    auto new_field_entry =
        std::make_shared<Node::FieldEntry>(current_scope, field);
    new_field_entry->initialize_node();
    modified = true;

    return std::make_pair(new_field_entry, Err::Null);
}

} // namespace nico
