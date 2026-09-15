#include "nico_core/driver/aot_runner.h"

#include <cstdlib>
#include <iostream>

#include <llvm/Passes/OptimizationLevel.h>

#include "nico_core/backend/emitter.h"
#include "nico_core/backend/optimizer.h"
#include "nico_core/frontend/frontend.h"
#include "nico_core/shared/code_file.h"
#include "nico_core/shared/status.h"

namespace nico {

void compile_and_output(
    std::string_view file_name, std::filesystem::path target_destination
) {
    // TODO: Provide a better interface for more configuration options.

    std::optional<std::shared_ptr<CodeFile>> code_file_opt =
        CodeFile::from_file(std::filesystem::path(file_name));

    if (!code_file_opt) {
        std::cerr << "Could not read file: " << file_name << std::endl;
        std::exit(66);
    }

    Frontend frontend;
    std::unique_ptr<FrontendContext>& context =
        frontend.compile(code_file_opt.value(), false);
    if (!IS_VARIANT(context->status, Status::Ok)) {
        std::cerr << "Compilation failed; exiting...";
        std::exit(1);
    }

    Optimizer optimizer;
    optimizer.optimize(context->mod_ctx.ir_module, llvm::OptimizationLevel::O2);

    Emitter emitter;
    emitter.emit(context->mod_ctx, target_destination.string());
}

} // namespace nico
