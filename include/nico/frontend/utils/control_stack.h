#ifndef NICO_CONTROL_STACK_H
#define NICO_CONTROL_STACK_H

#include <memory>
#include <string>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include "nico/frontend/utils/ast_node.h"

namespace nico {

/**
 * @brief A stack to manage control flow constructs during code generation.
 *
 * This class maintains a linked list stack of control flow blocks, which track
 * yield value allocation pointers and exit blocks for functions and loops.
 *
 * Note: Many of this class's methods will panic if a requested block is not
 * found. It is the caller's responsibility to ensure that the requested block
 * exists.
 */
class ControlStack {
    class Block;

    class Function;
    class Script;

    class Loop;

    // The top block in the control stack.
    std::shared_ptr<Block> top_block = nullptr;

public:
    /**
     * @brief Gets the name of the current function.
     *
     * A control stack should always start with a script block at the bottom,
     * so there is usually at least one function in the stack.
     *
     * @return The name of the current function.
     *
     * @warning This method will panic if there is no function in the stack.
     */
    std::string get_current_function_name() const;

    /**
     * @brief Gets the yield value allocation for the specified block kind and
     * label.
     *
     * @param kind The kind of block (Function, Loop, or Plain).
     * @param label An optional label to identify the block.
     *
     * @return The yield value allocation for the specified block.
     *
     * @warning This method will panic if the requested block is not found.
     */
    llvm::AllocaInst* get_yield_allocation(
        Expr::Block::Kind kind, std::optional<std::string> label = std::nullopt
    ) const;

    /**
     * @brief Gets the continue block for the nearest loop or loop with the
     * specified label.
     *
     * @param label An optional label to identify the loop block.
     * @return The continue block for the specified loop.
     * @warning This method will panic if the requested loop is not found.
     */
    llvm::BasicBlock*
    get_continue_block(std::optional<std::string> label = std::nullopt) const;

    /**
     * @brief Gets the exit block for the specified block kind and label.
     *
     * @param kind The kind of block (Function or Loop; Plain blocks have no
     * exit block).
     * @param label An optional label to identify the block.
     *
     * @return The exit block for the specified block.
     *
     * @warning This method will panic if the requested block is not found or if
     * kind is set to Plain.
     */
    llvm::BasicBlock* get_exit_block(
        Expr::Block::Kind kind, std::optional<std::string> label = std::nullopt
    ) const;

    /**
     * @brief Checks if the top block in the stack is a script block.
     *
     * @return true if the top block is a script block, false otherwise.
     *
     * @warning This method will panic if the control stack is empty.
     */
    bool top_block_is_script() const;

    /**
     * @brief Adds a plain block to the control stack.
     *
     * @param yield_allocation The yield value allocation for the block.
     * @param label An optional label for the block.
     */
    void add_block(
        llvm::AllocaInst* yield_allocation,
        std::optional<std::string> label = std::nullopt
    );

    /**
     * @brief Adds a script block to the control stack.
     *
     * The script block cannot have a label and must be at the bottom of the
     * stack.
     *
     * @param yield_allocation The yield value allocation for the script.
     * @param exit_block The exit block for the script.
     *
     * @warning This method will panic if the stack is not empty.
     */
    void add_script_block(
        llvm::AllocaInst* yield_allocation, llvm::BasicBlock* exit_block
    );

    /**
     * @brief Adds a loop block to the control stack.
     *
     * @param yield_allocation The yield value allocation for the loop.
     * @param merge_block The merge (exit) block for the loop.
     * @param continue_block The continue block for the loop.
     * @param label An optional label for the loop.
     */
    void add_loop_block(
        llvm::AllocaInst* yield_allocation,
        llvm::BasicBlock* merge_block,
        llvm::BasicBlock* continue_block,
        std::optional<std::string> label = std::nullopt
    );

    /**
     * @brief Removes the top block from the control stack.
     *
     * @warning This method will panic if the control stack is empty.
     */
    void pop_block();
};

/**
 * @brief Base class for control flow blocks in the control stack.
 *
 * All blocks store a pointer to the previous block, a yield value allocation,
 * and an optional label.
 */
class ControlStack::Block
    : public std::enable_shared_from_this<ControlStack::Block> {
public:
    // Pointer to the previous block in the stack, or nullptr if this is the
    // bottom block.
    std::shared_ptr<Block> prev;
    // The yield value allocation for this block.
    llvm::AllocaInst* const yield_allocation;
    // An optional label for this block.
    const std::optional<std::string> label;

    Block(
        std::shared_ptr<Block> prev,
        llvm::AllocaInst* yield_allocation,
        const std::optional<std::string> label = std::nullopt
    )
        : prev(prev), yield_allocation(yield_allocation), label(label) {}

    virtual ~Block() = default;

    /**
     * @brief Gets the top function block in the stack.
     *
     * @return The top function block in the stack, or nullptr if no function
     * block is found.
     */
    virtual std::shared_ptr<Function> get_function() {
        if (prev) {
            return prev->get_function();
        }
        else {
            return nullptr;
        }
    }

    /**
     * @brief Gets the top loop block in the stack with the specified label.
     *
     * @param label An optional label to identify the loop block.
     *
     * @return The top loop block with the specified label, or nullptr if no
     * matching loop block is found.
     */
    virtual std::shared_ptr<Loop>
    get_loop(std::optional<std::string> label = std::nullopt) {
        if (prev) {
            return prev->get_loop(label);
        }
        else {
            return nullptr;
        }
    }

    /**
     * @brief Gets the block with the specified label.
     *
     * @param label An optional label to identify the block.
     *
     * @return The block with the specified label, or nullptr if no matching
     * block is found.
     */
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

/**
 * @brief Represents a function block in the control stack.
 *
 * Function blocks store an exit block and the function name.
 */
class ControlStack::Function : public ControlStack::Block {
public:
    // The exit block for this function.
    llvm::BasicBlock* const exit_block;
    // The name of this function.
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

/**
 * @brief Represents a script block in the control stack.
 *
 * Script blocks are a special type of function block that represent the top-
 * level script context.
 */
class ControlStack::Script : public ControlStack::Function {
public:
    Script(llvm::AllocaInst* yield_allocation, llvm::BasicBlock* exit_block)
        : Function(
              nullptr, yield_allocation, exit_block, "script", std::nullopt
          ) {}
};

/**
 * @brief Represents a loop block in the control stack.
 *
 * Loop blocks store merge and continue blocks for loop control flow.
 */
class ControlStack::Loop : public ControlStack::Block {
public:
    // The merge (exit) block for this loop.
    llvm::BasicBlock* const merge_block;
    // The continue block for this loop.
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
