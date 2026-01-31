#include "nico/frontend/utils/nodes.h"

#include "nico/frontend/utils/symbol_tree.h"

namespace nico {

bool Node::add_to_tree(const SymbolTree* symbol_tree) {
    // TODO: Check for symbol uniqueness in the symbol tree.
    if (auto current_scope = symbol_tree->current_scope) {
        // Set this node's parent to the current scope.
        parent = current_scope;
        // Add this node to the current scope's children.
        current_scope->children[short_name] = shared_from_this();
        return true;
    }
    return false;
}

bool Node::set_symbol(
    const SymbolTree* symbol_tree, std::string_view new_symbol
) {
    // TODO: Check for symbol uniqueness in the symbol tree.
    symbol = std::string(new_symbol);
    return true;
}

bool Node::set_symbol_using_parent(const SymbolTree* symbol_tree) {
    if (auto parent_ptr = parent.lock()) {
        return set_symbol(
            symbol_tree,
            parent_ptr->get_symbol() + "::" + short_name
        );
    }
    return false;
}

} // namespace nico
