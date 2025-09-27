#ifndef NICO_CODE_FILE_H
#define NICO_CODE_FILE_H

#include <filesystem>
#include <optional>
#include <string>

/**
 * @brief A struct to hold the path and source code of a file.
 */
struct CodeFile {
    // The location of the file. If the code came from a file, this should be
    // the absolute path.
    const std::string path_string;
    // The source code from the file.
    const std::string src_code;

    /**
     * @brief Construct a new CodeFile object.
     *
     * A CodeFile is a wrapper for a source code string and a path string.
     *
     * @param src_code (Requires move) The source code from the file.
     * @param path_string The location where the file was read from. If the code
     * came from a file, this should be the absolute path.
     */
    CodeFile(const std::string&& src_code, const std::string& path_string)
        : path_string(path_string), src_code(std::move(src_code)) {}
};

#endif // NICO_CODE_FILE_H
