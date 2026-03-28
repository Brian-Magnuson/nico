#include "nico/backend/emitter.h"

#include <string>
#include <string_view>
#include <system_error>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#include "nico/shared/logger.h"

namespace nico {

void Emitter::emit(
    const IRModuleContext& mod_ctx, std::string_view target_destination
) {
    // Create an output stream for the object file
    std::error_code err;
    llvm::raw_fd_ostream dest(target_destination, err, llvm::sys::fs::OF_None);
    if (err) {
        Logger::inst().log_error(
            Err::FileIO,
            "Error opening output file: " + err.message()
        );
        return;
    }

    // Emit the module to the object file
    llvm::legacy::PassManager pass;
    auto file_type = llvm::CodeGenFileType::ObjectFile;
    if (mod_ctx.target_machine
            ->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
        Logger::inst().log_error(
            Err::EmitterCannotEmitFile,
            "Target machine cannot emit a file of this type."
        );
        return;
    }

    pass.run(*mod_ctx.ir_module);
    dest.flush();
}

} // namespace nico
