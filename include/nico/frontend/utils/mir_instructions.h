#ifndef NICO_MIR_INSTRUCTIONS_H
#define NICO_MIR_INSTRUCTIONS_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "nico/frontend/utils/ast_node.h"
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
    // The operation of the binary instruction.
    const Expr::Binary::Operation op;
    // The left operand of the binary instruction.
    const std::shared_ptr<MIRValue> left_operand;
    // The right operand of the binary instruction.
    const std::shared_ptr<MIRValue> right_operand;
    // The destination where the result is stored.
    const std::shared_ptr<MIRValue::Temporary> destination;

    Binary(
        Expr::Binary::Operation op,
        std::shared_ptr<MIRValue> left_operand,
        std::shared_ptr<MIRValue> right_operand,
        std::shared_ptr<Type> result_type
    )
        : op(op),
          left_operand(left_operand),
          right_operand(right_operand),
          destination(MIRValue::Temporary::create(result_type)) {}

    virtual ~Binary() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        return "binary " + Expr::Binary::operation_to_string(op) + " " +
               left_operand->to_string() + " " + right_operand->to_string() +
               " -> " + destination->to_string();
    }
};

/**
 * @brief A unary instruction in the MIR.
 *
 * Unary instructions perform operations on a single operand.
 */
class Instr::Unary : public INonTerm {
public:
    // The operation of the unary instruction.
    const Expr::Unary::Operation op;
    // The operand of the unary instruction.
    const std::shared_ptr<MIRValue> operand;
    // The destination where the result is stored.
    const std::shared_ptr<MIRValue::Temporary> destination;

    Unary(
        Expr::Unary::Operation op,
        std::shared_ptr<MIRValue> operand,
        std::shared_ptr<Type> result_type
    )
        : op(op),
          operand(operand),
          destination(MIRValue::Temporary::create(result_type)) {}

    virtual ~Unary() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        return "unary " + Expr::Unary::operation_to_string(op) + " " +
               operand->to_string() + " -> " + destination->to_string();
    }
};

/**
 * @brief A cast instruction in the MIR.
 *
 * Cast instructions perform type casting operations on a single operand.
 */
class Instr::Cast : public INonTerm {
public:
    // The operation of the cast instruction.
    const Expr::Cast::Operation op;
    // The operand of the cast instruction.
    const std::shared_ptr<MIRValue> operand;
    // The destination where the result is stored.
    const std::shared_ptr<MIRValue::Temporary> destination;

    Cast(
        Expr::Cast::Operation op,
        std::shared_ptr<MIRValue> operand,
        std::shared_ptr<Type> result_type
    )
        : op(op),
          operand(operand),
          destination(MIRValue::Temporary::create(result_type)) {}

    virtual ~Cast() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        return "cast " + Expr::Cast::operation_to_string(op) + " " +
               operand->to_string() + " -> " + destination->to_string();
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
              MIRValue::Temporary::create(target_function->get_return_type())
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
 * @brief A size-of instruction in the MIR.
 *
 * The size-of instruction computes the size of a type in bytes and stores it in
 * a temporary.
 *
 * This can actually be calculated in the code generator, not requiring an
 * instruction. But we include it in the MIR to keep its implementation less
 * dependent on LLVM.
 */
class Instr::SizeOf : public INonTerm {
public:
    // The type for which to compute the size.
    const std::shared_ptr<Type> target_type;
    // The destination where the size in bytes is stored.
    const std::shared_ptr<MIRValue::Temporary> destination;

    SizeOf(std::shared_ptr<Type> target_type)
        : target_type(target_type),
          destination(
              MIRValue::Temporary::create(
                  std::make_shared<Type::Int>(false, 64)
              )
          ) {}

    virtual ~SizeOf() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        return "sizeof " + target_type->to_string() + " -> " +
               destination->to_string();
    }
};

/**
 * @brief A heap allocation instruction in the MIR.
 *
 * Heap allocation instructions allocate memory on the heap for a specified
 * size in bytes and yields a pointer to the allocated memory.
 */
class Instr::HeapAlloc : public INonTerm {
public:
    // The value indicating the amount of memory to allocate in bytes.
    const std::shared_ptr<MIRValue> size_bytes;
    // The destination where the allocated memory pointer is stored.
    const std::shared_ptr<MIRValue::Temporary> destination;

    HeapAlloc(std::shared_ptr<MIRValue> size_bytes)
        : size_bytes(size_bytes),
          destination(
              MIRValue::Temporary::create(std::make_shared<Type::Anyptr>())
          ) {}

    virtual ~HeapAlloc() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        return "heapalloc " + size_bytes->to_string() + " -> " +
               destination->to_string();
    }
};

/**
 * @brief A heap free instruction in the MIR.
 *
 * The heap free instruction deallocates memory on the heap that was previously
 * allocated by a heap allocation instruction.
 */
class Instr::HeapFree : public INonTerm {
public:
    // The value indicating the pointer to the memory to free.
    const std::shared_ptr<MIRValue> pointer;

    HeapFree(std::shared_ptr<MIRValue> pointer)
        : pointer(pointer) {}

    virtual ~HeapFree() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        return "heapfree " + pointer->to_string();
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
        if (!Type::is_a<Type::IPointer>(destination->type)) {
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
          destination(MIRValue::Temporary::create(result_type)) {
        // Assert that the source is a pointer type.
        if (!Type::is_a<Type::IPointer>(source->type)) {
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
    const std::vector<
        std::pair<std::shared_ptr<BasicBlock>, std::shared_ptr<MIRValue>>>
        incoming_values;

    Phi(std::shared_ptr<Type> result_type,
        std::vector<
            std::pair<std::shared_ptr<BasicBlock>, std::shared_ptr<MIRValue>>>&&
            incoming_values)
        : destination(MIRValue::Temporary::create(result_type)),
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
 * @brief An instruction that creates an array in the MIR.
 *
 * In LLVM, the process for creating an array is more complex, but here, we
 * simplify it to a single instruction for the sake of the MIR.
 */
class Instr::Array : public INonTerm {
public:
    // The destination where the array is stored.
    const std::shared_ptr<MIRValue::Temporary> destination;
    // The elements of the array.
    const std::vector<std::shared_ptr<MIRValue>> elements;
    // Whether this array could be created as a constant expression.
    bool is_constexpr;

    Array(
        std::shared_ptr<Type> result_type,
        std::vector<std::shared_ptr<MIRValue>> elements,
        bool is_constexpr
    )
        : destination(MIRValue::Temporary::create(result_type)),
          elements(elements),
          is_constexpr(is_constexpr) {}

    virtual ~Array() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        std::string result = "array [ ";
        for (const auto& element : elements) {
            result += element->to_string() + " ";
        }
        result += "] -> " + destination->to_string();
        return result;
    }
};

/**
 * @brief An instruction that creates a struct in the MIR.
 *
 * In LLVM, the process for creating a struct is more complex, but here, we
 * simplify it to a single instruction for the sake of the MIR.
 */
class Instr::Struct : public INonTerm {
public:
    // The destination where the struct is stored.
    const std::shared_ptr<MIRValue::Temporary> destination;
    // The fields of the struct.
    const std::vector<std::shared_ptr<MIRValue>> fields;
    // Whether this struct could be created as a constant expression.
    bool is_constexpr;

    Struct(
        std::shared_ptr<Type> result_type,
        std::vector<std::shared_ptr<MIRValue>> fields,
        bool is_constexpr
    )
        : destination(MIRValue::Temporary::create(result_type)),
          fields(fields),
          is_constexpr(is_constexpr) {}

    virtual ~Struct() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        std::string result = "struct { ";
        for (const auto& field : fields) {
            result += field->to_string() + " ";
        }
        result += "} -> " + destination->to_string();
        return result;
    }
};

/**
 * @brief A printout instruction in the MIR.
 *
 * This high-level instruction is made to be one-to-one with the `printout`
 * statement in the source code.
 * Lowering this instruction to LLVM IR may require a call instruction that
 * handles the actual printing.
 */
class Instr::Printout : public INonTerm {
public:
    // The values to print out.
    const std::vector<std::shared_ptr<MIRValue>> values;

    Printout(std::vector<std::shared_ptr<MIRValue>> values)
        : values(values) {}

    virtual ~Printout() = default;

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    virtual std::string to_string() const override {
        std::string result = "printout";
        for (const auto& value : values) {
            result += " " + value->to_string();
        }
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
