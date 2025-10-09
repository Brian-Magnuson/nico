#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "nico/backend/jit.h"
#include "nico/backend/repl.h"
#include "nico/frontend/frontend.h"
#include "nico/shared/code_file.h"
#include "nico/shared/logger.h"
#include "nico/shared/status.h"

void run_jit(std::string_view file_name) {
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

    std::unique_ptr<IJit> jit = std::make_unique<SimpleJit>();
    auto err = jit->add_module_and_context(std::move(context->mod_ctx));

    auto result = jit->run_main_func(0, nullptr, context->main_fn_name);
}

int main(int argc, char** argv) {
    if (argc > 2) {
        std::cout << "Usage: nico [source_file]" << std::endl;
        return 64;
    }

    if (argc == 2) {
        run_jit(argv[1]);
    }
    else {
        Repl::run();
    }

    return 0;
}
