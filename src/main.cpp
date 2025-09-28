#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "nico/backend/jit.h"
#include "nico/frontend/frontend.h"
#include "nico/shared/code_file.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: nico <source file>" << std::endl;
        return 64;
    }

    // Open the file.
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << argv[1] << std::endl;
        return 66;
    }

    // Read the file's path.
    std::filesystem::path path = argv[1];

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
    if (context->status == FrontendContext::Status::Error) {
        std::cerr << "Compilation failed; exiting...";
        return 1;
    }

    std::unique_ptr<IJit> jit = std::make_unique<SimpleJit>();
    auto err = jit->add_module_and_context(std::move(context->mod_ctx));

    auto result = jit->run_main(0, nullptr);

    return 0;
}
