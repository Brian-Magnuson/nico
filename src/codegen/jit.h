#ifndef NICO_JIT_H
#define NICO_JIT_H

#include <memory>
#include <string>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/TargetSelect.h>

/**
 * @brief Interface for JIT compilation.
 *
 * A JIT (Just-In-Time) compiler compiles code as it is needed, rather than ahead of time. Though different from an interpreter, the behavior is similar in that it enables dynamic code execution.
 *
 * JIT compilers may or may not support optimizations; optimizations more ideal for ahead-of-time compilation may not be applicable in a JIT context.
 */
class IJit {
public:
    virtual ~IJit() = default;

    /**
     * @brief Adds a module to the JIT. Ownership of the module and context is transferred to the JIT.
     *
     * @param tsm ThreadSafeModule containing the module and context to be added.
     * @return An Error indicating success or failure of the operation.
     */
    virtual llvm::Error add_module(llvm::orc::ThreadSafeModule tsm) = 0;

    /**
     * @brief Adds a module to the JIT. Accepts ownership of both Module and Context.
     *
     * By default, this method will take ownership of the provided module and context, wrap it in a ThreadSafeModule, then add it to the JIT.
     *
     * @param _module (Requires move) The module to be added.
     * @param context (Requires move) The context to be added.
     * @return An Error indicating success or failure of the operation.
     */
    virtual llvm::Error add_module(std::unique_ptr<llvm::Module> _module, std::unique_ptr<llvm::LLVMContext> context) {
        llvm::orc::ThreadSafeModule tsm(std::move(_module), std::move(context));
        return add_module(std::move(tsm));
    }

    /**
     * @brief Looks up a symbol by name in the JIT.
     *
     * @param name The name of the symbol to look up.
     * @return An Expected containing the address of the symbol if found, or an error if not found.
     */
    virtual llvm::Expected<llvm::orc::ExecutorAddr> lookup(const std::string& name) = 0;

    /**
     * @brief Runs the main function of the JIT-compiled module.
     * @param argc The number of command-line arguments.
     * @param argv The command-line arguments.
     * @return An Expected containing the return value of the main function, or an error if the function could not be run.
     */
    virtual llvm::Expected<int> run_main(int argc, char** argv);
};

/**
 * @brief A simple JIT implementation using LLVM's LLJIT.
 *
 * This class provides a basic JIT compiler that can add modules and look up symbols.
 */
class SimpleJit : public IJit {
    // LLJIT instance for managing JIT compilation.
    std::unique_ptr<llvm::orc::LLJIT> jit;

public:
    SimpleJit();

    llvm::Error add_module(llvm::orc::ThreadSafeModule tsm) override;

    llvm::Expected<llvm::orc::ExecutorAddr> lookup(const std::string& name) override;
};

#endif // NICO_JIT_H
