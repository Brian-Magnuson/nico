#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "compiler/code_file.h"
#include "lexer/lexer.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: nico <source file>" << std::endl;
        return 64;
    }

    // Open the file.
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << argv[1] << std::endl;
        return 66;
    }

    // Read the file's path.
    std::filesystem::path path = argv[1];

    // Read the entire file.
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string src_code;
    src_code.resize(size);
    file.read(&src_code[0], size);

    std::shared_ptr<CodeFile> code_file = std::make_shared<CodeFile>(
        std::filesystem::absolute(path),
        std::move(src_code)
    );

    file.close();

    Lexer lexer;
    lexer.scan(code_file);

    return 0;
}
