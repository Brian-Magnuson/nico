#include "lexer.h"

#include <cstdlib>
#include <iostream>
#include <string>

#include "../logger/error_code.h"
#include "../logger/logger.h"

std::unordered_map<std::string, Tok> Lexer::keywords = {
    // Literals

    {"inf", Tok::Float},
    {"NaN", Tok::Float},
    {"true", Tok::Bool},
    {"false", Tok::Bool},

    // Keywords

    {"block", Tok::KwBlock},

    {"let", Tok::KwLet},
    {"var", Tok::KwVar},
};

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

char Lexer::peek(int lookahead) const {
    return current + lookahead < file->src_code.length() ? file->src_code[current + lookahead] : '\0';
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

void Lexer::consume_whitespace() {
    // Go back to the previous character.
    current--;

    // Consume all whitespace.
    unsigned current_spaces = 0;
    unsigned current_tabs = 0;
    bool newline = false;
    while (true) {
        char c = peek();
        if (c == ' ') {
            current_spaces++;
        } else if (c == '\t') {
            current_tabs++;
        } else if (c == '\r') {
            // Ignore.
        } else if (c == '\n') {
            line++;
            current_spaces = 0;
            current_tabs = 0;
            newline = true;
        } else {
            break;
        }
        advance();
    }

    // If within grouping tokens or not on a newline...
    if (!grouping_token_stack.empty() || !newline) {
        // Return early.
        return;
    }

    // If user tried to mix spacing...
    if (current_spaces > 0 && current_tabs > 0) {
        auto token = make_token(Tok::Unknown);
        Logger::inst().log_error(Err::MixedLeftSpacing, token->location, "Line contains both tabs and spaces.");
        return;
    } else if (
        (current_spaces && left_spacing_type == '\t') || (current_tabs && left_spacing_type == ' ')
    ) {
        auto token = make_token(Tok::Unknown);
        Logger::inst().log_error(Err::MixedLeftSpacing, token->location, "Left spacing is inconsistent with previous line.");
        return;
    }

    // Set spacing_amount to whichever isn't 0.
    unsigned spacing_amount = current_spaces | current_tabs;

    // Update left_spacing_type.
    if (spacing_amount == 0) {
        left_spacing_type = '\0';
    } else {
        left_spacing_type = current_spaces ? ' ' : '\t';
    }

    // Handle indents.
    if (!tokens.empty() && tokens.back()->tok_type == Tok::Colon) {
        // Left spacing must be greater than previous indent.
        if (spacing_amount <= (left_spacing_stack.empty() ? 0 : left_spacing_stack.back())) {
            Logger::inst().log_error(Err::MalformedIndent, tokens.back()->location, "Attempted to form indent with insufficient left-spacing.");
            return;
        }
        // Change the colon token to an indent token.
        tokens.back()->tok_type = Tok::Indent;
        left_spacing_stack.push_back(current_left_spacing);
    }

    // Handle dedents.
    while (!left_spacing_stack.empty() && spacing_amount <= left_spacing_stack.back()) {
        left_spacing_stack.pop_back();
        add_token(Tok::Dedent);
    }

    current_left_spacing = spacing_amount;
}

void Lexer::identifier() {
    while (is_alpha_numeric(peek())) {
        advance();
    }
    auto token = make_token(Tok::Identifier);
    std::string_view text = token->lexeme;

    auto it = keywords.find(std::string(text));
    if (it != keywords.end()) {
        token->tok_type = it->second;
    }

    tokens.push_back(token);
}

void Lexer::scan_token() {
    char c = advance();
    switch (c) {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
        consume_whitespace();
        break;
    case ':':
        add_token(Tok::Colon);
        break;
    case '(':
        add_token(Tok::LParen);
        grouping_token_stack.push_back(')');
        break;
    case '{':
        add_token(Tok::LBrace);
        grouping_token_stack.push_back('}');
        break;
    case '[':
        add_token(Tok::LSquare);
        grouping_token_stack.push_back(']');
        break;
    case ')': {
        auto t = make_token(Tok::RParen);
        if (grouping_token_stack.empty() || grouping_token_stack.back() != c) {
            Logger::inst().log_error(
                Err::InvalidGrouping,
                t->location,
                "Expected '" + std::string(1, grouping_token_stack.back()) + "' before ')'."
            );
        } else {
            grouping_token_stack.pop_back();
            tokens.push_back(t);
        }
        break;
    }
    case '}': {
        auto t = make_token(Tok::RBrace);
        if (grouping_token_stack.empty() || grouping_token_stack.back() != c) {
            Logger::inst().log_error(
                Err::InvalidGrouping,
                t->location,
                "Expected '" + std::string(1, grouping_token_stack.back()) + "' before '}'."
            );
        } else {
            grouping_token_stack.pop_back();
            tokens.push_back(t);
        }
        break;
    }
    case ']': {
        auto t = make_token(Tok::RSquare);
        if (grouping_token_stack.empty() || grouping_token_stack.back() != c) {
            Logger::inst().log_error(
                Err::InvalidGrouping,
                t->location,
                "Expected '" + std::string(1, grouping_token_stack.back()) + "' before ']'."
            );
        } else {
            grouping_token_stack.pop_back();
            tokens.push_back(t);
        }
        break;
    }
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
