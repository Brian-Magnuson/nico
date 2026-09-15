#include "nico_core/driver/jit_runner.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <utility>

#include "nico_core/backend/jit.h"
#include "nico_core/frontend/frontend.h"
#include "nico_core/shared/code_file.h"
#include "nico_core/shared/status.h"

namespace nico {

void compile_and_run(std::string_view file_name) {
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

    std::unique_ptr<IJIT> jit = std::make_unique<SimpleJIT>();
    auto err = jit->add_module_and_context(std::move(context->mod_ctx));

    auto result = jit->run_main_func(0, nullptr, context->main_fn_name);
}

} // namespace nico
