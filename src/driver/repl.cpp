#include "nico/driver/repl.h"

#include "nico/shared/code_file.h"
#include "nico/shared/logger.h"
#include "nico/shared/status.h"

const std::unordered_map<std::string, Repl::Command> Repl::commands = {
    {":help", Command::Help},
    {":h", Command::Help},
    {":?", Command::Help},
    {":version", Command::Version},
    {":license", Command::License},
    {":discard", Command::Discard},
    {":reset", Command::Reset},
    {":exit", Command::Exit},
    {":quit", Command::Exit},
    {":q", Command::Exit}
};

void Repl::discard(bool with_warning) {
    input.clear();
    continue_mode = false;
    if (with_warning) {
        *out << "Input discarded.\n";
        *out << "Warning: REPL state was modified. Proceed with caution.\n";
        *out << "Use `:reset` to clear the state.\n";
        use_caution = true;
    }
    else {
        *out << "Input discarded." << std::endl;
    }
}

void Repl::reset() {
    input.clear();
    frontend.reset();
    jit->reset();
    *out << "REPL state has been reset.\n";
    continue_mode = false;
    use_caution = false;
}

void Repl::print_version() {
    *out << project_version() << "\n";
}

void Repl::print_header() {
    *out << "Nico " << project_version() << "\n";
    *out << "Copyright (c) 2024 Brian Magnuson\n";
    *out << "Use ':help' for help and ':exit' to quit.\n";
}

void Repl::print_help() {
    *out << R"(Nico REPL Help
Available commands:
:help       Show this help message. (Also :h or :?)
:version    Show the REPL version.
:license    Show the LICENSE file.
:reset      Reset the REPL state, clearing all variables and definitions. 
:discard    Discard the current input.
:exit       Exit the REPL. (Also :quit or :q)
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
    if (continue_mode) {
        *out << colorize::gray << ".. " << colorize::reset;
    }
    else if (use_caution) {
        *out << colorize::yellow << ">> " << colorize::reset;
    }
    else {
        *out << colorize::green << ">> " << colorize::reset;
    }
}

void Repl::run(std::istream& in, std::ostream& out) {
    Repl repl(in, out);
    repl.run_repl();
}

void Repl::handle_command(Command cmd) {
    switch (cmd) {
    case Command::Help:
        print_help();
        break;
    case Command::Version:
        print_version();
        break;
    case Command::License:
        print_license();
        break;
    case Command::Discard:
        discard();
        break;
    case Command::Reset:
        reset();
        break;
    case Command::Exit:
        *out << "Exiting REPL..." << std::endl;
        exit(0);
    default:
        panic("Repl::handle_command: Unknown command");
        break;
    }
}

void Repl::run_repl() {
    print_header();

    while (true) {
        print_prompt();
        Logger::inst().reset();
        std::string line;
        if (!std::getline(*in, line)) {
            // Exit REPL on EOF (Ctrl+D)
            *out << "\nExiting REPL..." << std::endl;
            break;
        }
        else if (commands.find(line) != commands.end()) {
            handle_command(commands.at(line));
            continue;
        }
        // If the input is not a command, append it to the current input.
        input += line;
        // Put the input in a CodeFile.
        std::shared_ptr<CodeFile> code_file =
            std::make_shared<CodeFile>(std::move(input), "<stdin>");
        // Compile the CodeFile.
        std::unique_ptr<FrontendContext>& context =
            frontend.compile(code_file, true);

        if (IS_VARIANT(context->status, Status::Ok)) {
            // If the input compiled successfully, add the module to the JIT and
            // run the main function.
            auto err = jit->add_module_and_context(std::move(context->mod_ctx));
            auto result = jit->run_main_func(0, nullptr, context->main_fn_name);
            input.clear();
            continue_mode = false;
        }
        else if (WITH_VARIANT(context->status, Status::Pause, pause_state)) {
            if (pause_state->request == Request::Input) {
                continue_mode = true;
                input += "\n";
            }
            else if (pause_state->request == Request::Discard) {
                discard();
            }
            else if (pause_state->request == Request::DiscardWarn) {
                discard(true);
            }
        }
        else if (IS_VARIANT(context->status, Status::Error)) {
            // An error occurred; reset the input.
            *out << "Error occurred; exiting REPL..." << std::endl;
            break;
        }
    }
}
