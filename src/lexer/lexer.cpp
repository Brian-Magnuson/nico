#include "lexer.h"

#include <cstdlib>
#include <iostream>
#include <string>

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

char Lexer::peek() const {
    if (is_at_end())
        return '\0';
    return file->src_code[current];
}

char Lexer::advance() {
    if (is_at_end())
        return '\0';
    current++;
    return file->src_code[current - 1];
}

bool Lexer::match(char expected) {
    if (is_at_end())
        return false;
    if (file->src_code[current] != expected)
        return false;
    current++;
    return true;
}

bool Lexer::is_digit(char c, int base) const {
    switch (base) {
    case 2:
        return c == '0' || c == '1';
    case 8:
        return c >= '0' && c <= '7';
    case 10:
        return c >= '0' && c <= '9';
    case 16:
        return (c >= '0' && c <= '9') ||
               (c >= 'a' && c <= 'f') ||
               (c >= 'A' && c <= 'F');
    default:
        std::cerr << "Lexer::is_digit: Invalid base: " << base << std::endl;
        std::abort();
    }
}

bool Lexer::is_alpha(char c) const {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

bool Lexer::is_alpha_numeric(char c) const {
    return is_alpha(c) || is_digit(c);
}

void Lexer::identifier() {
    while (is_alpha_numeric(peek())) {
        advance();
    }
    auto token = make_token(Tok::Ident);
    std::string_view text = token->lexeme;

    if (text == "true" || text == "false") {
        token->tok_type = Tok::Bool;
    } else if (text == "NaN" || text == "inf") {
        token->tok_type = Tok::Float;
    }

    tokens.push_back(token);
}

void Lexer::scan_token() {
    char c = advance();
    switch (c) {
    case '(':
        add_token(Tok::LParen);
        break;
    case ')':
        add_token(Tok::RParen);
        break;
    case '{':
        add_token(Tok::LBrace);
        break;
    case '}':
        add_token(Tok::RBrace);
        break;
    case '[':
        add_token(Tok::LSquare);
        break;
    case ']':
        add_token(Tok::RSquare);
        break;
    case ',':
        add_token(Tok::Comma);
        break;
    case ';':
        add_token(Tok::Semicolon);
        break;
    case '+':
        add_token(match('=') ? Tok::PlusEq : Tok::Plus);
        break;
    case '-':
        if (match('='))
            add_token(Tok::MinusEq);
        else if (match('>'))
            add_token(Tok::Arrow);
        else
            add_token(Tok::Minus);
        break;
    case '*':
        if (match('='))
            add_token(Tok::StarEq);
        else if (match('/'))
            add_token(Tok::StarSlash);
        else
            add_token(Tok::Star);
        break;
    case '/':
        if (match('='))
            add_token(Tok::SlashEq);
        else if (match('/'))
            add_token(Tok::SlashSlash);
        else if (match('*'))
            add_token(Tok::SlashStar);
        else
            add_token(Tok::Slash);
        break;
    case '%':
        add_token(match('=') ? Tok::PercentEq : Tok::Percent);
        break;
    case '^':
        add_token(match('=') ? Tok::CaretEq : Tok::Caret);
        break;
    case '&':
        add_token(match('=') ? Tok::AmpEq : Tok::Amp);
        break;
    case '|':
        add_token(match('=') ? Tok::BarEq : Tok::Bar);
        break;
    case '!':
        add_token(match('=') ? Tok::BangEq : Tok::Bang);
        break;
    case '=':
        if (match('='))
            add_token(Tok::EqEq);
        else
            add_token(Tok::Eq);
        break;
    case '>':
        add_token(match('=') ? Tok::GtEq : Tok::Gt);
        break;
    case '<':
        add_token(match('=') ? Tok::LtEq : Tok::Lt);
        break;
    case '.':
        add_token(Tok::Dot);
        break;
    default:
        if (is_alpha(c)) {
            identifier();
        } else {
            auto token = make_token(Tok::Unknown);
            Logger::inst().log_error(Err::UnexpectedChar, token->location, "Unexpected character.");
        }
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
