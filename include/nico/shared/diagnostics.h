#ifndef NICO_DIAGNOSTICS_H
#define NICO_DIAGNOSTICS_H

#include <iostream>
#include <string_view>
#include <vector>

#include "nico/shared/colorize.h"
#include "nico/shared/error_code.h"
#include "nico/shared/token.h"
#include "nico/shared/utils.h"

namespace nico {

/**
 * @brief Diagnostics singleton for reporting errors and messages.
 */
class Diagnostics {
    // A pointer to the output stream to log errors to.
    std::ostream* out = &std::cerr;
    // A list of the errors that have been reported.
    std::vector<Err> errors;
    // A boolean to determine if the error reported should print to the ostream.
    bool printing_enabled = true;

    Diagnostics() = default;
    Diagnostics(const Diagnostics&) = delete;
    Diagnostics& operator=(const Diagnostics&) = delete;

    /**
     * @brief Prints the line of code at the provided location and underlines
     * the span of text indicated by the location.
     *
     * This function is used to print pretty info messages that show the
     * location of errors and notes. Usually, two lines are printed: the line of
     * code where the error occurred and an underline indicating the span of
     * text, both ending with a newline.
     *
     * @param location The location of the code to print.
     * @param color_manip The color manipulator to use for the underline.
     */
    void print_code_at_location(
        const Location& location,
        std::ostream& (*color_manip)(std::ostream& o) = colorize::red
    );

    /**
     * @brief Prints a message with line breaks at a specified indentation
     * level.
     *
     * Indents are applied to each line after the first line.
     *
     * The line breaks will be based on a terminal width of 80 characters.
     * If stdout is known not to be a terminal or if the terminal width cannot
     * be determined, then no line breaks will be applied.
     *
     * If the terminal width is less than 40 characters, indents will not be
     * applied.
     *
     * @param message The message to print.
     * @param indent The number of spaces to indent each line after the first.
     * Default is 0.
     */
    void print_message_with_breaks(std::string_view message, size_t indent = 0);

public:
    /**
     * @brief Get the instance of the Diagnostics singleton.
     *
     * If the instance does not exist, it will be created.
     *
     * @return A reference to the Diagnostics singleton instance.
     */
    static Diagnostics& inst() {
        static Diagnostics instance;
        return instance;
    }

    /**
     * @brief Sets the reporter to enable or disable printing.
     *
     * When printing is enabled, error messages will be printed to the output
     * stream.
     *
     * @param enabled True to enable printing, false to disable printing.
     */
    void set_printing_enabled(bool enabled) { printing_enabled = enabled; }

    /**
     * @brief Resets the reporter to its default state.
     *
     * The output stream is reset to std::cerr, the list of errors is cleared,
     * and printing is enabled.
     */
    void reset();

    /**
     * @brief Reports an error message with a location.
     *
     * If printing is enabled, the error message will be printed to the output
     * stream. The error code will be added to the stored list of errors.
     *
     * @param ec The error code to report.
     * @param location The location of the error in the source code.
     * @param message The message to report with the error.
     */
    void emit_error(Err ec, const Location& location, std::string_view message);

    /**
     * @brief Reports an error message with a location pointer.
     *
     * If printing is enabled, the error message will be printed to the output
     * stream. The error code will be added to the stored list of errors.
     *
     * @param ec The error code to report.
     * @param location The location of the error in the source code.
     * @param message The message to report with the error.
     *
     * @warning If `location` is `nullptr`, this function will panic.
     */
    void
    emit_error(Err ec, const Location* location, std::string_view message) {
        if (location == nullptr)
            panic("Diagnostics::emit_error was given a null Location pointer");
        emit_error(ec, *location, message);
    }

    /**
     * @brief Reports an error message without a location.
     *
     * If printing is enabled, the error message will be printed to the output
     * stream. The error code will be added to the stored list of errors.
     *
     * @param ec The error code to report.
     * @param message The message to report with the error.
     */
    void emit_error(Err ec, std::string_view message);

    /**
     * @brief Reports a note message with a location.
     *
     * If printing is enabled, the note message will be printed to the output
     * stream. Otherwise, this function does nothing.
     *
     * @param location The location of the note in the source code.
     * @param message The message to report with the note.
     */
    void emit_note(const Location& location, std::string_view message);

    /**
     * @brief Reports a note message with a location pointer.
     *
     * If printing is enabled, the note message will be printed to the output
     * stream. Otherwise, this function does nothing.
     *
     * @param location The location of the note in the source code.
     * @param message The message to report with the note.
     *
     * @warning If `location` is `nullptr`, this function will panic.
     */
    void emit_note(const Location* location, std::string_view message) {
        if (location == nullptr)
            panic("Diagnostics::emit_note was given a null Location pointer");
        emit_note(*location, message);
    }

    /**
     * @brief Reports a note message without a location.
     *
     * If printing is enabled, the note message will be printed to the output
     * stream. Otherwise, this function does nothing.
     *
     * @param message The message to report with the note.
     */
    void emit_note(std::string_view message);

    /**
     * @brief Gets the errors that have been reported.
     * @return A read-only reference to the list of errors that have been
     * reported.
     */
    const std::vector<Err>& get_errors() const { return errors; }
};

} // namespace nico

#endif // NICO_DIAGNOSTICS_H
