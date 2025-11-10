#include "nico/backend/jit.h"

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>

#include "nico/shared/logger.h"

namespace nico {

llvm::Expected<int>
IJit::run_main_func(int argc, char** argv, std::string_view main_fn_name) {
    auto symbol = lookup(main_fn_name);
    if (!symbol) {
        Logger::inst().log_error(
            Err::JitMissingEntryPoint,
            "Failed to find '" + std::string(main_fn_name) +
                "' function in JIT module: " +
                llvm::toString(symbol.takeError())
        );
        return llvm::make_error<llvm::StringError>(
            "Failed to find entry point",
            llvm::inconvertibleErrorCode()
        );
    }
    auto addr = symbol->getValue();
    if (!addr) {
        Logger::inst().log_error(
            Err::JitBadMainPointer,
            "Cannot cast 'main' function address to a "
            "function pointer because it is null."
        );
        return llvm::make_error<llvm::StringError>(
            "Null function pointer",
            llvm::inconvertibleErrorCode()
        );
    }
    using FuncPtr = int (*)(int, char**);
    auto func = reinterpret_cast<FuncPtr>(addr);
    if (!func) {
        Logger::inst().log_error(
            Err::JitBadMainPointer,
            "Failed to cast '" + std::string(main_fn_name) +
                "' function address to a function pointer."
        );
        return llvm::make_error<llvm::StringError>(
            "Failed to cast function pointer",
            llvm::inconvertibleErrorCode()
        );
    }

    return func(argc, argv);
}

SimpleJit::SimpleJit() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    auto jit_or_err = llvm::orc::LLJITBuilder().create();
    if (!jit_or_err) {
        Logger::inst().log_error(
            Err::JitCannotInstantiate,
            "Failed to create LLJIT: " + llvm::toString(jit_or_err.takeError())
        );
        exit(1);
    }
    jit = std::move(jit_or_err.get());
}

llvm::Error SimpleJit::add_module(llvm::orc::ThreadSafeModule tsm) {
    return jit->addIRModule(std::move(tsm));
}

llvm::Expected<llvm::orc::ExecutorAddr>
SimpleJit::lookup(std::string_view name) {
    return jit->lookup(name);
}

void SimpleJit::reset() {
    jit.reset(); // Destroys the current LLJIT instance
    auto jit_or_err = llvm::orc::LLJITBuilder().create();
    if (!jit_or_err) {
        Logger::inst().log_error(
            Err::JitCannotInstantiate,
            "Failed to create LLJIT: " + llvm::toString(jit_or_err.takeError())
        );
        exit(1);
    }
    jit = std::move(jit_or_err.get());
}

} // namespace nico
