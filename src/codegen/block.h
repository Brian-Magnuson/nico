#ifndef NICO_BLOCK_H
#define NICO_BLOCK_H

#include <memory>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Value.h>

/**
 * @brief Base class for llvm block wrapper linked list nodes.
 *
 * Objects of this class store information about the current block in the code
 * generator. Each object also contains a pointer to the previous block, forming
 * a linked list stack.
 */
class Block {
public:
    class Function;
    class Script;

    class Control;
    class Plain;
    class Loop;
    class Conditional;

    // A pointer to the previous block.
    std::shared_ptr<Block> prev;
    // The value yielded by the block. If this is a function block,
    // this will be the return value.
    llvm::Value* yield_value;

    Block(std::shared_ptr<Block> prev, llvm::Value* yield_value)
        : prev(prev), yield_value(yield_value) {}

    virtual ~Block() = default;

    virtual std::string get_function_name() const {
        if (prev) {
            return prev->get_function_name();
        }
        else {
            return "<unknown>";
        }
    }
};

/**
 * @brief A function block linked list node.
 *
 * Function blocks store a pointer to the exit block.
 * When a return statement is encountered, control jumps to the exit block where
 * the yield value is returned.
 */
class Block::Function : public Block {
public:
    // This function's exit block where the yield value is returned.
    llvm::BasicBlock* exit_block;
    // The name of this function.
    std::string function_name;

    Function(
        std::shared_ptr<Block> prev, llvm::Value* yield_value,
        llvm::BasicBlock* exit_block, std::string_view function_name
    )
        : Block(prev, yield_value), exit_block(exit_block),
          function_name(function_name) {}

    virtual ~Function() = default;

    virtual std::string get_function_name() const override {
        return function_name;
    }
};

/**
 * @brief A script block linked list node.
 *
 * A script is an implicitly declared function containing all statements
 * written at the top level.
 * The difference is that variable declarations are made global.
 *
 * This class adds no additional members to `Block::Function`.
 */
class Block::Script : public Block::Function {
public:
    Script(
        std::shared_ptr<Block> prev, llvm::Value* yield_value,
        llvm::BasicBlock* exit_block
    )
        : Block::Function(prev, yield_value, exit_block, "script") {}

    virtual ~Script() = default;
};

/**
 * @brief A subclass for control-type blocks.
 *
 * Control blocks are blocks that make up control structures such as
 * conditionals and loops. These blocks have a "merge block", where control flow
 * continues after the control structure.
 */
class Block::Control : public Block {
public:
    // This control block's merge block where control flow continues.
    llvm::BasicBlock* merge_block;

    Control(
        std::shared_ptr<Block> prev, llvm::Value* yield_value,
        llvm::BasicBlock* merge_block
    )
        : Block(prev, yield_value), merge_block(merge_block) {}

    virtual ~Control() = default;
};

/**
 * @brief A plain block linked list node.
 *
 * This class is used to distinguish this block from other kinds of control
 * blocks like loops and conditionals. It adds no additional members to
 * `Block::Control`.
 *
 * Plain blocks, though considered control blocks, do not actually affect
 * control flow.
 */
class Block::Plain : public Block::Control {
public:
    Plain(
        std::shared_ptr<Block> prev, llvm::Value* yield_value,
        llvm::BasicBlock* merge_block
    )
        : Control(prev, yield_value, merge_block) {}

    virtual ~Plain() = default;
};

/**
 * @brief A loop block linked list node.
 *
 * Loop blocks are used for looping control structures.
 * These structures, in addition to having a merge block, also have a continue
 * block used to implement the loop's continuation behavior.
 */
class Block::Loop : public Block::Control {
public:
    // This loop's continue block, allowing control flow to restart from the
    // beginning of the loop.
    llvm::BasicBlock* continue_block;

    Loop(
        std::shared_ptr<Block> prev, llvm::Value* yield_value,
        llvm::BasicBlock* merge_block, llvm::BasicBlock* continue_block
    )
        : Control(prev, yield_value, merge_block),
          continue_block(continue_block) {}

    virtual ~Loop() = default;
};

/**
 * @brief A conditional block linked list node.
 *
 * This class is used to distinguish this block from other kinds of control
 * blocks like loops and plain blocks. It adds no additional members to
 * `Block::Control`.
 *
 * Conditional blocks are used for conditional control structures.
 * These structures have a merge block where control flow continues after the
 * conditional.
 */
class Block::Conditional : public Block::Control {
public:
    Conditional(
        std::shared_ptr<Block> prev, llvm::Value* yield_value,
        llvm::BasicBlock* merge_block
    )
        : Control(prev, yield_value, merge_block) {}

    virtual ~Conditional() = default;
};

#endif // NICO_BLOCK_H
