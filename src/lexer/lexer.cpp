#include "lexer.h"

bool Lexer::is_at_end() const {
    return current >= file->src_code.length();
}

void Lexer::add_token(Tok tok_type) {
    Location location(file, start, current - start, line);
    tokens.push_back(std::make_shared<Token>(tok_type, location));
}

char Lexer::advance() {
    if (is_at_end())
        return '\0';
    current++;
    return file->src_code[current - 1];
}

void Lexer::scan_token() {
    char c = advance();
    switch (c) {
    case '(':
        add_token(Tok::LeftParen);
        break;
    case ')':
        add_token(Tok::RightParen);
        break;
    case '{':
        add_token(Tok::LeftBrace);
        break;
    case '}':
        add_token(Tok::RightBrace);
        break;
    case '[':
        add_token(Tok::LeftSquare);
        break;
    case ']':
        add_token(Tok::RightSquare);
        break;
    default:
        add_token(Tok::Unknown);
        break;
    }
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

    while (!is_at_end()) {
        start = current;
        scan_token();
    }

    add_token(Tok::Eof);
}
