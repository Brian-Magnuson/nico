#ifndef NICO_CORE_AOT_RUNNER_H
#define NICO_CORE_AOT_RUNNER_H

#include <filesystem>
#include <string_view>

namespace nico {

/**
 * @brief An AOT runner that compiles a source file to an object file.
 *
 * If the soruce file identified by `file_name` cannot be opened, the program
 * will exit with code 66. If the compilation fails, the program will exit with
 * code 1. If the compilation succeeds, the object file will be written to
 * `target_destination`. If the object file cannot be written, the program will
 * exit with code 1.
 *
 * @param file_name The name of the source file to compile.
 * @param target_destination The path to the object file to write. If the file
 * already exists, it will be overwritten. If the file cannot be written, the
 * program will exit with code 1.
 */
void compile_and_output(
    std::string_view file_name, std::filesystem::path target_destination
);

} // namespace nico

#endif // NICO_CORE_AOT_RUNNER_H
