#include "nico/backend/repl.h"

#include "nico/backend/jit.h"
#include "nico/frontend/frontend.h"
#include "nico/shared/code_file.h"
#include "nico/shared/logger.h"
#include "nico/shared/status.h"

void Repl::print_header() {
    *out << "Nico " << project_version() << "\n";
    *out << "Copyright (c) 2024 Brian Magnuson\n";
    *out << "Use ':help' for help and ':exit' to quit.\n";
}

void Repl::print_help() {
    *out << R"(Nico REPL Help
Available commands:
:help       Show this help message.
:license    Show the LICENSE file.
:reset      Reset the REPL state, clearing all variables and definitions. 
            (Also :clearall)
:discard    Discard the current input. (Also :clear or :cls)
)";
}

void Repl::print_license() {
    // Note: This is a temporary solution. Eventually, we should read the
    // LICENSE file from its file.
    *out << R"(MIT License
Copyright (c) 2024 Brian Magnuson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
)";
}

void Repl::print_prompt() {
    if (use_caution) {
        *out << colorize::yellow;
    }
    else {
        *out << colorize::green;
    }
    *out << ">> " << colorize::reset;
}

void Repl::print_continue_prompt() {
    *out << colorize::gray << ".. " << colorize::reset;
}

void Repl::run(std::istream& in, std::ostream& out) {
    Repl repl(in, out);
    repl.run_repl();
}

void Repl::run_repl() {
    Frontend frontend;
    std::unique_ptr<IJit> jit = std::make_unique<SimpleJit>();
    std::string input;

    print_header();
    print_prompt();

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
            print_prompt();
        }
        else if (WITH_VARIANT(context->status, Status::Pause, pause_state)) {
            if (pause_state->request == Request::Input) {
                print_continue_prompt();
                input += "\n";
            }
            else if (pause_state->request == Request::Discard) {
                input.clear();
                *out << "Input discarded." << std::endl;
                print_prompt();
            }
            else if (pause_state->request == Request::DiscardWarn) {
                input.clear();
                *out << "Input discarded.\n";
                *out << "Warning: REPL state was modified. Proceed with "
                        "caution.\n";
                *out << "Use `:reset` to clear the state.\n";
                use_caution = true;
                print_prompt();
            }
            else if (pause_state->request == Request::Reset) {
                input.clear();
                frontend.reset();
                jit->reset();
                *out << "REPL state has been reset.\n";
                use_caution = false;
                print_prompt();
            }
            else if (pause_state->request == Request::Exit) {
                *out << "Exiting REPL..." << std::endl;
                break;
            }
            else if (pause_state->request == Request::Help) {
                print_help();
                input.clear();
                print_prompt();
            }
            else if (pause_state->request == Request::License) {
                print_license();
                input.clear();
                print_prompt();
            }
            else {
                // Unknown request; reset the input.
                input.clear();
                print_prompt();
            }
        }
        else if (IS_VARIANT(context->status, Status::Error)) {
            // An error occurred; reset the input.
            *out << "Error occurred; exiting REPL..." << std::endl;
            break;
        }
    }
}
