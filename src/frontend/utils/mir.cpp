#include "nico/frontend/utils/mir.h"

#include <any>
#include <memory>
#include <queue>
#include <string>
#include <string_view>

#include "nico/frontend/utils/mir_instructions.h"
#include "nico/frontend/utils/mir_values.h"

#include "nico/shared/utils.h"

namespace nico {

std::unordered_map<std::string, size_t> MIRValue::mir_temp_name_counters;

std::unordered_map<std::string, size_t> BasicBlock::bb_name_counters;

BasicBlock::BasicBlock(Private, std::string_view name)
    : name(
          std::string(name) + "#" +
          std::to_string(bb_name_counters[std::string(name)]++)
      ) {}

void BasicBlock::set_as_function_return() {
    if (terminator)
        panic(
            "BasicBlock::set_as_function_return: Basic block already has a "
            "terminator"
        );

    terminator = std::make_shared<Instr::Return>();
}

void BasicBlock::add_instruction(std::shared_ptr<Instr::INonTerm> instruction) {
    instructions.push_back(instruction);
}

void BasicBlock::set_successor(std::shared_ptr<BasicBlock> successor) {
    if (terminator)
        panic(
            "BasicBlock::set_successor: Basic block already has a "
            "terminator"
        );
    terminator = std::make_shared<Instr::Jump>(successor);
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

    terminator = std::make_shared<Instr::Branch>(
        condition,
        main_successor,
        alt_successor
    );
    main_successor->predecessors.push_back(shared_from_this());
    alt_successor->predecessors.push_back(shared_from_this());
}

std::vector<std::shared_ptr<BasicBlock>> BasicBlock::get_successors() const {
    std::vector<std::shared_ptr<BasicBlock>> successors;
    // Reserve space for up to 2 successors (for branches).
    successors.reserve(2);

    if (terminator) {
        if (auto jump = std::dynamic_pointer_cast<Instr::Jump>(terminator)) {
            // Jump has one successor
            if (auto bb = jump->target.lock()) {
                successors.push_back(bb);
            }
        }
        else if (auto branch =
                     std::dynamic_pointer_cast<Instr::Branch>(terminator)) {
            // Branch has two successors
            if (auto bb = branch->main_target.lock()) {
                successors.push_back(bb);
            }
            if (auto bb = branch->alt_target.lock()) {
                successors.push_back(bb);
            }
        }
        // Return has no successors, so do nothing
    }

    return successors;
}

std::string BasicBlock::to_string() const {
    std::string result = name + " <-- [ ";
    for (const auto& pred_weak : predecessors) {
        if (auto pred = pred_weak.lock()) {
            result += pred->get_name() + " ";
        }
    }
    result += "]\n";

    for (const auto& instr : instructions) {
        result += "  " + instr->to_string() + "\n";
    }
    if (terminator) {
        result += "  " + terminator->to_string() + "\n";
    }
    else {
        result += "  <no terminator>\n";
    }
    return result;
}

std::shared_ptr<Type> Function::get_return_type() const {
    return return_value->type;
}

std::shared_ptr<Function>
Function::create(std::shared_ptr<Stmt::Func> func_stmt) {
    auto func = std::make_shared<Function>(Private());
    auto field_entry = func_stmt->field_entry.lock();

    func->name = field_entry->get_symbol();
    func->return_type = field_entry->field.type;
    for (const auto& param : func_stmt->parameters) {
        auto param_var =
            std::make_shared<MIRValue::Variable>(param.field_entry.lock());
        func->parameters.push_back(param_var);
    }
    func->return_value =
        std::make_shared<MIRValue::Variable>("$ret_val", func->return_type);

    func->entry_block =
        std::make_shared<BasicBlock>(BasicBlock::Private(), "entry");
    func->entry_block->parent_function = func;

    auto exit = func->create_basic_block("exit");
    func->exit_block = exit;
    exit->set_as_function_return();

    return func;
}

std::shared_ptr<Function> Function::create_script_function() {
    auto func = std::make_shared<Function>(Private());
    func->name = "$script";
    func->return_type = std::make_shared<Type::Unit>();
    func->return_value = std::make_shared<MIRValue::Variable>(
        "$script_ret_val",
        func->return_type
    );

    func->entry_block =
        std::make_shared<BasicBlock>(BasicBlock::Private(), "entry");
    func->entry_block->parent_function = func;

    auto exit = func->create_basic_block("exit");
    func->exit_block = exit;
    exit->set_as_function_return();

    return func;
}

std::shared_ptr<BasicBlock>
Function::create_basic_block(std::string_view bb_name) {
    auto bb = std::make_shared<BasicBlock>(BasicBlock::Private(), bb_name);
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

        auto successors = current->get_successors();
        for (const auto& succ : successors) {
            if (!visited.contains(succ)) {
                visited.insert(succ);
                to_visit.push(succ);
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

std::string Function::to_string() const {
    std::string result = "func " + name + "( ";
    for (const auto& param : parameters) {
        result += param->to_string() + " ";
    }
    result += ") -> " + return_type->to_string() + " {\n";

    // Print each basic block.
    for (const auto& bb : basic_blocks) {
        result += bb->to_string() + "\n";
    }

    result += "}\n";
    return result;
}

} // namespace nico
