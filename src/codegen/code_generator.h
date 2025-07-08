#ifndef NICO_CODE_GENERATOR_H
#define NICO_CODE_GENERATOR_H

#include <memory>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "../parser/ast.h"

/**
 * @brief A class to perform LLVM code generation.
 *
 * This class assumes that the AST has been type-checked.
 * It does not perform type-checking, it does not check for memory safety, and it does not check for undefined behavior.
 */
class CodeGenerator {
    std::unique_ptr<llvm::Module> ir_module;

public:
    CodeGenerator() = default;
};

#endif // CODE_GENERATOR_H
