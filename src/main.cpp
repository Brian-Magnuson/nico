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

void run_repl() {
    Frontend frontend;
    std::unique_ptr<IJit> jit = std::make_unique<SimpleJit>();
    std::string input;

    std::cout << ">> ";

    while (true) {
        Logger::inst().reset();
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }
        input += line;
        std::shared_ptr<CodeFile> code_file =
            std::make_shared<CodeFile>(std::move(input), "<stdin>");
        std::unique_ptr<FrontendContext>& context =
            frontend.compile(code_file, true);
        if (IS_VARIANT(context->status, Status::Ok)) {
            auto err = jit->add_module_and_context(std::move(context->mod_ctx));
            auto result = jit->run_main_func(0, nullptr, context->main_fn_name);
            input.clear();
            std::cout << ">> ";
        }
        else if (WITH_VARIANT(context->status, Status::Pause, pause_state)) {
            if (pause_state->request == Request::Input) {
                std::cout << ".. ";
                input += "\n";
            }
            else if (pause_state->request == Request::Discard) {
                input.clear();
                std::cout << "Input discarded." << std::endl;
                std::cout << ">> ";
            }
            else if (pause_state->request == Request::DiscardWarn) {
                input.clear();
                std::cout << "Input discarded.\n";
                std::cout << "Warning: REPL state was modified. Proceed with "
                             "caution.\n";
                std::cout << "Use `:reset` to clear the state.\n";
                std::cout << ">> ";
            }
            else if (pause_state->request == Request::Reset) {
                input.clear();
                frontend.reset();
                jit->reset();
                std::cout << ">> ";
            }
            else if (pause_state->request == Request::Exit) {
                std::cout << "Exiting REPL..." << std::endl;
                break;
            }
            else if (pause_state->request == Request::Help) {
                std::cout << "Help message not implemented yet." << std::endl;
                input.clear();
                std::cout << ">> ";
            }
            else {
                // Unknown request; reset the input.
                input.clear();
                std::cout << ">> ";
            }
        }
        else if (IS_VARIANT(context->status, Status::Error)) {
            // An error occurred; reset the input.
            std::cout << "Error occurred; exiting REPL..." << std::endl;
            break;
        }
    }
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
        run_repl();
    }

    return 0;
}
