#ifndef NICO_REPL_H
#define NICO_REPL_H

#include <iostream>
#include <string>

#include "nico/shared/utils.h"

class Repl {
    // The input stream (usually std::cin).
    std::istream* const in = &std::cin;
    // The output stream (usually std::cout).
    std::ostream* const out = &std::cout;
    // Whether the REPL should proceed with caution (e.g., when state is
    // possibly corrupted).
    bool use_caution = false;

    Repl(std::istream& in = std::cin, std::ostream& out = std::cout)
        : in(&in), out(&out) {}

    void print_version() { *out << project_version() << std::endl; }

    void print_header();

    void print_help();

    void print_license();

    void print_prompt();

    void print_continue_prompt();

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
