#ifndef NICO_JIT_H
#define NICO_JIT_H

#include <memory>
#include <string_view>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>

#include "nico/shared/ir_module_context.h"

/**
 * @brief Interface for JIT compilation.
 *
 * A JIT (Just-In-Time) compiler compiles code as it is needed, rather than
 * ahead of time. Though different from an interpreter, the behavior is similar
 * in that it enables dynamic code execution.
 *
 * JIT compilers may or may not support optimizations; optimizations more ideal
 * for ahead-of-time compilation may not be applicable in a JIT context.
 */
class IJit {
public:
    virtual ~IJit() = default;

    /**
     * @brief Adds a module to the JIT. Ownership of the module and context is
     * transferred to the JIT.
     *
     * @param tsm ThreadSafeModule containing the module and context to be
     * added.
     * @return An Error indicating success or failure of the operation.
     */
    virtual llvm::Error add_module(llvm::orc::ThreadSafeModule tsm) = 0;

    /**
     * @brief Adds an IrModuleContext to the JIT. Accepts ownership of both
     * Module and Context.
     *
     * @param mod_ctx (Requires move) The IrModuleContext to be added.
     * @return An Error indicating success or failure of the operation.
     */
    virtual llvm::Error add_module_and_context(IrModuleContext&& mod_ctx) {
        llvm::orc::ThreadSafeModule tsm(
            std::move(mod_ctx.ir_module),
            std::move(mod_ctx.llvm_context)
        );
        return add_module(std::move(tsm));
    }

    /**
     * @brief Looks up a symbol by name in the JIT.
     *
     * @param name The name of the symbol to look up.
     * @return An Expected containing the address of the symbol if found, or an
     * error if not found.
     */
    virtual llvm::Expected<llvm::orc::ExecutorAddr>
    lookup(std::string_view name) = 0;

    /**
     * @brief Runs the main function of the JIT-compiled module.
     *
     * @param argc The number of command-line arguments.
     * @param argv The command-line arguments.
     * @param main_fn_name The name of the main function to run. Defaults to
     * "main".
     * @return An Expected containing the return value of the main function, or
     * an error if the function could not be run.
     */
    virtual llvm::Expected<int> run_main_func(
        int argc, char** argv, std::string_view main_fn_name = "main"
    );

    /**
     * @brief Resets the JIT to its initial state, clearing all added modules.
     */
    virtual void reset() = 0;
};

/**
 * @brief A simple JIT implementation using LLVM's LLJIT.
 *
 * This class provides a basic JIT compiler that can add modules and look up
 * symbols.
 */
class SimpleJit : public IJit {
    // LLJIT instance for managing JIT compilation.
    std::unique_ptr<llvm::orc::LLJIT> jit;

public:
    SimpleJit();

    llvm::Error add_module(llvm::orc::ThreadSafeModule tsm) override;

    llvm::Expected<llvm::orc::ExecutorAddr>
    lookup(std::string_view name) override;

    void reset() override;
};

#endif // NICO_JIT_H
