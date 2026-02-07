#include "nico/frontend/components/mir_builder.h"

#include "nico/frontend/utils/mir_instructions.h"

namespace nico {

std::any MIRBuilder::visit(Stmt::Expression* stmt) {
    stmt->expression->accept(this, false);
    return std::any();
}

std::any MIRBuilder::visit(Stmt::Let* stmt) {
    auto field_entry = stmt->field_entry.lock();
    auto mir_var = std::make_shared<MIRValue::Variable>(field_entry);
    auto alloca_instr =
        std::make_shared<Instr::Alloca>(mir_var, field_entry->field.type);
    current_block->add_instruction(alloca_instr);

    if (stmt->expression.has_value()) {
        auto mir_val = std::any_cast<std::shared_ptr<MIRValue>>(
            stmt->expression.value()->accept(this, false)
        );
        auto store_instr = std::make_shared<Instr::Store>(mir_val, mir_var);
        current_block->add_instruction(store_instr);
    }
    return std::any();
}

std::any MIRBuilder::visit(Stmt::Func* stmt) {
    // TODO: Implementation for visiting Func statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Print* stmt) {
    // TODO: Implementation for visiting Print statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Dealloc* stmt) {
    // TODO: Implementation for visiting Dealloc statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Pass* stmt) {
    // TODO: Implementation for visiting Pass statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Yield* stmt) {
    // TODO: Implementation for visiting Yield statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Extern* stmt) {
    // TODO: Implementation for visiting Extern statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Continue* stmt) {
    // TODO: Implementation for visiting Continue statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Eof* stmt) {
    // TODO: Implementation for visiting Eof statements goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Assign* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Assign expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Logical* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Logical expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Binary* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Binary expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Unary* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Unary expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Address* expr, bool as_lvalue) {
    // The right expression is a possible lvalue.
    // Visiting it as one will give us its address.
    return std::any_cast<std::shared_ptr<MIRValue>>(
        expr->right->accept(this, true)
    );
}

std::any MIRBuilder::visit(Expr::Deref* expr, bool as_lvalue) {
    // The inner expression of a dereference is a pointer.
    auto mir_ptr = std::any_cast<std::shared_ptr<MIRValue>>(
        expr->right->accept(this, false)
    );
    if (as_lvalue) {
        // If we're treating this as an lvalue, return the pointer itself.
        return mir_ptr;
    }
    else {
        // Otherwise, we need to load the value from the pointer.
        auto load_instr = std::make_shared<Instr::Load>(mir_ptr);
        current_block->add_instruction(load_instr);

        return load_instr->destination;
    }
}

std::any MIRBuilder::visit(Expr::Cast* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Cast expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Access* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Access expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Subscript* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Subscript expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Call* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Call expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::SizeOf* expr, bool as_lvalue) {
    // TODO: Implementation for visiting SizeOf expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Alloc* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Alloc expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::NameRef* expr, bool as_lvalue) {
    // TODO: Implementation for visiting NameRef expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Literal* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Literal expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Tuple* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Tuple expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Array* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Array expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Block* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Block expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Conditional* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Conditional expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Loop* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Loop expressions goes here.
    return {};
}

void MIRBuilder::run_build() {
    // TODO: Implementation of MIR building goes here.
}

void MIRBuilder::build_mir(std::unique_ptr<FrontendContext>& context) {
    MIRBuilder builder(context->mir_module, context->symbol_tree);
    builder.run_build();
}

} // namespace nico
