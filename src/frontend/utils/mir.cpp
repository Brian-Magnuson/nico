#include "nico/frontend/utils/mir.h"

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
        panic("Basic block already has a terminator.");

    terminator = std::make_shared<Instr::Return>();
}

void BasicBlock::add_instruction(std::shared_ptr<Instr::INonTerm> instruction) {
    instructions.push_back(instruction);
}

void BasicBlock::set_successor(std::shared_ptr<BasicBlock> successor) {
    if (terminator)
        panic("Basic block already has a terminator.");
    terminator = std::make_shared<Instr::Jump>(successor);
    successor->predecessors.push_back(shared_from_this());
}

void BasicBlock::set_successors(
    std::shared_ptr<MIRValue> condition,
    std::shared_ptr<BasicBlock> main_successor,
    std::shared_ptr<BasicBlock> alt_successor
) {
    if (terminator)
        panic("Basic block already has a terminator.");

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
        else if (
            auto branch = std::dynamic_pointer_cast<Instr::Branch>(terminator)
        ) {
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
    std::string result = "  " + name + " <-- [ ";
    for (const auto& pred_weak : predecessors) {
        if (auto pred = pred_weak.lock()) {
            result += pred->get_name() + " ";
        }
    }
    result += "]\n";

    for (const auto& instr : instructions) {
        result += "    " + instr->to_string() + "\n";
    }
    if (terminator) {
        result += "    " + terminator->to_string() + "\n";
    }
    else {
        result += "    <no terminator>\n";
    }
    return result;
}

std::optional<std::shared_ptr<Function::ControlBlock>>
Function::ControlBlock::get_block(std::optional<std::string> label) {
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

std::optional<std::shared_ptr<Function::ControlLoop>>
Function::ControlBlock::get_loop(std::optional<std::string> label) {
    if (auto prev_block = prev.value_or(nullptr)) {
        return prev_block->get_loop(label);
    }
    else {
        return std::nullopt;
    }
}

std::optional<std::shared_ptr<Function::ControlLoop>>
Function::ControlLoop::get_loop(std::optional<std::string> label) {
    if (!label || this->label == label) {
        return std::dynamic_pointer_cast<ControlLoop>(shared_from_this());
    }
    else if (auto prev_block = prev.value_or(nullptr)) {
        return prev_block->get_loop(label);
    }
    else {
        return std::nullopt;
    }
}

std::shared_ptr<Function>
Function::create(std::shared_ptr<Stmt::Func> func_stmt) {
    auto func = std::make_shared<Function>(Private());
    auto binding_entry = func_stmt->binding_entry.lock();

    func->name = binding_entry->symbol;
    func->return_type = binding_entry->binding.type;
    for (const auto& param : func_stmt->parameters) {
        auto param_var = MIRValue::Variable::create(param.binding_entry.lock());
        func->parameters.push_back(param_var);
    }
    func->return_variable =
        MIRValue::Variable::create("$ret_val", func->return_type);

    func->entry_block = func->create_basic_block("entry");

    auto exit = func->create_basic_block("exit");
    func->exit_block = exit;
    exit->set_as_function_return();

    return func;
}

std::shared_ptr<Function> Function::create_script_function() {
    auto func = std::make_shared<Function>(Private());
    func->name = "$script";
    func->return_type = std::make_shared<Type::Void>();
    func->return_variable =
        MIRValue::Variable::create("$script_ret_val", func->return_type);

    func->entry_block = func->create_basic_block("entry");

    auto exit = func->create_basic_block("exit");
    func->exit_block = exit;
    exit->set_as_function_return();

    return func;
}

std::shared_ptr<Type> Function::get_return_type() const {
    return return_variable->type;
}

std::shared_ptr<MIRValue::Variable> Function::create_local_variable(
    std::shared_ptr<Node::BindingEntry> binding_entry
) {
    auto var = MIRValue::Variable::create(binding_entry);
    locals[binding_entry->symbol] = var;
    return var;
}

std::shared_ptr<MIRValue::Variable> Function::get_local_variable(
    std::shared_ptr<Node::BindingEntry> binding_entry
) {
    auto it = locals.find(binding_entry->symbol);
    if (it != locals.end()) {
        return it->second;
    }
    else {
        panic("No local variable found for binding entry.");
    }
}

std::shared_ptr<MIRValue::Variable> Function::get_yield_variable(
    Expr::Block::Kind kind, std::optional<std::string> label
) const {
    if (!control_stack.has_value() && kind != Expr::Block::Kind::Function) {
        panic("No control stack; cannot get yield variable.");
    }
    if (kind == Expr::Block::Kind::Function) {
        return return_variable;
    }
    if (kind == Expr::Block::Kind::Loop) {
        if (auto loop_block =
                control_stack.value()->get_loop(label).value_or(nullptr)) {
            return loop_block->yield_variable;
        }
        else {
            panic("No matching loop block found.");
        }
    }
    if (auto block =
            control_stack.value()->get_block(label).value_or(nullptr)) {
        return block->yield_variable;
    }
    else {
        panic("No matching block found for label.");
    }
}

std::shared_ptr<BasicBlock>
Function::create_basic_block(std::string_view bb_name) {
    auto bb = std::make_shared<BasicBlock>(BasicBlock::Private(), bb_name);
    bb->parent_function = shared_from_this();
    basic_blocks.push_back(bb);
    return bb;
}

void Function::add_plain_control_block(
    std::shared_ptr<MIRValue::Variable> yield_variable,
    const std::optional<std::string> label
) {
    control_stack =
        std::make_shared<ControlBlock>(control_stack, yield_variable, label);
}

void Function::add_loop_control_block(
    std::weak_ptr<MIRValue::Variable> yield_variable,
    std::weak_ptr<BasicBlock> continue_block,
    std::weak_ptr<BasicBlock> exit_block,
    const std::optional<std::string> label
) {
    control_stack = std::make_shared<ControlLoop>(
        control_stack,
        yield_variable.lock(),
        continue_block,
        exit_block,
        label
    );
}

void Function::pop_control_block() {
    if (!control_stack.has_value()) {
        panic("No control stack; cannot pop block.");
    }
    control_stack = control_stack.value()->prev;
    // Top block is deallocated as it is no longer referenced by the control
    // stack.
}

std::shared_ptr<BasicBlock>
Function::get_loop_continue_block(std::optional<std::string> label) {
    if (!control_stack.has_value()) {
        panic("No control stack; cannot get loop continue block.");
    }
    if (auto loop_block =
            control_stack.value()->get_loop(label).value_or(nullptr)) {
        return loop_block->continue_block.lock();
    }
    else {
        panic("No matching loop block found.");
    }
}

std::shared_ptr<BasicBlock> Function::get_exit_block(
    Expr::Block::Kind kind, std::optional<std::string> label
) {
    if (kind == Expr::Block::Kind::Plain) {
        panic("Illegal argument; kind cannot be Plain.");
    }
    else if (kind == Expr::Block::Kind::Loop) {
        if (!control_stack.has_value()) {
            panic("No control stack; cannot get loop exit block.");
        }
        if (auto loop_block =
                control_stack.value()->get_loop(label).value_or(nullptr)) {
            return loop_block->exit_block.lock();
        }
        else {
            panic("No matching loop block found.");
        }
    }
    else {
        return exit_block;
    }
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
    result.resize(result.size() - 1); // Remove the last newline

    result += "}\n";
    return result;
}

std::shared_ptr<MIRValue::Global> MIRModule::get_or_declare_global(
    std::shared_ptr<Node::BindingEntry> binding_entry
) {
    std::string suffix =
        Type::is_a<Type::Function>(binding_entry->binding.type) ? "$var" : "";
    std::string global_name = binding_entry->symbol + suffix;

    if (auto global_opt = globals.at(global_name)) {
        return *global_opt;
    }
    else {
        auto global = MIRValue::Global::create(binding_entry);
        globals.insert(global_name, global);
        return global;
    }
}

std::string MIRModule::to_string() const {
    std::string result = "module\n\n";

    // Print each global variable.
    for (const auto& [global_name, global_var] : globals) {
        result +=
            "global " + global_name + " " + global_var->to_string() + "\n";
    }
    result += "\n";

    // Print each function.
    for (const auto& func : functions) {
        result += func->to_string() + "\n";
    }

    result.resize(result.size() - 1); // Remove the last newline

    return result;
}

} // namespace nico
