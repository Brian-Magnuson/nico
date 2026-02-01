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

std::shared_ptr<Node::Namespace> Node::Namespace::create(
    std::shared_ptr<Node::IScope> parent, std::shared_ptr<Token> token
) {
    auto node = std::make_shared<Namespace>(Private());
    node->parent = parent;
    node->short_name = std::string(token->lexeme);
    node->location = &token->location;
    return node;
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

std::shared_ptr<Node::LocalScope> Node::LocalScope::create(
    std::shared_ptr<Node::IScope> parent, std::shared_ptr<Expr::Block> block
) {
    auto node = std::make_shared<LocalScope>(Private());
    node->parent = parent;
    node->short_name = std::to_string(next_scope_id++);
    node->block = block;

    return node;
}

std::shared_ptr<Node::FieldEntry> Node::FieldEntry::create(
    std::shared_ptr<Node::IScope> parent, const Field& field
) {
    auto node = std::make_shared<FieldEntry>(Private(), field);
    node->parent = parent;
    node->short_name = field.name;
    node->location = field.location;
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
        std::dynamic_pointer_cast<Type::OverloadedFn>(node->field.type);
    overloaded_fn_type->overload_group = node;

    return node;
}

} // namespace nico
