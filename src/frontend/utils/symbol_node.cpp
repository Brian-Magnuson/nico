#include "nico/frontend/utils/symbol_node.h"

#include "nico/frontend/utils/type_node.h"

namespace nico {

int Node::LocalScope::next_scope_id = 0;

Node::Node(
    std::weak_ptr<Node::IScope> parent_scope, const std::string& identifier
)
    : parent(parent_scope),
      symbol(
          parent_scope.lock() ? parent_scope.lock()->symbol + "::" + identifier
                              : identifier
      ),
      short_name(identifier) {}

void Node::initialize_node() {
    auto shared_this = shared_from_this();
    if (PTR_INSTANCEOF(shared_this, Node::RootScope)) {
        // Root scope does not have a parent, so we do nothing.
        return;
    }
    if (auto type_node =
            std::dynamic_pointer_cast<Node::StructDef>(shared_this)) {
        type_node->type = std::make_shared<Type::Named>(type_node);
    }
    // If this is an overload group...
    if (auto overload_group_node =
            std::dynamic_pointer_cast<Node::OverloadGroup>(shared_this)) {
        // Set the overload group's type's back-reference to this node.
        auto overloaded_fn_type = std::dynamic_pointer_cast<Type::OverloadedFn>(
            overload_group_node->field.type
        );
        overloaded_fn_type->overload_group = overload_group_node;
    }
    if (parent.expired()) {
        panic("Node::initialize_node: Parent scope is expired.");
    }
    auto parent_ptr = parent.lock();
    if (parent_ptr) {
        if (auto local_scope =
                std::dynamic_pointer_cast<Node::LocalScope>(shared_this)) {
            parent_ptr->local_scopes.push_back(local_scope);
        }
        else {
            parent_ptr->children[short_name] = shared_this;
        }
    }
}

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

} // namespace nico
