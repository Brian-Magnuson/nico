#include "frontend.h"

std::unique_ptr<Frontend::Context>&
Frontend::compile(const std::shared_ptr<CodeFile>& file, bool repl_mode) {
    // TODO: Incorporate the front end interface into everything.
    return context;
}
