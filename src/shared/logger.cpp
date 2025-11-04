#include "nico/shared/logger.h"

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
             << colorize::reset << (int)ec << " " << message << "\n";
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
        *out << colorize::cyan << "⤷ Note: " << colorize::reset << message
             << "\n  ";
        print_code_at_location(location, colorize::cyan);
    }
}

void Logger::log_note(std::string_view message) {
    if (printing_enabled) {
        *out << colorize::cyan << "⤷ Note: " << colorize::reset << message
             << "\n";
    }
}

} // namespace nico
