#include "nico/backend/jit.h"

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>

#include "nico/shared/error_code.h"
#include "nico/shared/logger.h"
#include "nico/shared/utils.h"

namespace nico {

llvm::Expected<int>
IJIT::run_main_func(int argc, char** argv, std::string_view main_fn_name) {
    auto symbol = lookup(main_fn_name);
    if (!symbol) {
        std::string err_msg =
            "Failed to find '" + std::string(main_fn_name) +
            "' function in JIT module: " + llvm::toString(symbol.takeError());
        Logger::inst().log_error(Err::JITMissingEntryPoint, err_msg);
        return llvm::make_error<llvm::StringError>(
            err_msg,
            llvm::inconvertibleErrorCode()
        );
    }
    auto addr = symbol->getValue();
    if (!addr) {
        panic("IJIT::run_main_func: 'main' function address is null");
    }
    using FuncPtr = int (*)(int, char**);
    auto func = reinterpret_cast<FuncPtr>(addr);
    if (!func) {
        panic(
            "IJIT::run_main_func: Failed to cast 'main' function address to a "
            "function pointer"
        );
    }

    return func(argc, argv);
}

SimpleJIT::SimpleJIT() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    auto jit_or_err = llvm::orc::LLJITBuilder().create();
    if (!jit_or_err) {
        panic(
            "SimpleJIT::SimpleJIT: Failed to create LLJIT: " +
            llvm::toString(jit_or_err.takeError())
        );
    }
    jit = std::move(jit_or_err.get());
}

llvm::Error SimpleJIT::add_module(llvm::orc::ThreadSafeModule tsm) {
    return jit->addIRModule(std::move(tsm));
}

llvm::Expected<llvm::orc::ExecutorAddr>
SimpleJIT::lookup(std::string_view name) {
    return jit->lookup(name);
}

void SimpleJIT::reset() {
    jit.reset(); // Destroys the current LLJIT instance
    auto jit_or_err = llvm::orc::LLJITBuilder().create();
    if (!jit_or_err) {
        panic(
            "SimpleJIT::reset: Failed to create LLJIT: " +
            llvm::toString(jit_or_err.takeError())
        );
    }
    jit = std::move(jit_or_err.get());
}

llvm::Error SimpleJIT::add_static_library(const std::string& lib_path) {
    auto err =
        jit->linkStaticLibraryInto(jit->getMainJITDylib(), lib_path.c_str());
    if (err) {
        std::string err_msg = "Failed to add static library '" + lib_path +
                              "' to JIT: " + llvm::toString(std::move(err));

        Logger::inst().log_error(Err::JITCannotAddStaticLibrary, err_msg);

        return llvm::make_error<llvm::StringError>(
            err_msg,
            llvm::inconvertibleErrorCode()
        );
    }
    return llvm::Error::success();
}

} // namespace nico
