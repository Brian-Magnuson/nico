#include "logger.h"

std::string colorize(Color color) {
// Include the appropriate headers based on the target OS.
#if defined(_WIN32) || defined(_WIN64)
// Windows.
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
// Unix-like systems.
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#else
// Fallback if neither header is available.
#define ISATTY(x) (0)          // Always return false, indicating not a terminal.
#define FILENO(x) ((void)0, 0) // Dummy to avoid unused variable warning.
#endif

    // Check if the standard output is a terminal.
    if (!ISATTY(FILENO(stdout))) {
        return "";
    }

    switch (color) {
    case Color::Red:
        return "\033[31m";
    case Color::Green:
        return "\033[32m";
    case Color::Yellow:
        return "\033[33m";
    case Color::Blue:
        return "\033[34m";
    case Color::Magenta:
        return "\033[35m";
    case Color::Cyan:
        return "\033[36m";
    case Color::White:
        return "\033[37m";
    case Color::Reset:
        return "\033[0m";
    default:
        return "\033[0m";
    }

#undef ISATTY
#undef FILENO
}

void Logger::print_code_at_location(const Location& location, Color underline_color) {
    std::string& src_code = location.file->src_code;
    size_t start = location.start;
    size_t length = location.length;

    // The start of the line.
    size_t line_start = src_code.rfind('\n', start);
    if (line_start == std::string::npos) {
        line_start = 0;
    } else {
        line_start += 1; // Move past the newline character.
    }

    // The end of the line.
    size_t line_end = src_code.find('\n', start + length);
    if (line_end == std::string::npos) {
        line_end = src_code.length();
    }

    std::string line = src_code.substr(line_start, line_end - line_start);

    *out << std::setw(5) << location.line << " | "
         << line << "\n";

    *out << std::string(start - line_start + 8, ' ') << colorize(underline_color)
         << "^";
    if (location.length > 1)
        *out << std::string(location.length - 1, '~');

    *out << colorize(Color::Reset) << "\n";

    /*
    Example output:
        1 | let x = 5
            ^~~
    */
}
