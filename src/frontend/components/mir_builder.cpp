#include "nico/frontend/components/mir_builder.h"

#include <memory>
#include <utility>
#include <vector>

#include "nico/frontend/utils/mir_instructions.h"

namespace nico {

void MIRBuilder::add_negative_alloc_size_check(
    std::shared_ptr<MIRValue> size_value, const Location* location
) {
    auto function = current_block->get_parent_function();

    auto panic_block = function->create_basic_block("panic");
    auto continue_block = function->create_basic_block("continue");

    auto zero_value =
        MIRValue::CustomInt::create(std::make_shared<Type::Int>(false, 64), 0);

    auto cmp_instr = std::make_shared<Instr::Binary>(
        Expr::Binary::Operation::SIntLT,
        size_value,
        zero_value,
        std::make_shared<Type::Bool>()
    );
    current_block->add_instruction(cmp_instr);

    // Branch based on the comparison result.
    current_block
        ->set_successors(cmp_instr->destination, panic_block, continue_block);

    // Panic block: Add a panic instruction and terminate the block.
    current_block = panic_block;
    auto panic_instr = std::make_shared<Instr::Panic>(
        "Allocation amount expression evaluated to a negative value.",
        location
    );
    current_block->add_instruction(panic_instr);
    current_block->set_successor(function->get_exit_block());

    // Continue block: Set the current block to continue building.
    current_block = continue_block;
}

std::shared_ptr<MIRValue::Variable> MIRBuilder::get_mir_variable(
    std::shared_ptr<Node::BindingEntry> binding_entry
) {
    if (binding_entry->is_global) {
        return mir_module->get_or_declare_global(binding_entry);
    }
    else {
        return current_block->get_parent_function()->get_local_variable(
            binding_entry
        );
    }
}

std::any MIRBuilder::visit(Stmt::Expression* stmt) {
    stmt->expression->accept(this, false);
    return std::any();
}

std::any MIRBuilder::visit(Stmt::Let* stmt) {
    auto binding_entry = stmt->binding_entry.lock();

    std::shared_ptr<MIRValue::Variable> mir_var;

    if (binding_entry->is_global) {
        // For global variables, we don't need to create a local variable in the
        // MIR. Instead, we can directly use the global variable.
        mir_var = mir_module->get_or_declare_global(binding_entry);
    }
    else {
        // For local variables, create a new MIR variable and add it to the
        // current function's locals.
        mir_var = current_block->get_parent_function()->create_local_variable(
            binding_entry
        );

        auto local_instr = std::make_shared<Instr::Local>(
            mir_var,
            binding_entry->binding.type
        );
        current_block->add_instruction(local_instr);
    }

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
    auto mir_val = std::any_cast<std::shared_ptr<MIRValue>>(
        stmt->expression->accept(this, false)
    );
    auto dealloc_instr = std::make_shared<Instr::Dealloc>(mir_val);
    current_block->add_instruction(dealloc_instr);

    return std::any();
}

std::any MIRBuilder::visit(Stmt::Pass* stmt) {
    // TODO: Implementation for visiting Pass statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Yield* stmt) {
    // Evaluate the expression to yield.
    auto yield_value = std::any_cast<std::shared_ptr<MIRValue>>(
        stmt->expression->accept(this, false)
    );

    // Determine if this yield statement requires an unreachable block after it.
    bool require_unreachable_block = false;

    if (stmt->yield_token->tok_type == Tok::KwYield) {
        Expr::Block::Kind target_kind = stmt->target_block.lock()->kind;
        auto yield_allocation =
            current_block->get_parent_function()->get_yield_variable(
                target_kind
            );
        auto store_instr =
            std::make_shared<Instr::Store>(yield_value, yield_allocation);
        current_block->add_instruction(store_instr);
    }
    else if (stmt->yield_token->tok_type == Tok::KwBreak) {
        // For break statements, find the nearest enclosing loop block.
        auto yield_allocation =
            current_block->get_parent_function()->get_yield_variable(
                Expr::Block::Kind::Loop
            );
        auto exit_block = current_block->get_parent_function()->get_exit_block(
            Expr::Block::Kind::Loop
        );
        auto store_instr =
            std::make_shared<Instr::Store>(yield_value, yield_allocation);
        current_block->add_instruction(store_instr);
        current_block->set_successor(exit_block);
        require_unreachable_block = true;
    }
    else if (stmt->yield_token->tok_type == Tok::KwReturn) {
        // For return statements, find the nearest enclosing function block.
        auto yield_allocation =
            current_block->get_parent_function()->get_yield_variable(
                Expr::Block::Kind::Function
            );
        auto exit_block = current_block->get_parent_function()->get_exit_block(
            Expr::Block::Kind::Function
        );
        auto store_instr =
            std::make_shared<Instr::Store>(yield_value, yield_allocation);
        current_block->add_instruction(store_instr);
        current_block->set_successor(exit_block);
        require_unreachable_block = true;
    }
    else {
        panic(
            "Expected yield token; got " +
            std::string(stmt->yield_token->lexeme) + "."
        );
        return std::any();
    }

    if (require_unreachable_block) {
        // After a break or return, we need to create a new block to continue
        // generating code in.
        current_block =
            current_block->get_parent_function()->create_basic_block(
                "unreachable"
            );
        // We allow statements to appear after a break or return, but they will
        // unreachable.
        // In the future, we can change prevent code generation after a break or
        // return.
    }

    return std::any();
}

std::any MIRBuilder::visit(Stmt::Continue* stmt) {
    auto continue_block =
        current_block->get_parent_function()->get_loop_continue_block();
    current_block->set_successor(continue_block);

    auto unreachable_block =
        current_block->get_parent_function()->create_basic_block("unreachable");
    current_block = unreachable_block;

    return std::any();
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
    std::shared_ptr<MIRValue> result;

    auto left_ptr = std::any_cast<std::shared_ptr<MIRValue>>(
        expr->left->accept(this, true)
    );
    auto right = std::any_cast<std::shared_ptr<MIRValue>>(
        expr->right->accept(this, false)
    );

    auto store_instr = std::make_shared<Instr::Store>(right, left_ptr);
    current_block->add_instruction(store_instr);
    result = right;

    return result;
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
    std::shared_ptr<MIRValue> result;

    auto sizeof_instr = std::make_shared<Instr::SizeOf>(expr->inner_type);
    current_block->add_instruction(sizeof_instr);

    result = sizeof_instr->destination;
    return result;
}

std::any MIRBuilder::visit(Expr::Alloc* expr, bool as_lvalue) {
    std::shared_ptr<MIRValue> result;
    std::shared_ptr<MIRValue> alloc_size;

    auto pointer_type = Type::as_a<Type::RawTypedPtr>(expr->type).value();

    if (expr->amount_expr.has_value()) {
        auto sizeof_instr = std::make_shared<Instr::SizeOf>(
            Type::as_a<Type::Array>(pointer_type->base).value()->base
        );
        current_block->add_instruction(sizeof_instr);
        alloc_size = sizeof_instr->destination;

        auto amount_value = std::any_cast<std::shared_ptr<MIRValue>>(
            expr->amount_expr.value()->accept(this, false)
        );

        // amount_value is definitely an integer, but may not be a u64.
        // Use SExt to convert it.
        if (Type::as_a<Type::Int>(expr->amount_expr.value()->type)
                .value()
                ->width != 64) {
            auto sext_instr = std::make_shared<Instr::Cast>(
                Expr::Cast::Operation::SignExt,
                amount_value,
                std::make_shared<Type::Int>(false, 64)
            );
            current_block->add_instruction(sext_instr);
            amount_value = sext_instr->destination;
        }
        add_negative_alloc_size_check(
            amount_value,
            expr->amount_expr.value()->location
        );

        auto mul_instr = std::make_shared<Instr::Binary>(
            Expr::Binary::Operation::IntMul,
            alloc_size,
            amount_value,
            std::make_shared<Type::Int>(false, 64)
        );
        current_block->add_instruction(mul_instr);
        alloc_size = mul_instr->destination;
    }
    else {
        auto sizeof_instr = std::make_shared<Instr::SizeOf>(pointer_type->base);
        current_block->add_instruction(sizeof_instr);
        alloc_size = sizeof_instr->destination;
    }

    auto alloc_instr = std::make_shared<Instr::Alloc>(alloc_size);
    current_block->add_instruction(alloc_instr);

    result = alloc_instr->destination;
    return result;
}

std::any MIRBuilder::visit(Expr::NewInst* expr, bool as_lvalue) {
    // TODO: Implementation for visiting NewInst expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::NameRef* expr, bool as_lvalue) {
    std::shared_ptr<MIRValue> result;
    std::shared_ptr<MIRValue::Variable> mir_var =
        get_mir_variable(expr->binding_entry.lock());

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

    result = MIRValue::Literal::create(expr->type, expr->token);
    return result;
}

std::any MIRBuilder::visit(Expr::Tuple* expr, bool as_lvalue) {
    std::shared_ptr<MIRValue> result;
    if (expr->is_constant()) {
        std::vector<std::shared_ptr<MIRValue::IConstant>> element_constants;
        for (const auto& element : expr->elements) {
            auto value = std::any_cast<std::shared_ptr<MIRValue>>(
                element->accept(this, false)
            );
            if (auto const_value =
                    std::dynamic_pointer_cast<MIRValue::IConstant>(value)) {
                element_constants.push_back(const_value);
            }
            else {
                panic(
                    "Expected constant value for tuple element, but got a "
                    "non-constant value."
                );
            }
        }
        result = MIRValue::Struct::create(expr->type, element_constants);
    }
    else {
        std::vector<std::shared_ptr<MIRValue>> element_values;
        for (const auto& element : expr->elements) {
            element_values.push_back(
                std::any_cast<std::shared_ptr<MIRValue>>(
                    element->accept(this, false)
                )
            );
        }
        auto struct_instr =
            std::make_shared<Instr::Struct>(expr->type, element_values);
        current_block->add_instruction(struct_instr);
        result = struct_instr->destination;
    }

    return result;
}

std::any MIRBuilder::visit(Expr::Array* expr, bool as_lvalue) {
    std::shared_ptr<MIRValue> result;
    if (!Type::is_a<Type::Array>(expr->type)) {
        panic(
            "Expected array type for array expression, but got `" +
            expr->type->to_string() + "`."
        );
    }
    auto array_type = Type::as_a<Type::Array>(expr->type).value();

    if (expr->is_constant()) {
        std::vector<std::shared_ptr<MIRValue::IConstant>> element_constants;
        for (const auto& element : expr->elements) {
            auto value = std::any_cast<std::shared_ptr<MIRValue>>(
                element->accept(this, false)
            );
            if (auto const_value =
                    std::dynamic_pointer_cast<MIRValue::IConstant>(value)) {
                element_constants.push_back(const_value);
            }
            else {
                panic(
                    "Expected constant value for array element, but got a "
                    "non-constant value."
                );
            }
        }
        result = MIRValue::Array::create(array_type, element_constants);
    }
    else {
        std::vector<std::shared_ptr<MIRValue>> element_values;
        for (const auto& element : expr->elements) {
            element_values.push_back(
                std::any_cast<std::shared_ptr<MIRValue>>(
                    element->accept(this, false)
                )
            );
        }
        auto array_instr =
            std::make_shared<Instr::Array>(expr->type, element_values);
        current_block->add_instruction(array_instr);
        result = array_instr->destination;
    }

    return result;
}

std::any MIRBuilder::visit(Expr::Object* expr, bool as_lvalue) {
    std::shared_ptr<MIRValue> result;
    if (expr->is_constant()) {
        std::vector<std::shared_ptr<MIRValue::IConstant>> field_constants;

        for (const auto& field : expr->fields) {
            auto value = std::any_cast<std::shared_ptr<MIRValue>>(
                field.expression->accept(this, false)
            );
            if (auto const_value =
                    std::dynamic_pointer_cast<MIRValue::IConstant>(value)) {
                field_constants.push_back(const_value);
            }
            else {
                panic(
                    "Expected constant value for struct field, but got a "
                    "non-constant value."
                );
            }
        }
        result = MIRValue::Struct::create(expr->type, field_constants);
    }
    else {
        std::vector<std::shared_ptr<MIRValue>> field_values;
        for (const auto& field : expr->fields) {
            field_values.push_back(
                std::any_cast<std::shared_ptr<MIRValue>>(
                    field.expression->accept(this, false)
                )
            );
        }
        auto struct_instr =
            std::make_shared<Instr::Struct>(expr->type, field_values);
        current_block->add_instruction(struct_instr);
        result = struct_instr->destination;
    }

    return result;
}

std::any MIRBuilder::visit(Expr::Block* expr, bool as_lvalue) {
    std::shared_ptr<MIRValue> result;

    auto yield_val = MIRValue::Variable::create("$yieldval", expr->type);

    auto local_instr = std::make_shared<Instr::Local>(yield_val, expr->type);
    current_block->add_instruction(local_instr);

    auto function = current_block->get_parent_function();
    function->add_plain_control_block(yield_val);

    for (const auto& stmt : expr->statements) {
        stmt->accept(this);
    }

    auto load_instr = std::make_shared<Instr::Load>(yield_val, expr->type);
    current_block->add_instruction(load_instr);

    function->pop_control_block();

    result = load_instr->destination;

    return result;
}

std::any MIRBuilder::visit(Expr::Conditional* expr, bool as_lvalue) {
    std::shared_ptr<MIRValue> result;

    auto function = current_block->get_parent_function();

    auto then_block = function->create_basic_block("cond_then");
    auto else_block = function->create_basic_block("cond_else");
    auto merge_block = function->create_basic_block("cond_merge");

    auto condition = std::any_cast<std::shared_ptr<MIRValue>>(
        expr->condition->accept(this, false)
    );

    current_block->set_successors(condition, then_block, else_block);

    // Then block
    current_block = then_block;
    auto then_value = std::any_cast<std::shared_ptr<MIRValue>>(
        expr->then_branch->accept(this, false)
    );
    current_block->set_successor(merge_block);

    // Else block
    current_block = else_block;
    auto else_value = std::any_cast<std::shared_ptr<MIRValue>>(
        expr->else_branch->accept(this, false)
    );
    current_block->set_successor(merge_block);

    // Merge block
    current_block = merge_block;
    std::vector<
        std::pair<std::shared_ptr<BasicBlock>, std::shared_ptr<MIRValue>>>
        incoming_values({{then_block, then_value}, {else_block, else_value}});
    auto phi_instr =
        std::make_shared<Instr::Phi>(expr->type, std::move(incoming_values));
    current_block->add_instruction(phi_instr);

    result = phi_instr->destination;
    return result;
}

std::any MIRBuilder::visit(Expr::Loop* expr, bool as_lvalue) {
    std::shared_ptr<MIRValue> result;

    auto break_val = MIRValue::Variable::create("$breakval", expr->type);
    auto local_instr = std::make_shared<Instr::Local>(break_val, expr->type);
    current_block->add_instruction(local_instr);

    // Loops are complicated because they allow break statements, which
    // interrupt the control flow in a block. This potentially causes the yield
    // value of a plain block to become unreachable. So rather than trying to
    // salvage the yield value of the inner expression, we create a new yield
    // allocation for the loop itself. Break statements will have the ability to
    // set this yield allocation.

    auto current_function = current_block->get_parent_function();
    auto do_block = current_function->create_basic_block("loop_do");
    auto merge_block = current_function->create_basic_block("loop_merge");

    if (expr->condition.has_value()) {
        // Conditional loops have a condition block.
        auto condition_block =
            current_function->create_basic_block("loop_cond");
        current_function
            ->add_loop_control_block(break_val, condition_block, merge_block);

        // Conditional loops include while loops and do-while loops.
        // The flow is the same: cond->do->cond->do...
        // The difference is that, in do-while loops, we enter the do block
        // first.
        if (expr->loops_once) {
            current_block->set_successor(do_block);
        }
        else {
            current_block->set_successor(condition_block);
        }

        // Condition block
        current_block = condition_block;
        auto condition = std::any_cast<std::shared_ptr<MIRValue>>(
            expr->condition.value()->accept(this, false)
        );
        current_block->set_successors(condition, do_block, merge_block);

        // Do block
        current_block = do_block;
        // The loop body is always a block expression, but we ignore the value
        // because it is always `void`.
        expr->body->accept(this, false);
        current_block->set_successor(condition_block);
    }
    else {
        // Unconditional loops don't have a condition block.
        current_function
            ->add_loop_control_block(break_val, do_block, merge_block);
        current_block->set_successor(do_block);

        // Do block
        current_block = do_block;
        expr->body->accept(this, false);
        // For non-conditional loops, we also ignore the value of
        // the loop body because the yield value is set by break statements,
        // which are the only way to exit the loop.
        current_block->set_successor(do_block);
    }

    // Merge block
    current_block = merge_block;
    auto load_instr = std::make_shared<Instr::Load>(break_val, expr->type);
    current_block->add_instruction(load_instr);
    current_function->pop_control_block();
    result = load_instr->destination;

    return std::any();
}

void MIRBuilder::run_build(std::unique_ptr<FrontendContext>& context) {
    for (size_t i = context->stmts_processed; i < context->stmts.size(); ++i) {
        context->stmts[i]->accept(this);
    }
    current_block->set_successor(
        context->mir_module->get_script_function()->get_exit_block()
    );
}

void MIRBuilder::build_mir(std::unique_ptr<FrontendContext>& context) {
    if (IS_VARIANT(context->status, Status::Error)) {
        panic("Context is in an error state.");
    }

    MIRBuilder builder(context->mir_module, context->symbol_tree);
    builder.run_build(context);
}

} // namespace nico
