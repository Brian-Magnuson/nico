#include "nico/frontend/utils/mir.h"

#include <any>
#include <memory>
#include <string>
#include <string_view>

#include "nico/frontend/utils/mir_instructions.h"

#include "nico/shared/utils.h"

namespace nico {

BasicBlock::BasicBlock(std::string_view name)
    : name(name) {}

void BasicBlock::set_as_function_return() {
    if (terminator)
        panic(
            "BasicBlock::set_as_function_return: Basic block already has a "
            "terminator"
        );

    terminator = std::make_shared<Instruction::Return>();
}

void BasicBlock::add_instruction(
    std::shared_ptr<Instruction::INonTerminator> instruction
) {
    instructions.push_back(instruction);
}

void BasicBlock::set_successor(std::shared_ptr<BasicBlock> successor) {
    if (terminator)
        panic(
            "BasicBlock::set_successor: Basic block already has a "
            "terminator"
        );
    terminator = std::make_shared<Instruction::Jump>(successor);
    successor->predecessors.push_back(shared_from_this());
}

void BasicBlock::set_successors(
    std::shared_ptr<MIRValue> condition,
    std::shared_ptr<BasicBlock> main_successor,
    std::shared_ptr<BasicBlock> alt_successor
) {
    if (terminator)
        panic(
            "BasicBlock::set_successors: Basic block already has a "
            "terminator"
        );

    terminator = std::make_shared<Instruction::Branch>(
        condition,
        main_successor,
        alt_successor
    );
    main_successor->predecessors.push_back(shared_from_this());
    alt_successor->predecessors.push_back(shared_from_this());
}

Function::Function(const std::string& name)
    : name(name) {}

std::shared_ptr<Function> Function::create(const std::string& name) {
    auto func = std::make_shared<Function>(name);
    auto entry = func->create_basic_block("entry");
    auto exit = func->create_basic_block("exit");

    exit->set_as_function_return();

    return func;
}

std::shared_ptr<BasicBlock>
Function::create_basic_block(std::string_view bb_name) {
    auto bb = std::make_shared<BasicBlock>(bb_name);
    bb->parent_function = shared_from_this();
    basic_blocks.push_back(bb);
    return bb;
}

} // namespace nico
