#ifndef NICO_LOGGER_H
#define NICO_LOGGER_H

#include <iostream>
#include <string>
#include <vector>

#include "../lexer/token.h"
#include "error_code.h"

/**
 * @brief A color to use for console output.
 */
enum class Color {
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White,
    Reset
};

/**
 * @brief Returns the escape sequence for coloring text in the terminal.
 *
 * If standard output is not a terminal or the current platform cannot be determined, this function returns an empty string.
 *
 * @param color The color to return the escape sequence for. Defaults to Color::Reset.
 * @return The escape sequence for the given color.
 */
std::string colorize(Color color = Color::Reset);

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
     * @brief Prints the line of code at the provided location and underlines the span of text indicated by the location.
     *
     * This function is used to print pretty info messages that show the location of errors and notes.
     * Usually, two lines are printed: the line of code where the error occurred and an underline indicating the span of text, both ending with a newline.
     *
     * @param location The location of the code to print.
     * @param underline_color The color to use for underlining the code. Defaults to Color::Red.
     */
    void print_code_at_location(const Location& location, Color underline_color = Color::Red);

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
     * @brief Logs an error message with a location.
     *
     * If printing is enabled, the error message will be printed to the output stream.
     * The error code will be added to the stored list of errors.
     *
     * @param ec The error code to log.
     * @param location The location of the error in the source code.
     * @param message The message to log with the error.
     */
    void log_error(Err ec, const Location& location, const std::string& message);
};

#endif // NICO_LOGGER_H
