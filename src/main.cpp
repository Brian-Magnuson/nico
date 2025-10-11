#include <iostream>

#include "nico/driver/jit_runner.h"
#include "nico/driver/repl.h"

int main(int argc, char** argv) {
    if (argc > 2) {
        std::cout << "Usage: nico [source_file]" << std::endl;
        return 64;
    }

    if (argc == 2) {
        nico::compile_and_run(argv[1]);
    }
    else {
        nico::Repl::run();
    }

    return 0;
}
