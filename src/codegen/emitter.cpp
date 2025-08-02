#include "emitter.h"

#include <string>
#include <system_error>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

void Emitter::emit(const std::unique_ptr<llvm::Module>& ir_module, const std::string& target_destination) {
    auto target_triple = llvm::sys::getDefaultTargetTriple();
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);
    if (!target) {
        llvm::errs() << "Error: " << error << "\n";
        return;
    }

    auto cpu = "generic";
    auto features = "";
    llvm::TargetOptions options;
    auto target_machine = target->createTargetMachine(target_triple, cpu, features, options, llvm::Reloc::PIC_);

    if (!target_machine) {
        llvm::errs() << "Error creating target machine.\n";
        return;
    }

    ir_module->setDataLayout(target_machine->createDataLayout());

    // Create an output stream for the object file
    std::error_code err;
    llvm::raw_fd_ostream dest(target_destination, err, llvm::sys::fs::OF_None);
    if (err) {
        llvm::errs() << "Error opening file: " << err.message() << "\n";
        return;
    }

    // Emit the module to the object file
    llvm::legacy::PassManager pass;
    auto file_type = llvm::CodeGenFileType::ObjectFile;
    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
        llvm::errs() << "Target machine can't emit a file of this type.\n";
        return;
    }

    pass.run(*ir_module);
    dest.flush();

    llvm::outs() << "Module emitted successfully to " << target_destination << "\n";
}
