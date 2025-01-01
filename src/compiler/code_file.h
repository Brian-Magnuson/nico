#ifndef NICO_CODE_FILE_H
#define NICO_CODE_FILE_H

#include <filesystem>

/**
 * @brief A struct to hold the path and source code of a file.
 */
struct CodeFile {
    // The absolute path to the file.
    std::filesystem::path path;
    // The source code from the file.
    std::string src_code;
};

#endif // NICO_CODE_FILE_H
