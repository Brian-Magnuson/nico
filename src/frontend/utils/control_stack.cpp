#include "nico/frontend/utils/control_stack.h"

#include "nico/shared/utils.h"

namespace nico {

std::string ControlStack::get_current_function_name() const {
    if (top_block) {
        auto func = top_block->get_function();
        if (func) {
            return func->function_name;
        }
    }

    panic("ControlStack::get_current_function_name: No function in stack.");
}

llvm::AllocaInst* ControlStack::get_yield_allocation(
    Expr::Block::Kind kind, std::optional<std::string> label
) const {
    if (!top_block) {
        panic("ControlStack::get_yield_allocation: No block in stack.");
    }
    if (kind == Expr::Block::Kind::Plain) {
        return top_block->get_block(label)->yield_allocation;
    }
    else if (kind == Expr::Block::Kind::Function) {
        auto func = top_block->get_function();
        if (func) {
            return func->yield_allocation;
        }
    }
    else if (kind == Expr::Block::Kind::Loop) {
        auto loop = top_block->get_loop(label);
        if (loop) {
            return loop->yield_allocation;
        }
    }
    panic(
        "ControlStack::get_yield_allocation: Target block not found in stack."
    );
}

llvm::BasicBlock*
ControlStack::get_continue_block(std::optional<std::string> label) const {
    if (!top_block) {
        panic("ControlStack::get_continue_block: No block in stack.");
    }
    auto loop = top_block->get_loop(label);
    if (loop) {
        return loop->continue_block;
    }
    panic("ControlStack::get_continue_block: Target loop not found in stack.");
}

llvm::BasicBlock* ControlStack::get_exit_block(
    Expr::Block::Kind kind, std::optional<std::string> label
) const {
    if (!top_block) {
        panic("ControlStack::get_exit_block: No block in stack.");
    }
    if (kind == Expr::Block::Kind::Plain) {
        panic("ControlStack::get_exit_block: Plain blocks have no exit block.");
    }
    else if (kind == Expr::Block::Kind::Loop) {
        auto loop = top_block->get_loop(label);
        if (loop) {
            return loop->merge_block;
        }
    }
    else if (kind == Expr::Block::Kind::Function) {
        auto func = top_block->get_function();
        if (func) {
            return func->exit_block;
        }
    }
    panic("ControlStack::get_exit_block: Target block not found in stack.");
}

bool ControlStack::top_block_is_script() const {
    if (top_block) {
        return PTR_INSTANCEOF(top_block, ControlStack::Script);
    }
    else {
        panic("ControlStack::top_block_is_script: No block in stack.");
    }
}

void ControlStack::add_block(
    llvm::AllocaInst* yield_allocation, std::optional<std::string> label
) {
    top_block = std::make_shared<ControlStack::Block>(
        top_block,
        yield_allocation,
        label
    );
}

void ControlStack::add_script_block(
    llvm::AllocaInst* yield_allocation, llvm::BasicBlock* exit_block
) {
    if (top_block)
        panic(
            "ControlStack::add_script_block: Cannot add script block inside "
            "another block."
        );

    top_block =
        std::make_shared<ControlStack::Script>(yield_allocation, exit_block);
}

void ControlStack::add_loop_block(
    llvm::AllocaInst* yield_allocation,
    llvm::BasicBlock* merge_block,
    llvm::BasicBlock* continue_block,
    std::optional<std::string> label
) {
    top_block = std::make_shared<ControlStack::Loop>(
        top_block,
        yield_allocation,
        merge_block,
        continue_block,
        label
    );
}

void ControlStack::pop_block() {
    if (top_block) {
        top_block = top_block->prev;
    }
    else {
        panic("ControlStack::pop_block: No block to pop.");
    }
}

} // namespace nico
