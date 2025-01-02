#include "lexer.h"

bool Lexer::is_at_end() const {
    return current >= file->src_code.length();
}

void Lexer::reset() {
    file = nullptr;
    tokens.clear();
    start = 0;
    current = 0;
}

std::vector<std::shared_ptr<Token>> Lexer::scan(const std::shared_ptr<CodeFile>& file) {
    reset();
    this->file = file;

    // TODO: Continue implementing this function.
}
