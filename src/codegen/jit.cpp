#include "jit.h"

#include <llvm/Support/InitLLVM.h>

SimpleJit::SimpleJit() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    auto jit_or_err = llvm::orc::LLJITBuilder().create();
    if (!jit_or_err) {
        llvm::errs() << "Failed to create LLJIT: " << toString(jit_or_err.takeError()) << "\n";
        exit(1);
    }
    jit = std::move(jit_or_err.get());
}

llvm::Error SimpleJit::add_module(llvm::orc::ThreadSafeModule tsm) {
    return jit->addIRModule(std::move(tsm));
}

llvm::Expected<llvm::orc::ExecutorAddr> SimpleJit::lookup(const std::string& name) {
    return jit->lookup(name);
}
