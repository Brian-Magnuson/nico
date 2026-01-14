#ifndef NICO_MIR_INSTRUCTIONS_H
#define NICO_MIR_INSTRUCTIONS_H

#include <memory>
#include <string>

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
public:
    enum class Op {
        Add,
        Sub,
        Mul,
        Div
        // More operations can be added here.
    };

    // The operation of the binary instruction.
    const Op op;
    // The left operand of the binary instruction.
    const std::shared_ptr<MIRValue> left_operand;
    // The right operand of the binary instruction.
    const std::shared_ptr<MIRValue> right_operand;
    // The destination where the result is stored.
    const std::shared_ptr<MIRValue> destination;

    Binary(
        Op op,
        std::shared_ptr<MIRValue> left_operand,
        std::shared_ptr<MIRValue> right_operand,
        std::shared_ptr<MIRValue> destination
    )
        : op(op),
          left_operand(left_operand),
          right_operand(right_operand),
          destination(destination) {}

    virtual ~Binary() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    std::string op_to_string() const {
        switch (op) {
        case Op::Add:
            return "add";
        case Op::Sub:
            return "sub";
        case Op::Mul:
            return "mul";
        case Op::Div:
            return "div";
        }
    }

    virtual std::string to_string() const override {
        return op_to_string() + left_operand->to_string() +
               right_operand->to_string() + " -> " + destination->to_string();
    }
};

class Instruction::Unary : public INonTerminator {
public:
    enum class Op {
        Copy,
        Neg
        // More operations can be added here.
    };

    // The operation of the unary instruction.
    const Op op;
    // The operand of the unary instruction.
    const std::shared_ptr<MIRValue> operand;
    // The destination where the result is stored.
    const std::shared_ptr<MIRValue> destination;

    Unary(
        Op op,
        std::shared_ptr<MIRValue> operand,
        std::shared_ptr<MIRValue> destination
    )
        : op(op), operand(operand), destination(destination) {}

    virtual ~Unary() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    std::string op_to_string() const {
        switch (op) {
        case Op::Copy:
            return "copy";
        case Op::Neg:
            return "neg";
        }
    }

    virtual std::string to_string() const override {
        return op_to_string() + operand->to_string() + " -> " +
               destination->to_string();
    }
};

class Instruction::Call : public INonTerminator {
public:
    // The target function to call.
    const std::weak_ptr<Function> target_function;
    // The arguments to pass to the function.
    const std::vector<std::shared_ptr<MIRValue>> arguments;
    // The destination where the return value is stored, if any.
    const std::shared_ptr<MIRValue> destination;

    Call(
        std::shared_ptr<Function> target_function,
        std::vector<std::shared_ptr<MIRValue>> arguments,
        std::shared_ptr<MIRValue> destination
    )
        : target_function(target_function),
          arguments(arguments),
          destination(destination) {}

    virtual ~Call() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        std::string result =
            "call " + target_function.lock()->get_name() + "( ";
        for (const auto& mir_val : arguments) {
            result += mir_val->to_string() + " ";
        }
        result += ") -> " + destination->to_string();
        return result;
    }
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
public:
    ITerminator() = default;
    virtual ~ITerminator() = default;
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
public:
    // The target basic block to jump to.
    const std::weak_ptr<BasicBlock> target;

    Jump(std::shared_ptr<BasicBlock> target)
        : target(target) {}

    virtual ~Jump() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        return "jump " + target.lock()->get_name();
    }
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
public:
    // The condition value for the branch.
    const std::shared_ptr<MIRValue> condition;
    // The main target basic block if the condition is true.
    const std::weak_ptr<BasicBlock> main_target;
    // The alternative target basic block if the condition is false.
    const std::weak_ptr<BasicBlock> alt_target;

    Branch(
        std::shared_ptr<MIRValue> condition,
        std::shared_ptr<BasicBlock> main_target,
        std::shared_ptr<BasicBlock> alt_target
    )
        : condition(condition),
          main_target(main_target),
          alt_target(alt_target) {}

    virtual ~Branch() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        return "branch " + condition->to_string() + " ? " +
               main_target.lock()->get_name() + " : " +
               alt_target.lock()->get_name();
    }
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
public:
    Return() = default;

    virtual ~Return() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override { return "return"; }
};

} // namespace nico

#endif // NICO_MIR_INSTRUCTIONS_H
