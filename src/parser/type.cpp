#include "type.h"

Node::Node(std::weak_ptr<Node::IScope> parent_scope, const std::string& name)
    : parent(parent_scope),
      unique_name(parent_scope.lock() ? parent_scope.lock()->unique_name + "::" + name : name),
      short_name(name) {}

void Node::add_self_to_parent() {
    auto shared_this = shared_from_this();
    if (std::dynamic_pointer_cast<Node::RootScope>(shared_this)) {
        // Root scope does not have a parent, so we do nothing.
        return;
    }
    if (auto type_node = std::dynamic_pointer_cast<Node::ITypeNode>(shared_this)) {
        type_node->type = std::make_shared<Type::Named>(type_node);
    }
    if (parent.expired()) {
        std::cerr << "Node::add_self_to_parent: Parent scope is expired." << std::endl;
        std::abort();
    }
    auto parent_ptr = parent.lock();
    if (parent_ptr) {
        parent_ptr->children[short_name] = shared_this;
    }
}
