#ifndef NICO_CONTROL_STACK_H
#define NICO_CONTROL_STACK_H

#include <memory>
#include <string>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include "nico/frontend/utils/ast_node.h"

namespace nico {

class ControlStack {
    class Block;

    class Function;
    class Script;

    class Loop;

    std::shared_ptr<Block> top_block = nullptr;

public:
    std::string get_current_function_name() const;

    llvm::AllocaInst* get_yield_allocation(
        Expr::Block::Kind kind, std::optional<std::string> label = std::nullopt
    ) const;

    llvm::BasicBlock* get_exit_block(
        Expr::Block::Kind kind, std::optional<std::string> label = std::nullopt
    ) const;

    bool top_block_is_script() const;

    void add_block(
        llvm::AllocaInst* yield_allocation,
        std::optional<std::string> label = std::nullopt
    );

    void add_script_block(
        llvm::AllocaInst* yield_allocation, llvm::BasicBlock* exit_block
    );

    void add_loop_block(
        llvm::AllocaInst* yield_allocation,
        llvm::BasicBlock* merge_block,
        llvm::BasicBlock* continue_block,
        std::optional<std::string> label = std::nullopt
    );

    void pop_block();
};

class ControlStack::Block
    : public std::enable_shared_from_this<ControlStack::Block> {
public:
    std::shared_ptr<Block> prev;
    llvm::AllocaInst* const yield_allocation;
    const std::optional<std::string> label;

    Block(
        std::shared_ptr<Block> prev,
        llvm::AllocaInst* yield_allocation,
        const std::optional<std::string> label = std::nullopt
    )
        : prev(prev), yield_allocation(yield_allocation), label(label) {}

    virtual ~Block() = default;

    virtual std::shared_ptr<Function> get_function() {
        if (prev) {
            return prev->get_function();
        }
        else {
            return nullptr;
        }
    }

    virtual std::shared_ptr<Loop>
    get_loop(std::optional<std::string> label = std::nullopt) {
        if (prev) {
            return prev->get_loop(label);
        }
        else {
            return nullptr;
        }
    }

    virtual std::shared_ptr<Block>
    get_block(std::optional<std::string> label = std::nullopt) {
        if (!label || this->label == label) {
            return shared_from_this();
        }
        else if (prev) {
            return prev->get_block(label);
        }
        else {
            return nullptr;
        }
    }
};

class ControlStack::Function : public ControlStack::Block {
public:
    llvm::BasicBlock* const exit_block;
    const std::string function_name;

    Function(
        std::shared_ptr<Block> prev,
        llvm::AllocaInst* yield_allocation,
        llvm::BasicBlock* exit_block,
        std::string_view function_name,
        const std::optional<std::string> label = std::nullopt
    )
        : Block(prev, yield_allocation, label),
          exit_block(exit_block),
          function_name(function_name) {}

    std::shared_ptr<Function> get_function() override {
        return std::dynamic_pointer_cast<Function>(shared_from_this());
    }

    std::shared_ptr<Loop>
    get_loop(std::optional<std::string> label = std::nullopt) override {
        return nullptr;
    }
};

class ControlStack::Script : public ControlStack::Function {
public:
    Script(llvm::AllocaInst* yield_allocation, llvm::BasicBlock* exit_block)
        : Function(
              nullptr, yield_allocation, exit_block, "script", std::nullopt
          ) {}
};

class ControlStack::Loop : public ControlStack::Block {
public:
    llvm::BasicBlock* const merge_block;
    llvm::BasicBlock* const continue_block;

    Loop(
        std::shared_ptr<Block> prev,
        llvm::AllocaInst* yield_allocation,
        llvm::BasicBlock* merge_block,
        llvm::BasicBlock* continue_block,
        const std::optional<std::string> label = std::nullopt
    )
        : Block(prev, yield_allocation, label),
          merge_block(merge_block),
          continue_block(continue_block) {}

    std::shared_ptr<Loop>
    get_loop(std::optional<std::string> label = std::nullopt) override {
        if (!label || this->label == label) {
            return std::dynamic_pointer_cast<Loop>(shared_from_this());
        }
        else if (prev) {
            return prev->get_loop(label);
        }
        else {
            return nullptr;
        }
    }
};

} // namespace nico

#endif // NICO_CONTROL_STACK_H
