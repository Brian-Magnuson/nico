#ifndef NICO_MIR_H
#define NICO_MIR_H

#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace nico {

class Value {
public:
    class Literal;
    class Variable;
    class Temporary;

    virtual ~Value() = default;

    /**
     * @brief A visitor class for values.
     */
    class Visitor {
    public:
        virtual std::any visit(Literal* value) = 0;
        virtual std::any visit(Variable* value) = 0;
        virtual std::any visit(Temporary* value) = 0;
    };

    /**
     * @brief Accept a visitor.
     *
     * @param visitor The visitor to accept.
     * @return The return value from the visitor.
     */
    virtual std::any accept(Visitor* visitor) = 0;
};

class Instruction {
public:
    class INonTerminator;
    class Binary;
    class Unary;
    class Call;

    class ITerminator;
    class Jump;
    class Branch;
    class Return;

    virtual ~Instruction() = default;

    /**
     * @brief A visitor class for instructions.
     */
    class Visitor {
    public:
        virtual std::any visit(Binary* instr) = 0;
        virtual std::any visit(Unary* instr) = 0;
        virtual std::any visit(Call* instr) = 0;
        virtual std::any visit(Jump* instr) = 0;
        virtual std::any visit(Branch* instr) = 0;
        virtual std::any visit(Return* instr) = 0;
    };

    /**
     * @brief Accept a visitor.
     *
     * @param visitor The visitor to accept.
     * @return The return value from the visitor.
     */
    virtual std::any accept(Visitor* visitor) = 0;
};

class Function;

class BasicBlock : public std::enable_shared_from_this<BasicBlock> {
private:
    friend class Function;

    // The name of the basic block.
    std::string name;
    // The instructions in the basic block.
    std::vector<std::shared_ptr<Instruction::INonTerminator>> instructions;
    // The terminator instruction of the basic block.
    std::shared_ptr<Instruction::ITerminator> terminator;
    // The parent function of the basic block.
    std::weak_ptr<Function> parent_function;

    // This block's predecessors in the control flow graph.
    std::vector<std::weak_ptr<BasicBlock>> predecessors;

protected:
    BasicBlock(const std::string& name);

public:
    /**
     * @brief Get the non-terminator instructions in the basic block.
     *
     * @return The non-terminator instructions in the basic block.
     */
    const std::vector<std::shared_ptr<Instruction::INonTerminator>>&
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
    void
    add_instruction(std::shared_ptr<Instruction::INonTerminator> instruction);

    /**
     * @brief Sets this block to use a jump terminator to the given successor.
     *
     * @param successor The successor basic block.
     */
    void set_successor(std::shared_ptr<BasicBlock> successor);

    /**
     * @brief Sets this block to use a branch terminator with the given
     * condition and successors.
     *
     * @param condition The condition value for the branch.
     * @param main_successor The main successor basic block.
     * @param alt_successor The alternative successor basic block.
     */
    void set_successors(
        std::shared_ptr<Value> condition,
        std::shared_ptr<BasicBlock> main_successor,
        std::shared_ptr<BasicBlock> alt_successor
    );

    /**
     * @brief Sets this block to use a return terminator.
     */
    void set_as_return();
};

class Function : public std::enable_shared_from_this<Function> {
public:
    // The name of the function.
    std::string name;
    // A special temporary value for the return value.
    std::shared_ptr<Value::Temporary> return_value;
    // The basic blocks in the function.
    std::vector<std::shared_ptr<BasicBlock>> basic_blocks;

    Function(const std::string& name);

    /**
     * @brief Creates a new basic block and adds it to the function.
     * @param bb_name The name of the basic block.
     * @return The newly created basic block.
     */
    std::shared_ptr<BasicBlock> create_basic_block(std::string_view bb_name);
};

} // namespace nico

#endif // NICO_MIR_H
