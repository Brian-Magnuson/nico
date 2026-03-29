#ifndef NICO_IR_MODULE_CONTEXT_H
#define NICO_IR_MODULE_CONTEXT_H

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

#include "nico/shared/error_code.h"
#include "nico/shared/logger.h"

namespace nico {

/**
 * @brief A safe wrapper around an LLVM context, module, and target machine.
 *
 * This class is move only. It cannot be copied.
 *
 * LLVM Contexts and modules are very closely related and are sensitive to
 * destruction order. This class ensures that the module is always destroyed
 * before the context, preventing potential segmentation faults.
 */
class IRModuleContext {
public:
    // The LLVM context used to generate the module.
    std::unique_ptr<llvm::LLVMContext> llvm_context;
    // The LLVM Module that will be generated.
    std::unique_ptr<llvm::Module> ir_module;
    // The target machine for code generation.
    std::unique_ptr<llvm::TargetMachine> target_machine;

    IRModuleContext()
        : llvm_context(nullptr), ir_module(nullptr) {}

    IRModuleContext(IRModuleContext&& other) {
        ir_module = nullptr;
        llvm_context = std::move(other.llvm_context);
        ir_module = std::move(other.ir_module);
        target_machine = std::move(other.target_machine);
    }
    IRModuleContext(const IRModuleContext&) = delete;

    IRModuleContext& operator=(IRModuleContext&& other) {
        if (this != &other) {
            ir_module = nullptr;
            llvm_context = std::move(other.llvm_context);
            ir_module = std::move(other.ir_module);
            target_machine = std::move(other.target_machine);
        }
        return *this;
    }
    IRModuleContext& operator=(const IRModuleContext&) = delete;

    ~IRModuleContext() {
        // Note: The order of destruction is important here. The module must be
        // destroyed before the context.
        ir_module = nullptr;
        llvm_context = nullptr;
        target_machine = nullptr;
    }

    /**
     * @brief Initialize an IRModuleContext with a new LLVM context and module.
     *
     * @param module_name The name of the module. Defaults to "main".
     */
    void initialize(std::string_view module_name = "main") {
        // The module must be destroyed before the context, so we set it to
        // nullptr first to ensure the correct destruction order in case of an
        // exception.
        ir_module = nullptr;
        llvm_context = std::make_unique<llvm::LLVMContext>();
        ir_module = std::make_unique<llvm::Module>(
            std::string(module_name),
            *llvm_context
        );

        auto target_triple = llvm::sys::getDefaultTargetTriple();
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmParser();
        llvm::InitializeNativeTargetAsmPrinter();

        std::string error;
        auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);
        if (!target) {
            Logger::inst().log_error(
                Err::CannotLookupTarget,
                "Failed to lookup target: " + error
            );
            return;
        }

        auto cpu = "generic";
        auto features = "";
        llvm::TargetOptions options;
        target_machine =
            std::unique_ptr<llvm::TargetMachine>(target->createTargetMachine(
                target_triple,
                cpu,
                features,
                options,
                llvm::Reloc::PIC_
            ));

        if (!target_machine) {
            Logger::inst().log_error(
                Err::CannotCreateTargetMachine,
                "Failed to create target machine for triple: " + target_triple
            );
            return;
        }

        ir_module->setDataLayout(target_machine->createDataLayout());
    }

    /**
     * @brief Get the width of a pointer in bits based on the data layout of the
     * IR module.
     *
     * @return The width of a pointer in bits.
     *
     * @warning This method assumes that the IR module has been initialized. If
     * the IR module is not initialized, this method will panic.
     */
    uint8_t get_ptr_width() const {
        if (ir_module == nullptr) {
            panic(
                "IRModuleContext::get_ptr_width: IRModuleContext is not "
                "initialized."
            );
        }
        return ir_module->getDataLayout().getPointerSizeInBits();
    }
};

} // namespace nico

#endif // NICO_IR_MODULE_CONTEXT_H
