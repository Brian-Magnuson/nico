#include "nico_core/backend/jit.h"

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>

#include "nico_rt/functions.h"

#include "nico_core/shared/diagnostics.h"
#include "nico_core/shared/error_code.h"
#include "nico_core/shared/utils.h"

namespace nico {

llvm::Expected<int>
IJIT::run_main_func(int argc, char** argv, std::string_view main_fn_name) {
    auto symbol = lookup(main_fn_name);
    if (!symbol) {
        std::string err_msg =
            "Failed to find/materialize '" + std::string(main_fn_name) +
            "' function in JIT module: " + llvm::toString(symbol.takeError());
        Diagnostics::inst().emit_error(Err::JITMissingEntryPoint, err_msg);
        return llvm::make_error<llvm::StringError>(
            err_msg,
            llvm::inconvertibleErrorCode()
        );
    }
    auto addr = symbol->getValue();
    if (!addr) {
        panic("Function address of `main` is null.");
    }
    using FuncPtr = int (*)(int, char**);
    auto func = reinterpret_cast<FuncPtr>(addr);
    if (!func) {
        panic("Failed to cast `main` function address to a function pointer");
    }

    return func(argc, argv);
}

SimpleJIT::SimpleJIT() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    reset();
}

llvm::Error SimpleJIT::add_module(llvm::orc::ThreadSafeModule tsm) {
    return jit->addIRModule(std::move(tsm));
}

llvm::Expected<llvm::orc::ExecutorAddr>
SimpleJIT::lookup(std::string_view name) {
    return jit->lookup(name);
}

void SimpleJIT::define_nico_rt_symbols() {
    llvm::orc::SymbolMap external_symbols;

    const auto add_symbol = [&](const char* name, auto* address) {
        external_symbols[jit->getExecutionSession().intern(name)] =
            llvm::orc::ExecutorSymbolDef::fromPtr(
                address,
                llvm::JITSymbolFlags::Exported
            );
    };

    add_symbol("nico_rt_printf", &nico_rt_printf);
    add_symbol("nico_rt_printerrf", &nico_rt_printerrf);
    add_symbol("nico_rt_abort", &nico_rt_abort);
    add_symbol("nico_rt_exit", &nico_rt_exit);
    add_symbol("nico_rt_malloc", &nico_rt_malloc);
    add_symbol("nico_rt_free", &nico_rt_free);
    add_symbol("nico_rt_panic", &nico_rt_panic);
    add_symbol("nico_rt_throw", &nico_rt_throw);

    if (auto error = jit->getMainJITDylib().define(
            llvm::orc::absoluteSymbols(external_symbols)
        )) {
        llvm::logAllUnhandledErrors(
            std::move(error),
            llvm::errs(),
            "Failed to define external symbols: "
        );
        panic("Failed to define external symbols in JIT");
    }
}

void SimpleJIT::reset() {
    jit.reset(); // Destroys the current LLJIT instance.

    // Create a new LLJIT instance using LLVM's default target triple.
    llvm::orc::JITTargetMachineBuilder jit_target_machine_builder(
        (llvm::Triple(llvm::sys::getDefaultTargetTriple()))
    );
    auto jit_or_err =
        llvm::orc::LLJITBuilder()
            .setJITTargetMachineBuilder(std::move(jit_target_machine_builder))
            .create();
    if (!jit_or_err) {
        panic(
            "Failed to create LLJIT: " + llvm::toString(jit_or_err.takeError())
        );
    }
    jit = std::move(jit_or_err.get());

    // Set up the error reporter for the execution session.
    auto& execution_session = jit->getExecutionSession();
    execution_session.setErrorReporter([](llvm::Error err) {
        std::string err_msg = llvm::toString(std::move(err));
        Diagnostics::inst().emit_error(
            Err::JITSessionError,
            "JIT execution session reported an error: " + err_msg
        );
    });

    // Make this process's symbols available to the JIT, so that it can resolve
    // calls to most C/C++ standard library functions.
    jit->getMainJITDylib().addGenerator(cantFail(
        llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
            jit->getDataLayout().getGlobalPrefix()
        )
    ));

    // Define the nico runtime symbols in the JIT.
    define_nico_rt_symbols();
}

llvm::Error SimpleJIT::add_static_library(std::filesystem::path lib_path) {
    auto err = jit->linkStaticLibraryInto(
        jit->getMainJITDylib(),
        lib_path.string().c_str()
    );
    if (err) {
        std::string err_msg = "Failed to add static library '" +
                              lib_path.string() +
                              "' to JIT: " + llvm::toString(std::move(err));

        Diagnostics::inst().emit_error(Err::JITCannotAddStaticLibrary, err_msg);

        return llvm::make_error<llvm::StringError>(
            err_msg,
            llvm::inconvertibleErrorCode()
        );
    }
    return llvm::Error::success();
}

} // namespace nico
