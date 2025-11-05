#include "nico/shared/utils.h"

namespace nico {

std::vector<std::string_view>
break_message(std::string_view message, size_t max_length) {
    if (max_length < 10) {
        max_length = 10;
    }

    std::vector<std::string_view> broken_lines;

    // Split the message into lines based on '\n'.
    size_t start = 0;
    while (start < message.size()) {
        size_t newline_pos = message.find('\n', start);
        size_t line_end = (newline_pos == std::string_view::npos)
                              ? message.size()
                              : newline_pos;
        std::string_view line = message.substr(start, line_end - start);

        // Break the line into chunks based on max_length.
        while (line.size() > max_length) {
            size_t break_pos = line.rfind(' ', max_length);
            if (break_pos == std::string_view::npos || break_pos == 0) {
                // No space found, force break at max_length.
                break_pos = max_length;
                broken_lines.push_back(line.substr(0, break_pos));
                line = line.substr(
                    break_pos
                ); // Do not skip space (there is no space).
                continue;
            }
            broken_lines.push_back(line.substr(0, break_pos));
            line = line.substr(break_pos + 1); // Skip the space.
        }

        // Add the remaining part of the line.
        broken_lines.push_back(line);

        // Move to the next line.
        start = line_end + 1;
    }

    return broken_lines;
}

} // namespace nico
