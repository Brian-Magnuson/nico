#include "nico/frontend/utils/nodes.h"

#include "nico/frontend/utils/symbol_tree.h"

namespace nico {

bool Node::set_symbol(
    const SymbolTree* symbol_tree, std::string_view new_symbol
) {
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
