#include "nico/frontend/utils/symbol_node.h"

#include "nico/frontend/utils/type_node.h"
#include "nico/shared/logger.h"
#include "nico/shared/sets.h"

namespace nico {

int Node::LocalScope::next_scope_id = 0;

std::string Node::IScope::to_tree_string(size_t indent) const {
    std::string indent_str(indent, ' ');
    std::string result = indent_str + to_string() + "\n";
    for (const auto& [name, child] : children) {
        result += child->to_tree_string(indent + 2);
    }
    for (const auto& local_scope : local_scopes) {
        result += local_scope->to_tree_string(indent + 2);
    }
    return result;
}

bool Node::IScope::add_child(
    SymbolTree& symbol_tree,
    std::shared_ptr<Node::ILocatable> child,
    std::optional<std::string> custom_symbol
) {
    // Step 1: Check if this node allows this child.
    // This is done by the implementing class.

    // Step 2: Check if the name is reserved.
    if (symbol_tree.is_name_reserved(child->short_name)) {
        Logger::inst().log_error(
            Err::NameIsReserved,
            child->location,
            "Name `" + child->short_name + "` is reserved and cannot be used."
        );
        return false;
    }

    // Step 3: Check if the name already exists in this scope.
    if (auto existing = children.at(child->short_name)) {
        Logger::inst().log_error(
            Err::NameAlreadyExists,
            child->location,
            "Name `" + child->short_name + "` already exists in this scope."
        );
        if (auto existing_locatable =
                std::dynamic_pointer_cast<Node::ILocatable>(existing.value())) {
            Logger::inst().log_note(
                existing_locatable->location,
                "Previous declaration of name `" + child->short_name +
                    "` found here."
            );
        }
        return false;
    }

    // Step 4: Register the symbol for this child.
    if (!symbol_tree.register_symbol(child, custom_symbol)) {
        // If registration fails, an error has already been logged.
        return false;
    }

    children[child->short_name] = child;
    return true;
}

std::shared_ptr<Node::Namespace> Node::Namespace::create(
    std::shared_ptr<Node::IScope> parent, std::shared_ptr<Token> token
) {
    auto node = std::make_shared<Namespace>(Private());
    node->parent = parent;
    node->short_name = std::string(token->lexeme);
    node->location = &token->location;
    return node;
}

std::shared_ptr<Node::ExternBlock> Node::ExternBlock::create(
    std::shared_ptr<Node::IScope> parent, std::shared_ptr<Token> token
) {
    auto node = std::make_shared<ExternBlock>(Private());
    node->parent = parent;
    node->short_name = std::string(token->lexeme);
    node->location = &token->location;
    return node;
}

bool Node::ExternBlock::add_child(
    SymbolTree& symbol_tree,
    std::shared_ptr<Node::ILocatable> child,
    std::optional<std::string> custom_symbol
) {
    // Extern blocks can only contain binding entries that represent functions.
    if (auto extern_child =
            std::dynamic_pointer_cast<Node::ExternBlock>(child)) {
        Logger::inst().log_error(
            Err::ExternBlockInExternBlock,
            child->location,
            "Extern block cannot contain another extern block."
        );
        return false;
    }
    else if (auto ns = std::dynamic_pointer_cast<Node::Namespace>(child)) {
        Logger::inst().log_error(
            Err::NamespaceInExternBlock,
            child->location,
            "Namespace is not allowed in extern block."
        );
        return false;
    }
    else if (auto struct_def =
                 std::dynamic_pointer_cast<Node::StructDef>(child)) {
        Logger::inst().log_error(
            Err::StructDefInExternBlock,
            child->location,
            "Struct definition is not allowed in extern block."
        );
        return false;
    }
    else if (PTR_INSTANCEOF(child, Node::LocalScope)) {
        panic(
            "Node::ExternBlock::add_child: Attempted to add local scope as a "
            "direct child to an extern block."
        );
        return false;
    }

    return Node::IScope::add_child(symbol_tree, child, custom_symbol);
}

std::shared_ptr<Node::PrimitiveType> Node::PrimitiveType::create(
    std::shared_ptr<Node::IScope> parent,
    std::string_view short_name,
    std::shared_ptr<Type> type
) {
    auto node = std::make_shared<PrimitiveType>(Private());
    node->parent = parent;
    node->short_name = std::string(short_name);

    if (type == nullptr) {
        panic("Node::PrimitiveType: Type cannot be null.");
    }
    node->type = type;
    return node;
}

std::shared_ptr<Node::StructDef> Node::StructDef::create(
    std::shared_ptr<Node::IScope> parent,
    std::shared_ptr<Token> token,
    bool is_class
) {
    auto node = std::make_shared<StructDef>(Private());
    node->parent = parent;
    node->short_name = std::string(token->lexeme);
    node->location = &token->location;
    node->is_class = is_class;

    node->type = std::make_shared<Type::Named>(node);
    return node;
}

bool Node::StructDef::add_child(
    SymbolTree& symbol_tree,
    std::shared_ptr<Node::ILocatable> child,
    std::optional<std::string> custom_symbol
) {
    if (auto ns = std::dynamic_pointer_cast<Node::Namespace>(child)) {
        Logger::inst().log_error(
            Err::NamespaceInStructDef,
            child->location,
            "Namespace is not allowed in struct definition."
        );
        return false;
    }
    else if (auto struct_def =
                 std::dynamic_pointer_cast<Node::StructDef>(child)) {
        Logger::inst().log_error(
            Err::StructDefInStructDef,
            child->location,
            "Struct definition is not allowed in struct definition."
        );
        return false;
    }
    else if (PTR_INSTANCEOF(child, Node::LocalScope)) {
        panic(
            "Node::StructDef::add_child: Attempted to add local scope as a "
            "direct child to a struct definition."
        );
        return false;
    }
    else if (auto extern_block =
                 std::dynamic_pointer_cast<Node::ExternBlock>(child)) {
        Logger::inst().log_error(
            Err::ExternBlockInStructDef,
            child->location,
            "Extern block is not allowed in struct definition."
        );
        return false;
    }

    return Node::IScope::add_child(symbol_tree, child, custom_symbol);
}

std::shared_ptr<Node::LocalScope> Node::LocalScope::create(
    std::shared_ptr<Node::IScope> parent, std::shared_ptr<Expr::Block> block
) {
    auto node = std::make_shared<LocalScope>(Private());
    node->parent = parent;
    node->short_name = std::to_string(next_scope_id++);
    node->block = block;

    return node;
}

bool Node::LocalScope::add_child(
    SymbolTree& symbol_tree,
    std::shared_ptr<Node::ILocatable> child,
    std::optional<std::string> custom_symbol
) {
    if (PTR_INSTANCEOF(child, Node::IGlobalScope)) {
        panic(
            "Node::LocalScope::add_child: Attempted to add global scope as a "
            "direct child to a local scope."
        );
        return false;
    }

    return Node::IScope::add_child(symbol_tree, child, custom_symbol);
}

std::shared_ptr<Node::BindingEntry> Node::BindingEntry::create(
    std::shared_ptr<Node::IScope> parent, const Binding& binding
) {
    auto node = std::make_shared<BindingEntry>(Private(), binding);
    node->parent = parent;
    node->short_name = binding.name;
    node->location = binding.location;
    node->is_global = PTR_INSTANCEOF(parent, Node::IGlobalScope);

    return node;
}

std::shared_ptr<Node::OverloadGroup> Node::OverloadGroup::create(
    std::shared_ptr<Node::IScope> parent,
    std::string_view overload_name,
    const Location* first_overload_location
) {
    auto node = std::make_shared<OverloadGroup>(
        Private(),
        overload_name,
        first_overload_location
    );
    node->parent = parent;
    node->short_name = std::string(overload_name);
    node->location = first_overload_location;
    node->is_global = PTR_INSTANCEOF(parent, Node::IGlobalScope);

    auto overloaded_fn_type =
        std::dynamic_pointer_cast<Type::OverloadedFn>(node->binding.type);
    overloaded_fn_type->overload_group = node;

    return node;
}

} // namespace nico
