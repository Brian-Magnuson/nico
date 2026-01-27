#include "nico/frontend/utils/symbol_node.h"

#include "nico/frontend/utils/type_node.h"

namespace nico {

int Node::LocalScope::next_scope_id = 0;

Node::Node(
    Node::Private,
    std::weak_ptr<Node::IScope> parent_scope,
    std::string_view identifier
)
    : parent(parent_scope),
      symbol(
          parent_scope.lock()
              ? parent_scope.lock()->symbol + "::" + std::string(identifier)
              : std::string(identifier)
      ),
      short_name(identifier) {}

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

std::shared_ptr<Node::StructDef> Node::StructDef::create(
    std::weak_ptr<Node::IScope> parent_scope,
    std::shared_ptr<Token> token,
    bool is_class
) {
    auto node = std::make_shared<StructDef>(Private());
    node->parent = parent_scope;
    node->short_name = std::string(token->lexeme);
    node->location = &token->location;
    node->is_class = is_class;

    if (parent_scope.expired()) {
        panic("Node::StructDef::create: Parent scope is expired.");
    }
    node->symbol = parent_scope.lock()->symbol + "::" + node->short_name;
    parent_scope.lock()->children[std::string(token->lexeme)] = node;

    node->type = std::make_shared<Type::Named>(node);
    return node;
}

std::shared_ptr<Node::OverloadGroup> Node::OverloadGroup::create(
    std::weak_ptr<Node::IScope> parent_scope,
    std::string_view overload_name,
    const Location* first_overload_location
) {
    auto node = std::make_shared<OverloadGroup>(
        Private(),
        overload_name,
        first_overload_location
    );
    node->parent = parent_scope;
    node->short_name = std::string(overload_name);
    node->location = first_overload_location;

    if (parent_scope.expired()) {
        panic("Node::OverloadGroup::create: Parent scope is expired.");
    }
    parent_scope.lock()->children[std::string(overload_name)] = node;
    node->symbol = parent_scope.lock()->symbol + "::" + node->short_name;
    node->is_global = PTR_INSTANCEOF(parent_scope.lock(), Node::IGlobalScope);

    auto overloaded_fn_type =
        std::dynamic_pointer_cast<Type::OverloadedFn>(node->field.type);
    overloaded_fn_type->overload_group = node;
    return node;
}

} // namespace nico
