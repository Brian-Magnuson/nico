#include "nico/frontend/utils/symbol_node.h"

#include "nico/frontend/utils/type_node.h"
#include "nico/shared/logger.h"
#include "nico/shared/sets.h"

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

std::pair<std::shared_ptr<Node::FieldEntry>, Err>
Node::FieldEntry::create_as_overload(
    std::weak_ptr<Node::IScope> parent_scope,
    std::shared_ptr<Node::OverloadGroup> overload_group,
    const Field& field
) {
    auto node = std::make_shared<FieldEntry>(Private(), field);
    node->parent = parent_scope;
    node->short_name = field.name;
    node->location = field.location;

    if (parent_scope.expired()) {
        panic("Node::FieldEntry::create_as_overload: Parent scope is expired.");
    }
    node->symbol = parent_scope.lock()->symbol + "::" + node->short_name;
    node->is_global = PTR_INSTANCEOF(parent_scope.lock(), Node::IGlobalScope);

    auto func_type =
        std::dynamic_pointer_cast<Type::Function>(node->field.type);
    if (!func_type)
        panic("Field added as overloadable function is not a function.");
    auto [m_f1, d_f1] = func_type->get_param_sets();

    // Check for overload conflicts.
    std::vector<std::shared_ptr<Node::FieldEntry>> conflicts;
    for (const auto& existing_overload : overload_group->overloads) {
        auto existing_func_type = std::dynamic_pointer_cast<Type::Function>(
            existing_overload->field.type
        );
        if (!existing_func_type)
            panic("Existing overload in overload group is not a function.");
        auto [m_f2, d_f2] = existing_func_type->get_param_sets();
        auto conflict_found =
            sets::equals(m_f1, m_f2) ||
            (sets::subset(m_f2, m_f1) &&
             sets::subseteq(sets::difference(m_f1, m_f2), d_f1)) ||
            (sets::subset(m_f1, m_f2) &&
             sets::subseteq(sets::difference(m_f2, m_f1), d_f2));

        if (conflict_found) {
            conflicts.push_back(existing_overload);
        }
    }
    if (!conflicts.empty()) {
        Logger::inst().log_error(
            Err::FunctionOverloadConflict,
            *field.location,
            "Function overload conflict for function `" + field.name + "`."
        );
        for (const auto& conflict : conflicts) {
            if (auto locatable =
                    std::dynamic_pointer_cast<Node::ILocatable>(conflict)) {
                Logger::inst().log_note(
                    locatable->location,
                    "Conflicting overload declared here."
                );
            }
            Logger::inst().log_note(
                "Two function overloads conflict if they have the same set of "
                "parameters, or if one set of parameters is a superset of the "
                "other, differing only by optional parameters."
            );
        }
        return std::make_pair(conflicts.at(0), Err::FunctionOverloadConflict);
    }

    overload_group->overloads.push_back(node);
    // Change the new overload's symbol to include a $N suffix to make it
    // unique.
    node->symbol += "$" + std::to_string(overload_group->overloads.size());

    return {node, Err::Null};
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
