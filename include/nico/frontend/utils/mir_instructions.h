#ifndef NICO_MIR_INSTRUCTIONS_H
#define NICO_MIR_INSTRUCTIONS_H

#include <memory>

#include "nico/frontend/utils/mir.h"

namespace nico {

/**
 * @brief A non-terminator instruction in the MIR.
 *
 * Non-terminator instructions perform operations but do not alter the control
 * flow. They typically make up most of the instructions within a basic block.
 *
 * Basic blocks in the MIR contain zero or more non-terminator instructions
 * followed by exactly one terminator instruction.
 */
class Instruction::INonTerminator : public Instruction {
public:
    virtual ~INonTerminator() = default;
};

class Instruction::Binary : public INonTerminator {
    // Implementation details...
};

class Instruction::Unary : public INonTerminator {
    // Implementation details...
};

class Instruction::Call : public INonTerminator {
    // Implementation details...
};

/**
 * @brief A terminator instruction in the MIR.
 *
 * Terminator instructions alter the control flow of a basic block. They
 * include jumps, branches, and returns.
 *
 * A basic block must have exactly one terminator instruction, which is executed
 * after all the non-terminator instructions.
 */
class Instruction::ITerminator : public Instruction {
    friend class BasicBlock;

public:
    virtual ~ITerminator() = default;

protected:
    ITerminator() = default;
};

/**
 * @brief A jump terminator instruction.
 *
 * A jump instruction unconditionally transfers control to a single successor
 * basic block.
 *
 * Do not instantiate this class outside of `BasicBlock`. Use
 * `BasicBlock::set_successor()` to set up a jump instruction.
 */
class Instruction::Jump : public ITerminator {
    friend class BasicBlock;

public:
    // The target basic block to jump to.
    std::weak_ptr<BasicBlock> target;

    virtual ~Jump() = default;

protected:
    Jump(std::shared_ptr<BasicBlock> target)
        : target(target) {}
};

/**
 * @brief A branch terminator instruction.
 *
 * A branch instruction conditionally transfers control to one of two
 * successor basic blocks based on the value of a condition.
 *
 * When executed, if the condition evaluates to true, control is transferred to
 * the main target; otherwise, control is transferred to the alternative target.
 *
 * Do not instantiate this class outside of `BasicBlock`. Use
 * `BasicBlock::set_successors()` to set up a branch instruction.
 */
class Instruction::Branch : public ITerminator {
    friend class BasicBlock;

public:
    // The condition value for the branch.
    std::shared_ptr<Value> condition;
    // The main target basic block if the condition is true.
    std::weak_ptr<BasicBlock> main_target;
    // The alternative target basic block if the condition is false.
    std::weak_ptr<BasicBlock> alt_target;

    virtual ~Branch() = default;

protected:
    Branch(
        std::shared_ptr<Value> condition,
        std::shared_ptr<BasicBlock> main_target,
        std::shared_ptr<BasicBlock> alt_target
    )
        : condition(condition),
          main_target(main_target),
          alt_target(alt_target) {}
};

/**
 * @brief A return terminator instruction.
 *
 * A return instruction signifies the end of a function, returning control to
 * the caller.
 *
 * Do not instantiate this class outside of `BasicBlock` or `Function`.
 * `Function` objects built using `Function::create()` will have an exit block
 * with a return instruction already set up.
 *
 * When building MIR, use `Function::get_exit_block()` to get the exit block and
 * jump to it when returning from the function.
 */
class Instruction::Return : public ITerminator {
    friend class BasicBlock;

public:
    virtual ~Return() = default;

protected:
    Return() = default;
};

} // namespace nico

#endif // NICO_MIR_INSTRUCTIONS_H
