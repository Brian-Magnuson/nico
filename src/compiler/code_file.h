#ifndef NICO_CODE_FILE_H
#define NICO_CODE_FILE_H

#include <filesystem>

/**
 * @brief A struct to hold the path and source code of a file.
 */
struct CodeFile {
    // The absolute path to the file.
    const std::filesystem::path path;
    // The source code from the file.
    const std::string src_code;

    /**
     * @brief Construct a new CodeFile object.
     * @param path The path to the file.
     * @param src_code The source code from the file. Use `std::move` to avoid copying the string.
     */
    CodeFile(const std::filesystem::path& path, const std::string&& src_code)
        : path(path), src_code(std::move(src_code)) {}
};

#endif // NICO_CODE_FILE_H
