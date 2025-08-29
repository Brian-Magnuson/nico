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

#include "../logger/logger.h"

void Emitter::emit(const std::unique_ptr<llvm::Module>& ir_module, std::string_view target_destination) {
    auto target_triple = llvm::sys::getDefaultTargetTriple();
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);
    if (!target) {
        Logger::inst().log_error(Err::EmitterCannotLookupTarget, "Failed to lookup target: " + error);
        return;
    }

    auto cpu = "generic";
    auto features = "";
    llvm::TargetOptions options;
    auto target_machine = target->createTargetMachine(target_triple, cpu, features, options, llvm::Reloc::PIC_);

    if (!target_machine) {
        Logger::inst().log_error(Err::EmitterCannotCreateTargetMachine, "Failed to create target machine for triple: " + target_triple);
        return;
    }

    ir_module->setDataLayout(target_machine->createDataLayout());

    // Create an output stream for the object file
    std::error_code err;
    llvm::raw_fd_ostream dest(target_destination, err, llvm::sys::fs::OF_None);
    if (err) {
        Logger::inst().log_error(Err::FileIO, "Error opening output file: " + err.message());
        return;
    }

    // Emit the module to the object file
    llvm::legacy::PassManager pass;
    auto file_type = llvm::CodeGenFileType::ObjectFile;
    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
        Logger::inst().log_error(Err::EmitterCannotEmitFile, "Target machine cannot emit a file of this type.");
        return;
    }

    pass.run(*ir_module);
    dest.flush();
}
