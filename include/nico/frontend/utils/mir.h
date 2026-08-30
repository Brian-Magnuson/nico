#ifndef NICO_MIR_H
#define NICO_MIR_H

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/nodes.h"
#include "nico/frontend/utils/symbol_node.h"
#include "nico/shared/dictionary.h"

namespace nico {

/**
 * @brief Represents a value in the MIR.
 *
 * A value can be a literal, variable, or temporary.
 *
 * Only members of this class and its subclasses may be used with instructions.
 */
class MIRValue : public std::enable_shared_from_this<MIRValue> {
    // A static map to keep track of temporary name counters for unique naming.
    static std::unordered_map<std::string, size_t> mir_temp_name_counters;

protected:
    /**
     * @brief A private struct used to restrict access to constructors.
     */
    struct Private {
        explicit Private() = default;
    };

public:
    class IConstant;
    class ZeroValue;
    class CustomInt;
    class Literal;
    class Array;
    class Struct;

    class Variable;
    class Temporary;
    class Global;

    virtual ~MIRValue() = default;

    /**
     * @brief A visitor class for values.
     */
    class Visitor {
    public:
        virtual std::any visit(ZeroValue* value) = 0;
        virtual std::any visit(CustomInt* value) = 0;
        virtual std::any visit(Literal* value) = 0;
        virtual std::any visit(Array* value) = 0;
        virtual std::any visit(Struct* value) = 0;
        virtual std::any visit(Variable* value) = 0;
        virtual std::any visit(Temporary* value) = 0;
        virtual std::any visit(Global* value) = 0;
    };

    // The type of this value.
    std::shared_ptr<Type> type;

    MIRValue(Private, std::shared_ptr<Type> type)
        : type(type) {}

    static void reset_counters() { mir_temp_name_counters.clear(); }

    /**
     * @brief Converts this value to a string.
     *
     * The value should not contain any newlines.
     *
     * Useful for debugging purposes.
     *
     * @return A string representation of the value.
     */
    virtual std::string to_string() const = 0;

    /**
     * @brief Accept a visitor.
     *
     * @param visitor The visitor to accept.
     * @return The return value from the visitor.
     */
    virtual std::any accept(Visitor* visitor) = 0;
};

/**
 * @brief Represents an instruction in the MIR.
 *
 * Instructions fall into two categories, each with its own interface:
 * - Non-terminator instructions (`INonTerm`): Instructions that do not
 *   terminate a basic block, e.g., arithmetic operations, function calls.
 * - Terminator instructions (`ITerm`): Instructions that terminate a
 *   basic block, e.g., jumps, branches, returns.
 */
class Instr {
public:
    class INonTerm;
    class Binary;
    class Unary;
    class Cast;
    class Call;
    class Local;
    class SizeOf;
    class Alloc;
    class Dealloc;
    class Store;
    class Load;
    class Phi;
    class Array;
    class Struct;
    class Printout;
    class Panic;

    class ITerm;
    class Jump;
    class Branch;
    class Return;

    virtual ~Instr() = default;

    /**
     * @brief A visitor class for instructions.
     */
    class Visitor {
    public:
        virtual std::any visit(Binary* instr) = 0;
        virtual std::any visit(Unary* instr) = 0;
        virtual std::any visit(Cast* instr) = 0;
        virtual std::any visit(Call* instr) = 0;
        virtual std::any visit(Local* instr) = 0;
        virtual std::any visit(SizeOf* instr) = 0;
        virtual std::any visit(Alloc* instr) = 0;
        virtual std::any visit(Dealloc* instr) = 0;
        virtual std::any visit(Store* instr) = 0;
        virtual std::any visit(Load* instr) = 0;
        virtual std::any visit(Phi* instr) = 0;
        virtual std::any visit(Array* instr) = 0;
        virtual std::any visit(Struct* instr) = 0;
        virtual std::any visit(Printout* instr) = 0;
        virtual std::any visit(Panic* instr) = 0;
        virtual std::any visit(Jump* instr) = 0;
        virtual std::any visit(Branch* instr) = 0;
        virtual std::any visit(Return* instr) = 0;
    };

    /**
     * @brief Converts this instruction to a string.
     *
     * The instruction should not contain any newlines.
     *
     * Useful for debugging purposes.
     *
     * @return A string representation of the instruction.
     */
    virtual std::string to_string() const = 0;

    /**
     * @brief Accept a visitor.
     *
     * @param visitor The visitor to accept.
     * @return The return value from the visitor.
     */
    virtual std::any accept(Visitor* visitor) = 0;
};

class Function;

/**
 * @brief Represents a basic block in the MIR.
 *
 * A basic block is a sequence of instructions that execute sequentially and end
 * with a terminator instruction.
 *
 * Basic blocks have predecessors and successors (accessed through the
 * terminator instruction) that, together, form the control flow graph of a
 * function. Each basic block is a vertex in this graph.
 *
 * It should not be confused with a block expression, which defines a lexical
 * scope.
 */
class BasicBlock : public std::enable_shared_from_this<BasicBlock> {
    friend class Function;

    // Empty private struct to restrict access to certain methods.
    struct Private {
        explicit Private() = default;
    };

    // A map to keep track of basic block name counters for unique naming.
    static std::unordered_map<std::string, size_t> bb_name_counters;

    // The name of the basic block.
    const std::string name;
    // The instructions in the basic block.
    std::vector<std::shared_ptr<Instr::INonTerm>> instructions;
    // The terminator instruction of the basic block.
    std::shared_ptr<Instr::ITerm> terminator;
    // The parent function of the basic block.
    std::weak_ptr<Function> parent_function;

    // This block's predecessors in the control flow graph.
    std::vector<std::weak_ptr<BasicBlock>> predecessors;

protected:
    /**
     * @brief Sets this block to use a return terminator.
     *
     * Only the Function class is allowed to call this method since only the
     * exit block may be set as the return block.
     *
     * @warning If the terminator instruction is already set, this method will
     * panic.
     */
    void set_as_function_return();

public:
    /**
     * @brief Resets the basic block name counters.
     *
     */
    static void reset_counters() { bb_name_counters.clear(); }

    /**
     * @brief Constructs a new BasicBlock with the given name.
     *
     * This constructor is intended to be called only by the Function class.
     * This is because the Function class is responsible for managing the
     * lifetimes of the basic blocks.
     *
     * @param private Unused, but required to verify that you can call this
     * function here.
     * @param name The name of the basic block.
     */
    BasicBlock(Private, std::string_view name);

    /**
     * @brief Get the name of the basic block.
     *
     * @return The name of the basic block.
     */
    std::string get_name() const { return name; }

    /**
     * @brief Get the parent function of the basic block.
     *
     * @return The parent function of the basic block.
     */
    std::shared_ptr<Function> get_parent_function() const {
        return parent_function.lock();
    }

    /**
     * @brief Get the non-terminator instructions in the basic block.
     *
     * @return The non-terminator instructions in the basic block.
     */
    const std::vector<std::shared_ptr<Instr::INonTerm>>&
    get_instructions() const {
        return instructions;
    }

    /**
     * @brief Adds a non-terminator instruction to the basic block.
     *
     * Only non-terminator instructions can be added with this method.
     *
     * @param instruction The non-terminator instruction to add.
     */
    void add_instruction(std::shared_ptr<Instr::INonTerm> instruction);

    /**
     * @brief Sets this block to use a jump terminator to the given successor.
     *
     * @param successor The successor basic block.
     *
     * @warning If the terminator instruction is already set, this method will
     * panic.
     */
    void set_successor(std::shared_ptr<BasicBlock> successor);

    /**
     * @brief Sets this block to use a branch terminator with the given
     * condition and successors.
     *
     * @param condition The condition value for the branch.
     * @param main_successor The main successor basic block.
     * @param alt_successor The alternative successor basic block.
     *
     * @warning If the terminator instruction is already set, this method will
     * panic.
     */
    void set_successors(
        std::shared_ptr<MIRValue> condition,
        std::shared_ptr<BasicBlock> main_successor,
        std::shared_ptr<BasicBlock> alt_successor
    );

    /**
     * @brief Retrieves the successors of this basic block.
     *
     * A basic block can have 0, 1, or 2 successors depending on its terminator
     * instruction.
     *
     * This function helps abstract away the process of checking the type of the
     * terminator instruction and converting the weak pointers to shared
     * pointers.
     *
     * @return A vector of shared pointers to the successor basic blocks.
     */
    std::vector<std::shared_ptr<BasicBlock>> get_successors() const;

    /**
     * @brief Checks if this basic block has any living predecessors.
     *
     * A living predecessor is a predecessor that has not been destroyed.
     *
     * @return True if this basic block has at least one living predecessor,
     * false otherwise.
     */
    bool has_living_predecessors() const {
        for (const auto& pred_weak : predecessors) {
            if (!pred_weak.expired()) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Converts this basic block to a string.
     *
     * The string will include multiple lines and will end with a newline.
     *
     * Note: The string representation includes the entire contents of the basic
     * block, including all instructions and the terminator.
     *
     * For just the name of the basic block, use `get_name()`.
     *
     * @return A string representation of the basic block.
     */
    std::string to_string() const;
};

/**
 * @brief Represents a function in the MIR.
 *
 * A function consists of a series of basic blocks forming a control flow graph.
 *
 * All functions start with the same basic structure: an entry block with an
 * unset terminator, and an exit block that returns from the function. MIR
 * building should start from the entry block, filling in its terminator
 * instruction at some point. When returning from the function, control should
 * jump to the exit block, and should not return directly.
 */
class Function : public std::enable_shared_from_this<Function> {
    friend class MIRModule;

    // Empty private struct to restrict access to certain methods.
    struct Private {
        explicit Private() = default;
    };

    // TODO: Inner classes are good, but putting the whole implementation here
    // is a bit messy. Consider moving them out.

    struct ControlLoop;

    /**
     * @brief A control block in the function's control stack.
     *
     * Control blocks are used to track yield variables and block labels,
     * allowing yield statements to properly target the correct block and
     * variable.
     */
    struct ControlBlock : public std::enable_shared_from_this<ControlBlock> {
        // A pointer to the previous control block in the stack, if any.
        std::optional<std::shared_ptr<ControlBlock>> prev;
        // The yield variable associated with this control block.
        std::shared_ptr<MIRValue::Variable> yield_variable;
        // The label associated with this control block, if any.
        std::optional<std::string> label;

        ControlBlock(
            std::optional<std::shared_ptr<ControlBlock>> prev,
            std::shared_ptr<MIRValue::Variable> yield_variable,
            std::optional<std::string> label = std::nullopt
        )
            : prev(prev), yield_variable(yield_variable), label(label) {}

        virtual ~ControlBlock() = default;

        /**
         * @brief Get the block object with the specified label, or the top
         * block if no label is provided.
         *
         * This function does not distinguish between plain blocks and loop
         * blocks. It will return the first block in the control stack that
         * matches the label, regardless of its type.
         *
         * @param label (Optional) The label of the block to find. If not
         * provided, the top block is returned.
         * @return std::optional<std::shared_ptr<ControlBlock>> The block object
         * with the specified label, or std::nullopt if no such block exists.
         */
        virtual std::optional<std::shared_ptr<ControlBlock>>
        get_block(std::optional<std::string> label = std::nullopt) {
            if (!label || this->label == label) {
                return shared_from_this();
            }
            else if (auto prev_block = prev.value_or(nullptr)) {
                return prev_block->get_block(label);
            }
            else {
                return std::nullopt;
            }
        }

        /**
         * @brief Get the loop object with the specified label, or the top loop
         * if no label is provided.
         *
         * @param label (Optional) The label of the loop to find. If not
         * provided, the top loop is returned.
         * @return std::optional<std::shared_ptr<ControlLoop>> The loop object
         * with the specified label, or std::nullopt if no such loop exists.
         */
        virtual std::optional<std::shared_ptr<ControlLoop>>
        get_loop(std::optional<std::string> label = std::nullopt) {
            if (auto prev_block = prev.value_or(nullptr)) {
                return prev_block->get_loop(label);
            }
            else {
                return std::nullopt;
            }
        }
    };

    /**
     * @brief A control loop block in the function's control stack.
     *
     * Control loop blocks are used to track the continue and exit points of
     * loops, allowing `continue` and `break` statements to properly target the
     * correct loop.
     */
    struct ControlLoop : public ControlBlock {
        // The continue block associated with this loop.
        std::weak_ptr<BasicBlock> continue_block;
        // The exit block associated with this loop.
        std::weak_ptr<BasicBlock> exit_block;

        ControlLoop(
            std::optional<std::shared_ptr<ControlBlock>> prev,
            std::shared_ptr<MIRValue::Variable> yield_variable,
            std::weak_ptr<BasicBlock> continue_block,
            std::weak_ptr<BasicBlock> exit_block,
            std::optional<std::string> label = std::nullopt
        )
            : ControlBlock(prev, yield_variable, label),
              continue_block(continue_block),
              exit_block(exit_block) {}

        std::optional<std::shared_ptr<ControlLoop>>
        get_loop(std::optional<std::string> label = std::nullopt) override {
            if (!label || this->label == label) {
                return std::dynamic_pointer_cast<ControlLoop>(
                    shared_from_this()
                );
            }
            else if (auto prev_block = prev.value_or(nullptr)) {
                return prev_block->get_loop(label);
            }
            else {
                return std::nullopt;
            }
        }
    };

    // The name of the function.
    std::string name;
    // The return type of the function.
    std::shared_ptr<Type> return_type;
    // The parameters of the function.
    std::vector<std::shared_ptr<MIRValue::Variable>> parameters;
    // A special temporary value for the return value.
    std::shared_ptr<MIRValue::Variable> return_variable;
    // The local variables declared in this function.
    Dictionary<std::string, std::shared_ptr<MIRValue::Variable>> locals;
    // The entry basic block of the function.
    std::shared_ptr<BasicBlock> entry_block;
    // The basic blocks in the function.
    std::vector<std::shared_ptr<BasicBlock>> basic_blocks;
    // The exit block of the function.
    std::shared_ptr<BasicBlock> exit_block;
    // The control stack of the function, used to track yield variables and
    // block labels.
    std::optional<std::shared_ptr<ControlBlock>> control_stack = std::nullopt;

protected:
    /**
     * @brief Creates a new function using the provided function statement.
     *
     * The function will have an entry and exit basic block created
     * automatically.
     *
     * The entry block will start without a terminator instruction.
     * During MIR building, the terminator instruction must be filled in at some
     * point.
     *
     * @param func_stmt The statement from which this function was created.
     * @return The newly created function.
     */
    static std::shared_ptr<Function>
    create(std::shared_ptr<Stmt::Func> func_stmt);

    /**
     * @brief Creates the script function.
     *
     * The script function is a special implicit function that contains the
     * top-level statements in the source code.
     *
     * For executables, this function is called by the `main` function.
     *
     * @return The newly created script function.
     */
    static std::shared_ptr<Function> create_script_function();

public:
    /**
     * @brief Constructs an empty Function.
     *
     * Do not call this constructor directly; create a function from the
     * MIRModule instead.
     *
     * @param private Unused, but required to verify that you can call this
     * function here.
     */
    Function(Private) {}

    /**
     * @brief Get the name of the function.
     *
     * @return The name of the function.
     */
    std::string get_name() const { return name; }

    /**
     * @brief Get the return type of the function.
     *
     * @return The return type of the function.
     */
    std::shared_ptr<Type> get_return_type() const;

    /**
     * @brief Create a local variable object from the given binding entry and
     * add it to the function's locals.
     *
     * The produced local variable must still be added in a local instruction in
     * a basic block.
     *
     * @param binding_entry The binding entry for which to create a local
     * variable.
     * @return std::shared_ptr<MIRValue::Variable> The newly created local
     * variable.
     */
    std::shared_ptr<MIRValue::Variable>
    create_local_variable(std::shared_ptr<Node::BindingEntry> binding_entry);

    /**
     * @brief Get the local variable object corresponding to the given binding
     * entry.
     *
     * If the local variable does not exist in the function's locals, this
     * method will panic.
     *
     * @param binding_entry The binding entry for which to get the local
     * variable.
     * @return std::shared_ptr<MIRValue::Variable> The local variable
     * corresponding to the binding entry.
     *
     * @warning This method will panic if the local variable does not exist in
     * the function's locals. Ensure that the binding entry is valid and has
     * been declared before calling this method.
     */
    std::shared_ptr<MIRValue::Variable>
    get_local_variable(std::shared_ptr<Node::BindingEntry> binding_entry);

    /**
     * @brief Get the return variable of the function.
     *
     * @return The return variable of the function.
     */
    std::shared_ptr<MIRValue::Variable> get_return_variable() const {
        return return_variable;
    }

    /**
     * @brief Get the yield variable of the specified block kind and label.
     *
     * The yield variable is the variable targeted by yield, break, and return
     * statements. Which yield variable is targeted depends on the kind of block
     * and the label.
     *
     * If kind is set to `Function`, the function's return variable is returned,
     * effectively the same as calling `get_return_variable()`.
     *
     * @param kind The kind of block for which to get the yield variable.
     * @param label (Optional) An optional label to identify the block.
     * @return std::shared_ptr<MIRValue::Variable> The yield variable of the
     * specified block kind and label.
     *
     * @warning This method will panic if the requested block is not found. The
     * MIR builder should be designed to ensure that the requested block always
     * exists when this method is called.
     */
    std::shared_ptr<MIRValue::Variable> get_yield_variable(
        Expr::Block::Kind kind, std::optional<std::string> label = std::nullopt
    ) const;

    /**
     * @brief Creates a new basic block and adds it to the function.
     *
     * @param bb_name The name of the basic block.
     * @return The newly created basic block.
     */
    std::shared_ptr<BasicBlock> create_basic_block(std::string_view bb_name);

    /**
     * @brief Adds a plain control block to this function's internal control
     * stack.
     *
     * This allows the function to track yield variables and labels for plain
     * blocks, allowing yield statements to target the correct yield variable.
     * This should be called when block expressions are created.
     *
     * @param yield_variable The yield variable for the block.
     * @param label (Optional) An optional label to identify the block.
     */
    void add_plain_control_block(
        std::shared_ptr<MIRValue::Variable> yield_variable,
        const std::optional<std::string> label = std::nullopt
    );

    /**
     * @brief Adds a loop control block to this function's internal control
     * stack.
     *
     * This allows the function to track yield variables, continue blocks, exit
     * blocks, and labels for loop blocks, allowing yield, break, and continue
     * statements to target the correct blocks. This should be called when loop
     * expressions are created.
     *
     * @param yield_variable The yield variable for the loop block.
     * @param continue_block The continue block for the loop block.
     * @param exit_block The exit block for the loop block.
     * @param label (Optional) An optional label to identify the loop block.
     */
    void add_loop_control_block(
        std::weak_ptr<MIRValue::Variable> yield_variable,
        std::weak_ptr<BasicBlock> continue_block,
        std::weak_ptr<BasicBlock> exit_block,
        const std::optional<std::string> label = std::nullopt
    );

    /**
     * @brief Removes the top control block from this function's internal
     * control stack.
     *
     * This should be called when reaching the end of a block or loop
     * expression.
     *
     * @warning This method will panic if the control stack is empty.
     */
    void pop_control_block();

    /**
     * @brief Get the entry basic block of the function.
     *
     * The entry block is always the first basic block.
     *
     * @return The entry basic block.
     */
    std::shared_ptr<BasicBlock> get_entry_block() { return entry_block; }

    /**
     * @brief Get the loop continue block object.
     *
     * A loop continue block is the block that is jumped to when a `continue`
     * statement is executed. It is used to skip the rest of the current
     * iteration of the loop and proceed to the next iteration.
     *
     * @param label (Optional) An optional label to identify the loop block to
     * target.
     * @return std::shared_ptr<BasicBlock> The continue basic block of the
     * specified loop block.
     *
     * @warning This method will panic if the requested loop block is not found.
     */
    std::shared_ptr<BasicBlock>
    get_loop_continue_block(std::optional<std::string> label = std::nullopt);

    /**
     * @brief Get the exit basic block of the specified block kind and label.
     *
     * An exit block is always created when the function is created.
     *
     * @param kind The kind of block for which to get the exit block. Defaults
     * to `Function`.
     * @param label An optional label to identify the block.
     * @return The exit basic block.
     *
     * @warning This method will panic if the requested block is not found or if
     * kind is set to Plain.
     */
    std::shared_ptr<BasicBlock> get_exit_block(
        Expr::Block::Kind kind = Expr::Block::Kind::Function,
        std::optional<std::string> label = std::nullopt
    );

    /**
     * @brief Removes all basic blocks that are not reachable from the entry
     * block.
     *
     * Useful for dead code elimination and further CFG analysis.
     */
    void purge_unreachable_blocks();

    /**
     * @brief Converts this function to a string.
     *
     * The string representation includes multiple lines and ends with a
     * newline.
     *
     * Note: The string representation includes the entire contents of the
     * function, including all basic blocks and their instructions.
     *
     * For just the name of the function, use `get_name()`.
     *
     * @return A string representation of the function.
     */
    std::string to_string() const;
};

/**
 * @brief Represents a MIR module containing functions.
 */
class MIRModule {
    // Empty private struct to restrict access to certain methods.
    struct Private {
        explicit Private() = default;
    };

    // The global variables declared in this module.
    Dictionary<std::string, std::shared_ptr<MIRValue::Global>> globals;
    // The functions in the module.
    std::vector<std::shared_ptr<Function>> functions;

public:
    /**
     * @brief Constructs an empty MIR module.
     *
     * Do not call this constructor directly; use MIRModule::create instead.
     *
     * @param private Unused, but required to verify that you can call this
     * function here.
     */
    MIRModule(Private) {}

    /**
     * @brief Creates a new MIR module with the script function.
     *
     * @return The newly created MIR module.
     */
    static std::shared_ptr<MIRModule> create() {
        auto mod = std::make_shared<MIRModule>(Private());
        auto func = Function::create_script_function();
        mod->functions.push_back(func);
        return mod;
    }

    /**
     * @brief Creates a new function and adds it to the module.
     *
     * The function will have an entry and exit basic block created
     * automatically.
     *
     * The entry block will start without a terminator instruction.
     * During MIR building, the terminator instruction must be filled in at some
     * point.
     *
     * @param func_stmt The statement from which this function was
     * created.
     * @return The newly created function.
     */
    std::shared_ptr<Function>
    create_function(std::shared_ptr<Stmt::Func> func_stmt) {
        auto func = Function::create(func_stmt);
        functions.push_back(func);
        return func;
    }

    /**
     * @brief Retrieves the global variable corresponding to the given binding
     * entry, or declares a new one if it does not yet exist.
     *
     * @param binding_entry The binding entry for which to get or declare the
     * global variable.
     * @return std::shared_ptr<MIRValue::Global> The global variable
     * corresponding to the binding entry.
     */
    std::shared_ptr<MIRValue::Global>
    get_or_declare_global(std::shared_ptr<Node::BindingEntry> binding_entry);

    /**
     * @brief Gets the script function in the module.
     *
     * The script function is a special implicit function that contains
     * the top-level statements in the source code.
     *
     * For executables, this function is called by the `main` function.
     *
     * The script function is always the first function in the module.
     *
     * @return The script function.
     */
    std::shared_ptr<Function> get_script_function() {
        return functions.front();
    }

    /**
     * @brief Converts this module to a string.
     *
     * The string representation includes multiple lines and ends with a
     * newline.
     *
     * Note: The string representation includes the entire contents of the
     * module, including all functions and their basic blocks and instructions.
     *
     * @return A string representation of the module.
     */
    std::string to_string() const;
};

} // namespace nico

#endif // NICO_MIR_H
