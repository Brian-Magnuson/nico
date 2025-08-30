#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "checker/global_checker.h"
#include "checker/local_checker.h"
#include "codegen/code_generator.h"
#include "common/code_file.h"
#include "compiler/jit.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "logger/logger.h"
#include "parser/parser.h"

/**
 * @brief Checks if there are any errors logged in the Logger singleton and
 * exits the program if there are.
 *
 * This function is used to ensure that the compilation process does not
 * continue if there are any errors logged. If there are errors, it prints a
 * message to std::cerr and exits with a non-zero status code.
 */
void exit_if_errors() {
    const auto& errors = Logger::inst().get_errors();
    if (!errors.empty()) {
        std::cerr << "Compilation failed; exiting...";
        std::exit(1);
    }
}

int main(int argc, char** argv) {
    // if (argc != 2) {
    //     std::cout << "Usage: nico <source file>" << std::endl;
    //     return 64;
    // }

    // // Open the file.
    // std::ifstream file(argv[1]);
    // if (!file.is_open()) {
    //     std::cerr << "Could not open file: " << argv[1] << std::endl;
    //     return 66;
    // }

    // // Read the file's path.
    // std::filesystem::path path = argv[1];

    // // Read the entire file.
    // file.seekg(0, std::ios::end);
    // size_t size = file.tellg();
    // file.seekg(0, std::ios::beg);

    // std::string src_code;
    // src_code.resize(size);
    // file.read(&src_code[0], size);

    // std::shared_ptr<CodeFile> code_file = std::make_shared<CodeFile>(
    //     std::filesystem::absolute(path),
    //     std::move(src_code)
    // );

    // file.close();

    // Lexer lexer;
    // auto tokens = lexer.scan(code_file);
    // exit_if_errors();

    // Parser parser;
    // auto ast = parser.parse(std::move(tokens));
    // exit_if_errors();

    // std::shared_ptr<SymbolTree> symbol_tree = std::make_shared<SymbolTree>();

    // GlobalChecker global_checker(symbol_tree);
    // global_checker.check(ast);
    // exit_if_errors();

    // LocalChecker local_checker(symbol_tree);
    // local_checker.check(ast);
    // exit_if_errors();

    CodeGenerator codegen;
    codegen.generate({}, false);
    codegen.generate_main();
    exit_if_errors();

    // Eject the generated LLVM module and context.
    auto output = codegen.eject();

    std::unique_ptr<IJit> jit = std::make_unique<SimpleJit>();
    auto err =
        jit->add_module(std::move(output.module), std::move(output.context));
    exit_if_errors();

    auto result = jit->run_main(0, nullptr);
    exit_if_errors();

    return 0;
}
