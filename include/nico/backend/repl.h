#ifndef NICO_REPL_H
#define NICO_REPL_H

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "nico/backend/jit.h"
#include "nico/frontend/frontend.h"
#include "nico/shared/utils.h"

class Repl {
    enum class Command { Help, Version, License, Discard, Reset, Exit };

    static const std::unordered_map<std::string, Command> commands;

    // The input stream (usually std::cin).
    std::istream* const in = &std::cin;
    // The output stream (usually std::cout).
    std::ostream* const out = &std::cout;

    Frontend frontend;

    std::unique_ptr<IJit> jit = std::make_unique<SimpleJit>();

    std::string input;

    bool continue_mode = false;
    // Whether the REPL should proceed with caution (e.g., when state is
    // possibly corrupted).
    bool use_caution = false;

    Repl(std::istream& in = std::cin, std::ostream& out = std::cout)
        : in(&in), out(&out) {}

    void discard(bool with_warning = false);

    void reset();

    void print_version();

    void print_header();

    void print_help();

    void print_license();

    void print_prompt();

    void handle_command(Command cmd);

    /**
     * @brief Runs the REPL loop.
     *
     * This function reads input from the input stream, processes it, and
     * writes output to the output stream. It continues until the user exits
     * the REPL.
     */
    void run_repl();

public:
    /**
     * @brief Runs the REPL with the specified input and output streams.
     *
     * This is a static convenience method that creates a Repl instance and
     * calls its run_repl() method.
     *
     * @param in The input stream to read from. Default is std::cin.
     * @param out The output stream to write to. Default is std::cout.
     */
    static void run(std::istream& in = std::cin, std::ostream& out = std::cout);
};

#endif // NICO_REPL_H
