#ifndef NICO_LOGGER_H
#define NICO_LOGGER_H

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "../lexer/token.h"
#include "error_code.h"

/**
 * @brief A color to use for console output.
 */
enum class Color { Red, Green, Yellow, Blue, Magenta, Cyan, White, Reset };

/**
 * @brief Returns the escape sequence for coloring text in the terminal.
 *
 * If standard output is not a terminal or the current platform cannot be
 * determined, this function returns an empty string.
 *
 * @param color The color to return the escape sequence for. Defaults to
 * Color::Reset.
 * @return The escape sequence for the given color.
 */
std::string colorize(Color color = Color::Reset);

/**
 * @brief Logger singleton for logging errors and messages.
 */
class Logger {
    // A pointer to the output stream to log errors to.
    std::ostream* out = &std::cerr;
    // A list of the errors that have been logged.
    std::vector<Err> errors;
    // A boolean to determine if the error logger should print to the ostream.
    bool printing_enabled = true;

    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

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
     * @param underline_color The color to use for underlining the code.
     * Defaults to Color::Red.
     */
    void print_code_at_location(
        const Location& location, Color underline_color = Color::Red
    );

public:
    /**
     * @brief Get the instance of the Logger singleton.
     *
     * If the instance does not exist, it will be created.
     *
     * @return A reference to the Logger singleton instance.
     */
    static Logger& inst() {
        static Logger instance;
        return instance;
    }

    /**
     * @brief Sets the logger to enable or disable printing.
     *
     * When printing is enabled, error messages will be printed to the output
     * stream.
     *
     * @param enabled True to enable printing, false to disable printing.
     */
    void set_printing_enabled(bool enabled) { printing_enabled = enabled; }

    /**
     * @brief Resets the logger to its default state.
     *
     * The output stream is reset to std::cerr, the list of errors is cleared,
     * and printing is enabled.
     */
    void reset();

    /**
     * @brief Logs an error message with a location.
     *
     * If printing is enabled, the error message will be printed to the output
     * stream. The error code will be added to the stored list of errors.
     *
     * @param ec The error code to log.
     * @param location The location of the error in the source code.
     * @param message The message to log with the error.
     */
    void log_error(Err ec, const Location& location, std::string_view message);

    /**
     * @brief Logs an error message without a location.
     *
     * If printing is enabled, the error message will be printed to the output
     * stream. The error code will be added to the stored list of errors.
     *
     * @param ec The error code to log.
     * @param message The message to log with the error.
     */
    void log_error(Err ec, std::string_view message);

    /**
     * @brief Logs a note message with a location.
     *
     * If printing is enabled, the note message will be printed to the output
     * stream. Otherwise, this function does nothing.
     *
     * @param location The location of the note in the source code.
     * @param message The message to log with the note.
     */
    void log_note(const Location& location, std::string_view message);

    /**
     * @brief Logs a note message without a location.
     *
     * If printing is enabled, the note message will be printed to the output
     * stream. Otherwise, this function does nothing.
     *
     * @param message The message to log with the note.
     */
    void log_note(std::string_view message);

    /**
     * @brief Gets the errors that have been logged.
     * @return A read-only reference to the list of errors that have been
     * logged.
     */
    const std::vector<Err>& get_errors() const { return errors; }
};

#endif // NICO_LOGGER_H
