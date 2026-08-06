#include "nico/frontend/components/mir_builder.h"

#include <memory>

#include "nico/frontend/utils/mir_instructions.h"

namespace nico {

std::any MIRBuilder::visit(Stmt::Expression* stmt) {
    stmt->expression->accept(this, false);
    return std::any();
}

std::any MIRBuilder::visit(Stmt::Let* stmt) {
    auto binding_entry = stmt->binding_entry.lock();
    auto mir_var = MIRValue::Variable::create(binding_entry);

    auto alloca_instr =
        std::make_shared<Instr::Alloca>(mir_var, binding_entry->binding.type);
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

std::any MIRBuilder::visit(Stmt::Static* stmt) {
    // TODO: Implement static variables. This will likely involve creating a
    // global variable.
    return std::any();
}

std::any MIRBuilder::visit(Stmt::Func* stmt) {
    // TODO: Implementation for visiting Func statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Print* stmt) {
    std::vector<std::shared_ptr<MIRValue>> mir_values;
    for (const auto& expr : stmt->expressions) {
        auto mir_val =
            std::any_cast<std::shared_ptr<MIRValue>>(expr->accept(this, false));
        mir_values.push_back(mir_val);
    }
    auto printout_instr = std::make_shared<Instr::Printout>(mir_values);
    current_block->add_instruction(printout_instr);

    return std::any();
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

std::any MIRBuilder::visit(Stmt::Continue* stmt) {
    // TODO: Implementation for visiting Continue statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Namespace* stmt) {
    // TODO: Implementation for visiting Namespace statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::ExternBlock* stmt) {
    // TODO: Implementation for visiting Extern statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::TypeDef* stmt) {
    // TODO: Implementation for visiting TypeDef statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::StructDef* stmt) {
    // TODO: Implementation for visiting StructDef statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Field* stmt) {
    // TODO: Implementation for visiting Field statements goes here.
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
    std::shared_ptr<MIRValue> result;

    auto left_operand = std::any_cast<std::shared_ptr<MIRValue>>(
        expr->left->accept(this, false)
    );
    auto right_operand = std::any_cast<std::shared_ptr<MIRValue>>(
        expr->right->accept(this, false)
    );

    auto binary_instr = std::make_shared<Instr::Binary>(
        expr->operation,
        left_operand,
        right_operand,
        expr->type
    );

    current_block->add_instruction(binary_instr);
    result = binary_instr->destination;

    return result;
}

std::any MIRBuilder::visit(Expr::Unary* expr, bool as_lvalue) {
    std::shared_ptr<MIRValue> result;

    auto operand = std::any_cast<std::shared_ptr<MIRValue>>(
        expr->right->accept(this, false)
    );

    auto unary_instr =
        std::make_shared<Instr::Unary>(expr->operation, operand, expr->type);

    current_block->add_instruction(unary_instr);
    result = unary_instr->destination;

    return result;
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
        auto load_instr = std::make_shared<Instr::Load>(mir_ptr, expr->type);
        current_block->add_instruction(load_instr);

        return load_instr->destination;
    }
}

std::any MIRBuilder::visit(Expr::Cast* expr, bool as_lvalue) {
    std::shared_ptr<MIRValue> result;

    auto operand = std::any_cast<std::shared_ptr<MIRValue>>(
        expr->expression->accept(this, false)
    );

    auto cast_instr =
        std::make_shared<Instr::Cast>(expr->operation, operand, expr->type);

    current_block->add_instruction(cast_instr);
    result = cast_instr->destination;

    return result;
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

std::any MIRBuilder::visit(Expr::NewInst* expr, bool as_lvalue) {
    // TODO: Implementation for visiting NewInst expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::NameRef* expr, bool as_lvalue) {
    std::shared_ptr<MIRValue> result;
    std::shared_ptr<MIRValue::Variable> mir_var =
        expr->binding_entry.lock()->mir_variable;

    if (as_lvalue) {
        // If we're treating this as an lvalue, return the variable itself.
        result = mir_var;
    }
    else {
        // Otherwise, we need to load the value from the variable.
        auto load_instr = std::make_shared<Instr::Load>(mir_var, expr->type);
        current_block->add_instruction(load_instr);

        result = load_instr->destination;
    }
    return result;
}

std::any MIRBuilder::visit(Expr::Literal* expr, bool as_lvalue) {
    std::shared_ptr<MIRValue> result;

    result = MIRValue::Literal::create(
        expr->type,
        std::dynamic_pointer_cast<Expr::Literal>(expr->shared_from_this())
    );
    return result;
}

std::any MIRBuilder::visit(Expr::Tuple* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Tuple expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Array* expr, bool as_lvalue) {
    std::shared_ptr<MIRValue> result;

    std::vector<std::shared_ptr<MIRValue>> element_values;
    for (const auto& element : expr->elements) {
        element_values.push_back(
            std::any_cast<std::shared_ptr<MIRValue>>(
                element->accept(this, false)
            )
        );
    }
    auto array_instr = std::make_shared<Instr::Array>(
        expr->type,
        element_values,
        expr->is_constant()
    );
    current_block->add_instruction(array_instr);
    result = array_instr->destination;

    return result;
}

std::any MIRBuilder::visit(Expr::Object* expr, bool as_lvalue) {
    // TODO: Implementation for visiting Object expressions goes here.
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

void MIRBuilder::run_build(std::unique_ptr<FrontendContext>& context) {
    for (size_t i = context->stmts_processed; i < context->stmts.size(); ++i) {
        context->stmts[i]->accept(this);
    }
    context->mir_module->get_script_function()
        ->get_entry_block()
        ->set_successor(
            context->mir_module->get_script_function()->get_exit_block().value()
        );
}

void MIRBuilder::build_mir(std::unique_ptr<FrontendContext>& context) {
    if (IS_VARIANT(context->status, Status::Error)) {
        panic("MIRBuilder::run_build: Context is in an error state.");
    }

    MIRBuilder builder(context->mir_module, context->symbol_tree);
    builder.run_build(context);
}

} // namespace nico
