#include "nico/frontend/utils/type_node.h"

#include <vector>

namespace nico {

bool Type::Tuple::is_assignable_to(const Type& other) const {
    if (const auto* other_tuple = dynamic_cast<const Tuple*>(&other)) {
        if (elements.size() != other_tuple->elements.size()) {
            return false;
        }
        for (size_t i = 0; i < elements.size(); ++i) {
            if (!elements[i]->is_assignable_to(*(other_tuple->elements[i]))) {
                return false;
            }
        }
        return true;
    }
    else if (dynamic_cast<const Void*>(&other) != nullptr) {
        // A tuple can be assigned to void if it's empty.
        return elements.empty();
    }

    return false;
}

std::string Type::Named::to_string() const {
    if (auto node_ptr = node.lock()) {
        return node_ptr->symbol;
    }
    return "<expired>";
}

llvm::Type*
Type::Named::get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const {
    if (!is_sized_type().value_or(false)) {
        panic(
            "Type::Named::get_llvm_type: Cannot get LLVM type of unsized type."
        );
    }
    if (auto node_ptr = node.lock()) {
        return node_ptr->type->get_llvm_type(builder);
    }
    panic("Type::Named: Node is expired; LLVM type cannot be generated.");
}

} // namespace nico
