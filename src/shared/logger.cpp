#include "nico/shared/logger.h"

#include <cctype>

#include "nico/shared/utils.h"

namespace nico {

void Logger::print_code_at_location(
    const Location& location, std::ostream& (*color_manip)(std::ostream& o)
) {
    const std::string& src_code = location.file->src_code;
    size_t start = location.start;
    size_t length = location.length;

    // The start of the line.
    size_t line_start = src_code.rfind('\n', start);
    if (line_start == std::string::npos) {
        line_start = 0;
    }
    else {
        line_start += 1; // Move past the newline character.
    }

    // The end of the line.
    size_t line_end = src_code.find('\n', start + length);
    if (line_end == std::string::npos) {
        line_end = src_code.length();
    }

    std::string line = src_code.substr(line_start, line_end - line_start);

    *out << location.to_string() << "\n";

    *out << std::setw(5) << location.line << " | " << line << "\n";

    *out << std::string(start - line_start + 8, ' ') << color_manip << "^";
    if (location.length > 1)
        *out << std::string(location.length - 1, '~');

    *out << colorize::reset << "\n";

    /*
    Example output:
        1 | let x = 5
            ^~~
    */
}

void Logger::print_message_with_breaks(
    std::string_view message, size_t indent
) {
    int terminal_width = get_terminal_width();
    if (terminal_width < 1) {
        // Not a terminal or unable to determine; print without breaks.
        *out << message << "\n";
        return;
    }
    if (terminal_width > 80) {
        // Cap at 80 for readability.
        terminal_width = 80;
    }
    else if (terminal_width < 40) {
        // Terminal too narrow; print without indents.
        indent = 0;
    }
    if (indent >= static_cast<size_t>(terminal_width)) {
        // Indent too large; adjust to fit.
        indent = static_cast<size_t>(terminal_width) - 1;
    }
    size_t max_length = static_cast<size_t>(terminal_width) - indent;
    auto lines = break_message(message, max_length);
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i == 0) {
            *out << lines[i] << "\n";
        }
        else {
            *out << std::string(indent, ' ') << lines[i] << "\n";
        }
    }
}

void Logger::reset() {
    out = &std::cerr;
    errors.clear();
    printing_enabled = true;
}

void Logger::log_error(
    Err ec, const Location& location, std::string_view message
) {
    errors.push_back(ec);
    if (printing_enabled) {
        *out << colorize::red << "Error " << errors.size() << ": "
             << colorize::reset << (int)ec << " ";
        // 14 to 16 characters before the error message.
        // 80 - 15 = 65 characters per line for code display.
        print_message_with_breaks(message, 15);
        print_code_at_location(location);
    }
}

void Logger::log_error(Err ec, std::string_view message) {
    errors.push_back(ec);
    if (printing_enabled) {
        *out << colorize::red << "Error " << errors.size() << ": "
             << colorize::reset << (int)ec << " " << message << "\n";
    }
}

void Logger::log_note(const Location& location, std::string_view message) {
    if (printing_enabled) {
        *out << colorize::cyan << "⤷ Note: " << colorize::reset;
        print_message_with_breaks(message, 8);
        print_code_at_location(location, colorize::cyan);
    }
}

void Logger::log_note(std::string_view message) {
    if (printing_enabled) {
        *out << colorize::cyan << "⤷ Note: " << colorize::reset;
        print_message_with_breaks(message, 8);
    }
}

} // namespace nico
