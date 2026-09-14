#include "nico_core/driver/aot_runner.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

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

    // Open the file.
    std::ifstream file(file_name.data());
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << file_name << std::endl;
        std::exit(66);
    }

    // Read the file's path.
    std::filesystem::path path = file_name;

    // Read the entire file.
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string src_code;
    src_code.resize(size);
    file.read(&src_code[0], size);

    std::shared_ptr<CodeFile> code_file = std::make_shared<CodeFile>(
        std::move(src_code),
        std::filesystem::absolute(path).string()
    );

    file.close();

    Frontend frontend;
    std::unique_ptr<FrontendContext>& context =
        frontend.compile(code_file, false);
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
