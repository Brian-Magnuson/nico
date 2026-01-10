#ifndef NICO_MIR_INSTRUCTIONS_H
#define NICO_MIR_INSTRUCTIONS_H

#include <memory>

#include "mir.h"

namespace nico {

class Instruction::INonTerminator : public Instruction {};

class Instruction::Binary : public INonTerminator {
    // Implementation details...
};

class Instruction::Unary : public INonTerminator {
    // Implementation details...
};

class Instruction::Call : public INonTerminator {
    // Implementation details...
};

class Instruction::ITerminator : public Instruction {};

class Instruction::Jump : public ITerminator {
public:
    std::weak_ptr<BasicBlock> target;

    virtual ~Jump() = default;

    Jump(std::shared_ptr<BasicBlock> target)
        : target(target) {}
};

class Instruction::Branch : public ITerminator {
public:
    std::shared_ptr<Value> condition;
    std::weak_ptr<BasicBlock> main_target;
    std::weak_ptr<BasicBlock> alt_target;

    virtual ~Branch() = default;

    Branch(
        std::shared_ptr<Value> condition,
        std::shared_ptr<BasicBlock> main_target,
        std::shared_ptr<BasicBlock> alt_target
    )
        : condition(condition),
          main_target(main_target),
          alt_target(alt_target) {}
};

class Instruction::Return : public ITerminator {
    // Implementation details...
};

} // namespace nico

#endif // NICO_MIR_INSTRUCTIONS_H
