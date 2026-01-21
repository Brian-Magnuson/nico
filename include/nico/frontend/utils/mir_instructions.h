#ifndef NICO_MIR_INSTRUCTIONS_H
#define NICO_MIR_INSTRUCTIONS_H

#include <memory>
#include <string>

#include "nico/frontend/utils/mir.h"
#include "nico/frontend/utils/mir_values.h"
#include "nico/frontend/utils/type_node.h"

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
class Instr::INonTerm : public Instr {
public:
    virtual ~INonTerm() = default;
};

/**
 * @brief A binary instruction in the MIR.
 *
 * Binary instructions perform operations on two operands.
 */
class Instr::Binary : public INonTerm {
public:
    /**
     * @brief The operation performed by the binary instruction.
     */
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
    const std::shared_ptr<MIRValue::Temporary> destination;

    Binary(
        Op op,
        std::shared_ptr<MIRValue> left_operand,
        std::shared_ptr<MIRValue> right_operand,
        std::shared_ptr<Type> result_type
    )
        : op(op),
          left_operand(left_operand),
          right_operand(right_operand),
          destination(std::make_shared<MIRValue::Temporary>(result_type)) {}

    virtual ~Binary() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    /**
     * @brief Converts the operation to a string.
     *
     * E.g., if `this->op` is `Op::Add`, this function returns `"add"`.
     *
     * @return A string representation of the operation.
     */
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

/**
 * @brief A unary instruction in the MIR.
 *
 * Unary instructions perform operations on a single operand.
 */
class Instr::Unary : public INonTerm {
public:
    enum class Op {
        Neg
        // More operations can be added here.
    };

    // The operation of the unary instruction.
    const Op op;
    // The operand of the unary instruction.
    const std::shared_ptr<MIRValue> operand;
    // The destination where the result is stored.
    const std::shared_ptr<MIRValue::Temporary> destination;

    Unary(
        Op op,
        std::shared_ptr<MIRValue> operand,
        std::shared_ptr<Type> result_type
    )
        : op(op),
          operand(operand),
          destination(std::make_shared<MIRValue::Temporary>(result_type)) {}

    virtual ~Unary() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    /**
     * @brief Converts the operation to a string.
     *
     * E.g., if `this->op` is `Op::Neg`, this function returns `"neg"`.
     *
     * @return The string representation of the operation.
     */
    std::string op_to_string() const {
        switch (op) {
        case Op::Neg:
            return "neg";
        }
    }

    virtual std::string to_string() const override {
        return op_to_string() + operand->to_string() + " -> " +
               destination->to_string();
    }
};

/**
 * @brief A call instruction in the MIR.
 *
 * The call instruction represents a function call in the MIR.
 *
 * It includes the target function to call, the arguments to pass to the
 * function, and the destination where the return value is stored, if any.
 */
class Instr::Call : public INonTerm {
public:
    // The target function to call.
    const std::weak_ptr<Function> target_function;
    // The arguments to pass to the function.
    const std::vector<std::shared_ptr<MIRValue>> arguments;
    // The destination where the return value is stored, if any.
    const std::shared_ptr<MIRValue::Temporary> destination;

    Call(
        std::shared_ptr<Function> target_function,
        std::vector<std::shared_ptr<MIRValue>> arguments
    )
        : target_function(target_function),
          arguments(arguments),
          destination(
              std::make_shared<MIRValue::Temporary>(
                  target_function->get_return_type()
              )
          ) {}

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
 * @brief An alloca instruction in the MIR.
 *
 * The alloca instruction allocates memory on the stack for a variable of a
 * specified type.
 *
 * The allocated memory is associated with a destination MIR value, which can
 * be used to reference the allocated memory in subsequent instructions.
 */
class Instr::Alloca : public INonTerm {
public:
    // The destination where the allocated value is stored.
    const std::shared_ptr<MIRValue::Variable> variable;
    // The type of the allocated value.
    std::shared_ptr<Type> allocated_type;

    Alloca(
        std::shared_ptr<MIRValue::Variable> variable,
        std::shared_ptr<Type> allocated_type
    )
        : variable(variable), allocated_type(allocated_type) {}

    virtual ~Alloca() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        return "alloca " + allocated_type->to_string() + " " +
               variable->to_string();
    }
};

/**
 * @brief A store instruction in the MIR.
 *
 * The store instruction copies a value from a source MIR value to a
 * destination variable MIR value.
 */
class Instr::Store : public INonTerm {
public:
    // The source value to copy from.
    const std::shared_ptr<MIRValue> source;
    // The destination value to copy to.
    const std::shared_ptr<MIRValue> destination;

    Store(
        std::shared_ptr<MIRValue> source, std::shared_ptr<MIRValue> destination
    )
        : source(source), destination(destination) {
        // Assert that the destination is a pointer type.
        if (!PTR_INSTANCEOF(destination->type, Type::IPointer)) {
            panic(
                "Instr::Store::Store: Destination must be a pointer type. "
                "Got `" +
                destination->type->to_string() + "`."
            );
        }
    }

    virtual ~Store() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        return "store " + source->to_string() + " -> " +
               destination->to_string();
    }
};

/**
 * @brief A load instruction in the MIR.
 *
 * The load instruction reads a value from a source MIR value (which must be a
 * pointer) and stores it in a destination temporary MIR value.
 */
class Instr::Load : public INonTerm {
public:
    // The source value to load from.
    const std::shared_ptr<MIRValue> source;
    // The destination where the loaded value is stored.
    const std::shared_ptr<MIRValue::Temporary> destination;

    Load(std::shared_ptr<MIRValue> source, std::shared_ptr<Type> result_type)
        : source(source),
          destination(std::make_shared<MIRValue::Temporary>(result_type)) {
        // Assert that the source is a pointer type.
        if (!PTR_INSTANCEOF(source->type, Type::IPointer)) {
            panic(
                "Instr::Load::Load: Source must be a pointer type. Got `" +
                source->type->to_string() + "`."
            );
        }
    }

    virtual ~Load() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        return "load " + source->to_string() + " -> " +
               destination->to_string();
    }
};

/**
 * @brief A phi instruction in the MIR.
 *
 * The phi instruction selects a value based on the predecessor basic block
 * from which control arrived.
 *
 * This is used in SSA form to merge values coming from different control flow
 * paths.
 */
class Instr::Phi : public INonTerm {
public:
    // The temporary where the result is stored.
    const std::shared_ptr<MIRValue::Temporary> destination;
    // A map of predecessor basic blocks to their corresponding values.
    const std::
        unordered_map<std::shared_ptr<BasicBlock>, std::shared_ptr<MIRValue>>
            incoming_values;

    Phi(std::shared_ptr<Type> result_type,
        std::unordered_map<
            std::shared_ptr<BasicBlock>,
            std::shared_ptr<MIRValue>> incoming_values)
        : destination(std::make_shared<MIRValue::Temporary>(result_type)),
          incoming_values(incoming_values) {}

    virtual ~Phi() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        std::string result = "phi ";
        for (const auto& [block, value] : incoming_values) {
            result +=
                "[" + block->get_name() + ": " + value->to_string() + "] ";
        }
        result += "-> " + destination->to_string();
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
class Instr::ITerm : public Instr {
public:
    ITerm() = default;
    virtual ~ITerm() = default;
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
class Instr::Jump : public ITerm {
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
class Instr::Branch : public ITerm {
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
class Instr::Return : public ITerm {
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
