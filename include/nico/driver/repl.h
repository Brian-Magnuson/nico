#ifndef NICO_REPL_H
#define NICO_REPL_H

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "nico/backend/jit.h"
#include "nico/frontend/frontend.h"
#include "nico/shared/utils.h"

/**
 * @brief REPL (Read-Eval-Print Loop) class for handling user input and
 * commands.
 */
class Repl {
    /**
     * @brief Enumeration of REPL commands.
     *
     * REPL commands are handled by the REPL, not by the frontend or JIT.
     */
    enum class Command {
        // Display help information.
        Help,
        // Display version information.
        Version,
        // Display the license.
        License,
        // Discard the current input.
        Discard,
        // Reset the REPL state.
        Reset,
        // Exit the REPL.
        Exit
    };

    /**
     * @brief A mapping of command strings to Command enum values.
     *
     * All command strings consist of a colon immediately followed by the
     * command name, with no spaces.
     *
     * The user must type the command string exactly, with no other input, to
     * invoke the command. This is an intentional design choice to prevent the
     * frontend from bearing any responsibility for REPL commands.
     *
     * However, we do map multiple command strings to the same command for some
     * ease-of-use.
     */
    static const std::unordered_map<std::string, Command> commands;

    // The input stream (usually std::cin).
    std::istream* const in = &std::cin;
    // The output stream (usually std::cout).
    std::ostream* const out = &std::cout;
    // The frontend instance for compiling code.
    Frontend frontend;
    // The JIT instance for executing compiled code.
    std::unique_ptr<IJit> jit = std::make_unique<SimpleJit>();
    // The current input buffer.
    std::string input;
    // Whether the REPL is in "continue mode" (i.e., waiting for more input to
    // complete a statement).
    bool continue_mode = false;
    // Whether the REPL should proceed with caution (e.g., when state is
    // possibly corrupted).
    bool use_caution = false;

    Repl(std::istream& in = std::cin, std::ostream& out = std::cout)
        : in(&in), out(&out) {}

    /**
     * @brief Discards the current input buffer.
     *
     * The input buffer is cleared and the REPL exits continue mode.
     *
     * When an input causes an error, the frontend issues a request to have the
     * input discarded. However, some inputs may be partially processed before
     * the error is detected, resulting in the frontend context being modified.
     * In such cases, we set `with_warning` to true to inform the user that the
     * REPL state may have been altered and to proceed with caution.
     *
     * @param with_warning If true, a warning message is printed to inform the
     * user that the REPL state may have been modified. Default is false.
     */
    void discard(bool with_warning = false);

    /**
     * @brief Resets the REPL state, clearing all variables and definitions.
     *
     * This function clears the input buffer, resets the frontend and JIT
     * instances, and exits continue mode. It also clears any cautionary state.
     */
    void reset();

    /**
     * @brief Prints the REPL version information.
     */
    void print_version();

    /**
     * @brief Prints the REPL header information.
     *
     * This includes the REPL name, version, copyright, and basic usage
     * instructions.
     *
     * This is typically printed first when the REPL starts.
     */
    void print_header();

    /**
     * @brief Prints the REPL help information.
     *
     * This includes a list of available commands and their descriptions.
     */
    void print_help();

    /**
     * @brief Prints the contents of the LICENSE file to the output stream.
     *
     * If the LICENSE file cannot be found or read, an error message is printed
     * instead.
     */
    void print_license();

    /**
     * @brief Prints the REPL prompt to the output stream.
     *
     * The normal prompt is `>>`.
     * In terminals, it is colored green if the REPL is in normal mode, and
     * yellow if it is in caution mode.
     *
     * The continue prompt is `..`.
     * In terminals, it is colored gray.
     */
    void print_prompt();

    /**
     * @brief Handles a REPL command.
     *
     * This function executes the specified command, which may involve
     * modifying the REPL state or printing information to the output stream.
     *
     * @param cmd The command to handle.
     */
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
