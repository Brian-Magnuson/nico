#include "lexer.h"

#include "../logger/error_code.h"
#include "../logger/logger.h"

bool Lexer::is_at_end() const {
    return current >= file->src_code.length();
}

std::shared_ptr<Token> Lexer::make_token(Tok tok_type) const {
    Location location(file, start, current - start, line);
    return std::make_shared<Token>(tok_type, location);
}

void Lexer::add_token(Tok tok_type) {
    tokens.push_back(make_token(tok_type));
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
        auto token = make_token(Tok::Unknown);
        Logger::inst().log_error(Err::UnexpectedChar, token->location, "Unexpected character.");
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
    return tokens;
}
