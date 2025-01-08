#include "lexer.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include "../logger/error_code.h"
#include "../logger/logger.h"

std::unordered_map<std::string_view, Tok> Lexer::keywords = {
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
    char c = file->src_code[current - 1];
    return c;
}

bool Lexer::match(char expected) {
    if (is_at_end())
        return false;
    if (file->src_code[current] != expected)
        return false;
    current++;
    return true;
}

bool Lexer::is_whitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool Lexer::is_digit(char c, int base, bool allow_underscore) const {
    if (allow_underscore && c == '_') {
        return true;
    }
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

    unsigned current_spaces = 0;
    unsigned current_tabs = 0;
    bool newline = current == 0;

    // Consume all whitespace.
    while (true) {
        char c = peek();
        if (c == ' ') {
            current_spaces++;
        } else if (c == '\t') {
            current_tabs++;
        } else if (c == '\r') {
            // Ignore.
        } else if (c == '\n') {
            current_spaces = 0;
            current_tabs = 0;
            newline = true;
            line++;
            start = current + 1;
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
    } else if (current_spaces && left_spacing_type == '\t') {
        auto token = make_token(Tok::Unknown);
        Logger::inst().log_error(
            Err::InconsistentLeftSpacing,
            token->location,
            "Left spacing uses spaces when previous lines used tabs."
        );
        return;
    } else if (current_tabs && left_spacing_type == ' ') {
        auto token = make_token(Tok::Unknown);
        Logger::inst().log_error(
            Err::InconsistentLeftSpacing,
            token->location,
            "Left spacing uses tabs when previous lines used spaces."
        );
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
        if (spacing_amount <= current_left_spacing) {
            Logger::inst().log_error(
                Err::MalformedIndent,
                tokens.back()->location,
                "Expected indent with left-spacing greater than " + std::to_string(current_left_spacing) + "."

            );
            Logger::inst().log_note(
                make_token(Tok::Unknown)->location,
                "Next line only has left-spacing of " + std::to_string(spacing_amount) + "."
            );
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

    auto it = keywords.find(text);
    if (it != keywords.end()) {
        token->tok_type = it->second;
    }

    tokens.push_back(token);
}

void Lexer::number() {
    current--;
    uint8_t base = 10;
    bool has_dot = false;
    bool has_exp = false;

    if (peek() == '0') {
        advance();
        if (peek() == 'b' && is_digit(peek(1), 2)) {
            base = 2;
            advance();
        } else if (peek() == 'o' && is_digit(peek(1), 8)) {
            base = 8;
            advance();
        } else if (peek() == 'x' && is_digit(peek(1), 16)) {
            base = 16;
            advance();
        }
    }

    while (is_digit(peek(), base, true)) {
        advance();

        if (peek() == '.') {
            advance();
            // A dot can only appear once, before any exponent, only in base 10, and only if the next character is a digit.
            if (has_dot || has_exp || base != 10 || !is_digit(peek(1))) {
                Logger::inst().log_error(Err::UnexpectedDotInNumber, make_token(Tok::Unknown)->location, "Unexpected '.' in number.");
                return;
            }
            has_dot = true;
        }

        if (base != 16 && (peek() == 'e' || peek() == 'E')) {
            advance();
            // If the next character is a '+' or '-', advance.
            if (peek() == '+' || peek() == '-') {
                advance();
            }
            // An exponent can only appear once, only in base 10, and only if the next character is a digit.
            if (has_exp || base != 10 || !is_digit(peek())) {
                Logger::inst().log_error(Err::UnexpectedExpInNumber, make_token(Tok::Unknown)->location, "Unexpected exponent in number.");
                return;
            }
            has_exp = true;
        }
    }

    // If the number ends in an f, it is a float.
    if (peek() == 'f') {
        // Any base is allowed to end in an `f`, even when the number is already considered a float.
        // The exception is base 16 since `f` is a valid hex digit and is considered part of the number.
        advance();
        has_dot = true; // Saving an extra byte.
    }

    if (has_dot || has_exp) {
        add_token(Tok::Float);
    } else {
        add_token(Tok::Int);
    }

    // Numbers cannot be followed by alphanumeric characters.
    if (is_alpha_numeric(peek())) {
        Logger::inst().log_error(Err::InvalidCharAfterNumber, make_token(Tok::Unknown)->location, "Number cannot be followed by an alphanumeric character.");
    }
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
                Err::UnclosedGrouping,
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
                Err::UnclosedGrouping,
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
                Err::UnclosedGrouping,
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
        } else if (is_digit(c)) {
            number();
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

    auto eof_token = make_token(Tok::Eof);

    if (!grouping_token_stack.empty()) {
        Logger::inst().log_error(
            Err::UnclosedGrouping,
            eof_token->location,
            "Expected '" + std::string(1, grouping_token_stack.back()) + "' before end of file."
        );
    }
    tokens.push_back(eof_token);

    return tokens;
}
