#ifndef NICO_IR_MODULE_CONTEXT_H
#define NICO_IR_MODULE_CONTEXT_H

#include <memory>
#include <string>
#include <string_view>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

namespace nico {

/**
 * @brief A safe wrapper around an LLVM context and module.
 *
 * This class is move only. It cannot be copied.
 *
 * LLVM Contexts and modules are very closely related and are sensitive to
 * destruction order. This class ensures that the module is always destroyed
 * before the context, preventing potential segmentation faults.
 */
class IrModuleContext {
public:
    // The LLVM context used to generate the module.
    std::unique_ptr<llvm::LLVMContext> llvm_context;
    // The LLVM Module that will be generated.
    std::unique_ptr<llvm::Module> ir_module;

    IrModuleContext()
        : llvm_context(nullptr), ir_module(nullptr) {}

    IrModuleContext(IrModuleContext&& other) {
        ir_module = nullptr;
        llvm_context = std::move(other.llvm_context);
        ir_module = std::move(other.ir_module);
    }
    IrModuleContext(const IrModuleContext&) = delete;

    IrModuleContext& operator=(IrModuleContext&& other) {
        if (this != &other) {
            ir_module = nullptr;
            llvm_context = std::move(other.llvm_context);
            ir_module = std::move(other.ir_module);
        }
        return *this;
    }
    IrModuleContext& operator=(const IrModuleContext&) = delete;

    ~IrModuleContext() {
        ir_module = nullptr;
        llvm_context = nullptr;
    }

    /**
     * @brief Initialize an IrModuleContext with a new LLVM context and module.
     *
     * @param module_name The name of the module. Defaults to "main".
     */
    void initialize(std::string_view module_name = "main") {
        ir_module = nullptr;
        llvm_context = std::make_unique<llvm::LLVMContext>();
        ir_module = std::make_unique<llvm::Module>(
            std::string(module_name),
            *llvm_context
        );
    }

    /**
     * @brief Resets the LLVM context and module to their initial states.
     */
    void reset() {
        ir_module = nullptr;
        llvm_context = nullptr;
    }
};

} // namespace nico

#endif // NICO_IR_MODULE_CONTEXT_H
