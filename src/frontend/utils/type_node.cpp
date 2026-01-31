#include "nico/frontend/utils/type_node.h"

namespace nico {

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
