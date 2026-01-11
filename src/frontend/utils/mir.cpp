#include "nico/frontend/utils/mir.h"

#include <any>
#include <memory>
#include <queue>
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
    func->entry_block = std::make_shared<BasicBlock>("entry");
    func->entry_block->parent_function = func;

    auto exit = func->create_basic_block("exit");
    func->exit_block = exit;
    exit->set_as_function_return();

    return func;
}

std::shared_ptr<BasicBlock>
Function::create_basic_block(std::string_view bb_name) {
    auto bb = std::make_shared<BasicBlock>(bb_name);
    bb->parent_function = shared_from_this();
    basic_blocks.insert(bb);
    return bb;
}

void Function::purge_unreachable_blocks() {
    std::queue<std::shared_ptr<BasicBlock>> to_visit;
    std::unordered_set<std::shared_ptr<BasicBlock>> visited;

    to_visit.push(entry_block);
    visited.insert(entry_block);

    while (!to_visit.empty()) {
        auto current = to_visit.front();
        to_visit.pop();

        // Get successors from the terminator instruction.
        if (current->terminator) {
            if (auto branch_instr =
                    std::dynamic_pointer_cast<Instruction::Branch>(
                        current->terminator
                    )) {
                auto main_target = branch_instr->main_target.lock();
                auto alt_target = branch_instr->alt_target.lock();
                if (main_target && !visited.contains(main_target)) {
                    visited.insert(main_target);
                    to_visit.push(main_target);
                }
                if (alt_target && !visited.contains(alt_target)) {
                    visited.insert(alt_target);
                    to_visit.push(alt_target);
                }
            }
            else {
                if (auto jump_instr =
                        std::dynamic_pointer_cast<Instruction::Jump>(
                            current->terminator
                        )) {
                    auto target = jump_instr->target.lock();
                    if (target && !visited.contains(target)) {
                        visited.insert(target);
                        to_visit.push(target);
                    }
                }
            }
        }
    }

    // Now remove unvisited blocks from basic_blocks.
    for (auto it = basic_blocks.begin(); it != basic_blocks.end();) {
        if (!visited.contains(*it)) {
            it = basic_blocks.erase(it);
        }
        else {
            ++it;
        }
    }
}

} // namespace nico
