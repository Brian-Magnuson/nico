#include "nico/frontend/utils/type_node.h"

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
    if (auto node_ptr = node.lock()) {
        llvm::StructType* struct_ty = llvm::StructType::getTypeByName(
            builder->getContext(),
            node_ptr->symbol
        );
        if (!struct_ty) {
            struct_ty = llvm::StructType::create(
                builder->getContext(),
                node_ptr->symbol
            );
        }
        return struct_ty;
    }
    panic("Type::Named: Node is expired; LLVM type cannot be generated.");
}

} // namespace nico
