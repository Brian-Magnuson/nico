#ifndef NICO_CORE_CODE_FILE_H
#define NICO_CORE_CODE_FILE_H

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace nico {

/**
 * @brief A class to hold the path and source code of a file.
 */
class CodeFile {
public:
    // The location of the file. If the code came from a file, this should be
    // the absolute path.
    const std::string path_string;
    // The source code from the file.
    const std::string src_code;

protected:
    /**
     * @brief A private struct used to restrict access to constructors.
     */
    struct Private {
        explicit Private() = default;
    };

public:
    CodeFile(
        Private, const std::string& src_code, const std::string& path_string
    )
        : path_string(path_string), src_code(src_code) {}

    /**
     * @brief Constructs a new CodeFile from a string and a path.
     *
     * This function is for creating code files from strings to be accepted by
     * the frontend. It is useful for testing and for creating code files from
     * strings in memory. As such, the path does not need to refer to a real
     * file, but it should be a valid path string.
     *
     * @param src_code (Requires move) A string containing the source code.
     * @param path The code file's path. Does not need to refer to a real file.
     * @return std::shared_ptr<CodeFile>
     */
    static std::shared_ptr<CodeFile>
    from_string(std::string&& src_code, std::filesystem::path path) {
        return std::make_shared<CodeFile>(
            Private{},
            std::move(src_code),
            path.string()
        );
    }

    /**
     * @brief Attempts to read a file at the given path and create a CodeFile
     * from it.
     *
     * @param path The path to the file to read.
     * @return std::optional<std::shared_ptr<CodeFile>> A shared pointer to the
     * CodeFile if successful, or std::nullopt if the file could not be opened.
     */
    static std::optional<std::shared_ptr<CodeFile>>
    from_file(std::filesystem::path path) {
        // Open the file.
        std::ifstream file(path);
        if (!file.is_open()) {
            return std::nullopt;
        }

        // Take the size of the file.
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        // Read the entire file into a string.
        std::string src_code;
        src_code.resize(size);
        file.read(&src_code[0], size);

        std::shared_ptr<CodeFile> code_file = std::make_shared<CodeFile>(
            Private{},
            std::move(src_code),
            std::filesystem::absolute(path).string()
        );

        file.close();
        return code_file;
    }
};

} // namespace nico

#endif // NICO_CORE_CODE_FILE_H
