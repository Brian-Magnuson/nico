#ifndef NICO_OPTIMIZER_H
#define NICO_OPTIMIZER_H

#include <memory>

#include <llvm/IR/Module.h>
#include <llvm/Passes/PassBuilder.h>

/**
 * @brief A class to perform optimization on an IR module.
 *
 * Optimization helps remove unnecessary code and make the code more efficient.
 * This may be unnecessary for some applications, such as JIT-compilation.
 */
class Optimizer {
public:
    /**
     * @brief Optimizes the given IR module.
     *
     * This step is optional and may be skipped.
     *
     * @param ir_module The IR module to optimize.
     * @param opt_level The optimization level to use. Defaults to O2.
     */
    void optimize(std::unique_ptr<llvm::Module>& ir_module, llvm::OptimizationLevel opt_level = llvm::OptimizationLevel::O2);
};

#endif // NICO_OPTIMIZER_H
