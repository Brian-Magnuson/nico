#include "nico/frontend/utils/symbol_tree.h"

#include "nico/shared/logger.h"
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

std::pair<std::shared_ptr<Node::LocalScope>, Err>
SymbolTree::add_local_scope(std::shared_ptr<Expr::Block> block) {
    // Add the local scope to its parent scope's children.
    auto new_local_scope =
        std::make_shared<Node::LocalScope>(current_scope, block);
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
    auto node = search_name_from_scope(name, reserved_scope);
    // If not found, search from the current scope.
    if (!node.has_value()) {
        node = search_name_from_scope(name, current_scope);
    }
    // If the found node is an OverloadGroup with exactly one overload, return
    // the single overload instead.
    if (node.has_value()) {
        if (auto overload_group =
                std::dynamic_pointer_cast<Node::OverloadGroup>(node.value())) {
            if (overload_group->overloads.size() == 1) {
                return overload_group->overloads.at(0);
            }
        }
    }

    return node;
}

std::optional<std::shared_ptr<Node::LocalScope>>
SymbolTree::get_local_scope_of_kind(Expr::Block::Kind kind) const {
    auto current = current_scope;
    while (current) {
        if (auto local_scope =
                std::dynamic_pointer_cast<Node::LocalScope>(current)) {
            // If this is a local scope, check if it matches the kind.
            if (local_scope->block && local_scope->block->kind == kind) {
                return local_scope;
            }
        }
        else {
            // If this scope is not a local scope, stop searching.
            break;
        }
        current = current->parent.lock();
    }
    return std::nullopt;
}

std::pair<std::shared_ptr<Node>, Err>
SymbolTree::add_field_entry(const Field& field) {
    // Make sure the name is not reserved.
    if (auto node =
            reserved_scope->children.at(std::string(field.token->lexeme))) {
        Logger::inst().log_error(
            Err::NameIsReserved,
            field.token->location,
            "Name `" + std::string(field.token->lexeme) +
                "` is reserved and cannot be used."
        );
        return std::make_pair(node.value(), Err::NameIsReserved);
    }

    if (auto node =
            current_scope->children.at(std::string(field.token->lexeme))) {
        Logger::inst().log_error(
            Err::NameAlreadyExists,
            field.token->location,
            "Name `" + std::string(field.token->lexeme) +
                "` already exists in the current scope."
        );
        if (auto locatable =
                std::dynamic_pointer_cast<Node::ILocatable>(node.value())) {
            Logger::inst().log_note(
                locatable->location_token->location,
                "Previous declaration here."
            );
        }
        return std::make_pair(node.value(), Err::NameAlreadyExists);
    }

    // Add the field entry to its parent scope's children.
    auto new_field_entry =
        std::make_shared<Node::FieldEntry>(current_scope, field);
    new_field_entry->initialize_node();
    modified = true;

    return std::make_pair(new_field_entry, Err::Null);
}

std::pair<std::shared_ptr<Node>, Err>
SymbolTree::add_overloadable_func(const Field& field) {
    // Make sure the name is not reserved.
    if (auto node =
            reserved_scope->children.at(std::string(field.token->lexeme))) {
        Logger::inst().log_error(
            Err::NameIsReserved,
            field.token->location,
            "Name `" + std::string(field.token->lexeme) +
                "` is reserved and cannot be used."
        );
        return std::make_pair(node.value(), Err::NameIsReserved);
    }

    // Check if the name already exists.
    std::shared_ptr<Node::OverloadGroup> overload_group;
    if (auto node =
            current_scope->children.at(std::string(field.token->lexeme))) {
        if (auto existing_overload_group =
                std::dynamic_pointer_cast<Node::OverloadGroup>(node.value())) {
            // If existing name is an overload group, add to it.
            overload_group = existing_overload_group;
        }
        else {
            // If existing name is not an overload group...
            Logger::inst().log_error(
                Err::NameAlreadyExists,
                field.token->location,
                "Name `" + std::string(field.token->lexeme) +
                    "` already exists in the current scope and is not an "
                    "overloadable function."
            );
            if (auto locatable =
                    std::dynamic_pointer_cast<Node::ILocatable>(node.value())) {
                Logger::inst().log_note(
                    locatable->location_token->location,
                    "Previous declaration here."
                );
            }
            return std::make_pair(node.value(), Err::NameAlreadyExists);
        }
    }
    else {
        // If name does not exist, create a new overload group.
        overload_group =
            std::make_shared<Node::OverloadGroup>(current_scope, field.token);
        overload_group->initialize_node();
        modified = true;
    }

    // Add the new overload to the overload group.
    auto new_overload =
        std::make_shared<Node::FieldEntry>(current_scope, field);
    new_overload->initialize_node();

    // TODO: TEMPORARY SOLUTION: We only allow one overload for now.
    if (overload_group->overloads.size() == 1) {
        Logger::inst().log_error(
            Err::FunctionOverloadConflict,
            field.token->location,
            "Function overloading is not yet supported; only one overload is "
            "allowed."
        );
        return std::make_pair(
            overload_group->overloads.at(0),
            Err::FunctionOverloadConflict
        );
    }

    overload_group->overloads.push_back(new_overload);
    modified = true;

    return std::make_pair(new_overload, Err::Null);
}

} // namespace nico
