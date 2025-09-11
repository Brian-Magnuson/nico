#include "lexer.h"

#include <cstdint>
#include <string>

#include "../common/utils.h"
#include "../logger/error_code.h"
#include "../logger/logger.h"

std::unordered_map<std::string_view, Tok> Lexer::keywords = {
    // Literals

    {"inf", Tok::Float},
    {"NaN", Tok::Float},
    {"true", Tok::Bool},
    {"false", Tok::Bool},

    // Keywords

    {"and", Tok::KwAnd},
    {"or", Tok::KwOr},
    {"not", Tok::KwNot},

    {"block", Tok::KwBlock},

    {"let", Tok::KwLet},
    {"var", Tok::KwVar},

    {"pass", Tok::KwPass},
    {"yield", Tok::KwYield},

    {"printout", Tok::KwPrintout},
};

bool Lexer::is_at_end() const {
    return current >= file->src_code.length();
}

std::shared_ptr<Token> Lexer::make_token(Tok tok_type, std::any literal) const {
    Location location(file, start, current - start, line);
    return std::make_shared<Token>(tok_type, location, literal);
}

void Lexer::add_token(Tok tok_type, std::any literal) {
    tokens.push_back(make_token(tok_type, literal));
}

char Lexer::peek(int lookahead) const {
    return current + lookahead < file->src_code.length()
               ? file->src_code[current + lookahead]
               : '\0';
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
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
               (c >= 'A' && c <= 'F');
    default:
        panic("Lexer::is_digit: Invalid base: " + std::to_string(base));
    }
}

bool Lexer::is_alpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
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
        }
        else if (c == '\t') {
            current_tabs++;
        }
        else if (c == '\r') {
            // Ignore.
        }
        else if (c == '\n') {
            current_spaces = 0;
            current_tabs = 0;
            newline = true;
            line++;
            start = current + 1;
        }
        else {
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
        Logger::inst().log_error(
            Err::MixedLeftSpacing,
            token->location,
            "Line contains both tabs and spaces."
        );
        return;
    }
    else if (current_spaces && left_spacing_type == '\t') {
        auto token = make_token(Tok::Unknown);
        Logger::inst().log_error(
            Err::InconsistentLeftSpacing,
            token->location,
            "Left spacing uses spaces when previous lines used tabs."
        );
        return;
    }
    else if (current_tabs && left_spacing_type == ' ') {
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
    }
    else {
        left_spacing_type = current_spaces ? ' ' : '\t';
    }

    // Handle indents.
    if (!tokens.empty() && tokens.back()->tok_type == Tok::Colon) {
        // Left spacing must be greater than previous indent.
        if (spacing_amount <= current_left_spacing) {
            Logger::inst().log_error(
                Err::MalformedIndent,
                tokens.back()->location,
                "Expected indent with left-spacing greater than " +
                    std::to_string(current_left_spacing) + "."

            );
            Logger::inst().log_note(
                make_token(Tok::Unknown)->location,
                "Next line only has left-spacing of " +
                    std::to_string(spacing_amount) +
                    ". If this is meant to be an empty block, add a `pass` "
                    "statement."
            );
        }
        // Change the colon token to an indent token.
        tokens.back()->tok_type = Tok::Indent;
        left_spacing_stack.push_back(current_left_spacing);
    }

    // Handle dedents.
    while (!left_spacing_stack.empty() &&
           spacing_amount <= left_spacing_stack.back()) {
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

void Lexer::numeric_literal(bool integer_only) {
    current--;
    std::string numeric_string;

    if (integer_only) {
        while (is_digit(peek())) {
            numeric_string += advance();
        }
        return add_token(Tok::Int, std::stoi(numeric_string));
    }

    uint8_t base = 10;
    bool has_dot = false;
    bool has_exp = false;

    if (peek() == '0') {
        if (peek(1) == 'b') {
            base = 2;
            advance();
            advance();
        }
        else if (peek(1) == 'o') {
            base = 8;
            advance();
            advance();
        }
        else if (peek(1) == 'x') {
            base = 16;
            advance();
            advance();
        }
    }

    while (is_digit(peek(), base, true)) {
        if (peek() == '_') {
            advance();
            continue;
        }
        numeric_string += advance();

        if (peek() == '.') {
            numeric_string += advance();
            // A dot can only appear once, before any exponent, only in base 10,
            // and only if the next character is a digit.
            if (has_dot || has_exp || base != 10 || !is_digit(peek())) {
                auto prev_start = start;
                start = current - 1;
                Logger::inst().log_error(
                    Err::UnexpectedDotInNumber,
                    make_token(Tok::Unknown)->location,
                    "Unexpected '.' in number."
                );
                start = prev_start;
                return;
            }
            has_dot = true;
        }

        if (base != 16 && (peek() == 'e' || peek() == 'E')) {
            numeric_string += advance();
            // If the next character is a '+' or '-', advance.
            if (peek() == '+' || peek() == '-') {
                numeric_string += advance();
            }
            // An exponent can only appear once, only in base 10, and only if
            // the next character is a digit.
            if (has_exp || base != 10 || !is_digit(peek())) {
                auto prev_start = start;
                start = current - 1;
                Logger::inst().log_error(
                    Err::UnexpectedExpInNumber,
                    make_token(Tok::Unknown)->location,
                    "Unexpected exponent in number."
                );
                start = prev_start;
                return;
            }
            has_exp = true;
        }
    }

    // If the number ends in an f, it is a float.
    if (peek() == 'f') {
        // Any base is allowed to end in an `f`, even when the number is already
        // considered a float. The exception is base 16 since `f` is a valid hex
        // digit and is considered part of the number.
        advance();
        has_dot = true; // Saving an extra byte.
    }

    // Numbers cannot be followed by alphanumeric characters.
    if (is_digit(peek(), 16)) {
        auto prev_start = start;
        start = current;
        Logger::inst().log_error(
            Err::DigitInWrongBase,
            make_token(Tok::Unknown)->location,
            "Digit not allowed in numbers of base " + std::to_string(base) + "."
        );
        start = prev_start;
        return;
    }
    else if (is_alpha(peek())) {
        auto prev_start = start;
        start = current;
        Logger::inst().log_error(
            Err::InvalidCharAfterNumber,
            make_token(Tok::Unknown)->location,
            "Number cannot be followed by an alphabetic character."
        );
        Logger::inst().log_note("Consider adding a space here.");
        start = prev_start;
        return;
    }
    else if (numeric_string.empty()) {
        // This can only happen if the first part of the number is a base
        // prefix.
        auto prev_start = start;
        start = current;
        Logger::inst().log_error(
            Err::UnexpectedEndOfNumber,
            make_token(Tok::Unknown)->location,
            "Expected digits in number after base prefix."
        );
        start = prev_start;
        return;
    }

    if (has_dot || has_exp) {
        double value;
        try {
            value = std::stod(numeric_string);
        }
        catch (...) {
            panic(
                "Lexer::numeric_literal: std::stod failed to parse `" +
                numeric_string + "`"
            );
        }
        add_token(Tok::Float, value);
    }
    else {
        int32_t value;
        try {
            value =
                static_cast<int32_t>(std::stoll(numeric_string, nullptr, base));
        }
        catch (...) {
            panic(
                "Lexer::numeric_literal: std::stoll failed to parse `" +
                numeric_string + "`"
            );
        }
        add_token(Tok::Int, value);
    }
}

void Lexer::str_literal() {
    std::string str_content;
    while (peek() != '"' && !is_at_end()) {
        // A normal str literal cannot span multiple lines
        if (peek() == '\n') {
            Logger::inst().log_error(
                Err::UnterminatedStr,
                make_token(Tok::Unknown)->location,
                "Unterminated string."
            );
            add_token(Tok::Str);
            return;
        }

        // Handle escape sequences.
        if (peek() == '\\') {
            advance();
            switch (advance()) {
            case 'n':
                str_content += '\n';
                break;
            case 'r':
                str_content += '\r';
                break;
            case 't':
                str_content += '\t';
                break;
            case 'b':
                str_content += '\b';
                break;
            case 'f':
                str_content += '\f';
                break;
            case '0':
                str_content += '\0';
                break;
            case '\\':
                str_content += '\\';
                break;
            case '"':
                str_content += '"';
                break;
            case '\'':
                str_content += '\'';
                break;
            case '%':
                str_content += '%';
                break;
            case '{':
                str_content += '{';
                break;
            default: {
                auto prev_start = start;
                start = current - 1;
                Logger::inst().log_error(
                    Err::InvalidEscSeq,
                    make_token(Tok::Unknown)->location,
                    "Invalid escape sequence."
                );
                start = prev_start;
            }
            }
        }
        else {
            str_content += advance();
        }
    }

    if (is_at_end()) {
        Logger::inst().log_error(
            Err::UnterminatedStr,
            make_token(Tok::Unknown)->location,
            "Unterminated string."
        );
        return;
    }

    advance(); // Consume the closing quote.

    add_token(Tok::Str, str_content);
}

void Lexer::multi_line_comment() {
    unsigned open_count = 1;
    auto opening_token = make_token(Tok::SlashStar);
    while (open_count) {
        if (is_at_end()) {
            Logger::inst().log_error(
                Err::UnclosedComment,
                opening_token->location,
                "Unclosed multi-line comment."
            );

            auto prev_start = start;
            start = current;
            std::string msg = "Consider adding `";
            for (unsigned i = 0; i < open_count; i++)
                msg += "*/";
            msg += "` here.";
            Logger::inst().log_note(make_token(Tok::Unknown)->location, msg);
            start = prev_start;
            return;
        }

        if (peek() == '/' && peek(1) == '*') {
            open_count++;
            advance();
        }
        else if (peek() == '*' && peek(1) == '/') {
            open_count--;
            advance();
        }

        if (peek() == '\n') {
            line++;
        }

        advance();
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
                "Expected '" + std::string(1, grouping_token_stack.back()) +
                    "' before ')'."
            );
        }
        else {
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
                "Expected '" + std::string(1, grouping_token_stack.back()) +
                    "' before '}'."
            );
        }
        else {
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
                "Expected '" + std::string(1, grouping_token_stack.back()) +
                    "' before ']'."
            );
        }
        else {
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
    case ':':
        add_token(match(':') ? Tok::ColonColon : Tok::Colon);
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
            Logger::inst().log_error(
                Err::ClosingUnopenedComment,
                make_token(Tok::Unknown)->location,
                "Found '*/' without '/*'."
            );
        else
            add_token(Tok::Star);
        break;
    case '/':
        if (match('='))
            add_token(Tok::SlashEq);
        else if (match('/'))
            // Single-line comment.
            while (peek() != '\n' && !is_at_end())
                advance();
        else if (match('*'))
            // Multi-line comment.
            multi_line_comment();
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
    case '"':
        str_literal();
        break;
    default:
        if (is_alpha(c)) {
            identifier();
        }
        else if (is_digit(c)) {
            if (tokens.size() > 0 && tokens.back()->tok_type == Tok::Dot)
                // If the previous token was a dot, scan integer only.
                numeric_literal(true);
            else
                numeric_literal(false);
        }
        else {
            auto token = make_token(Tok::Unknown);
            Logger::inst().log_error(
                Err::UnexpectedChar,
                token->location,
                "Unexpected character."
            );
        }
    }
}

void Lexer::reset() {
    file = nullptr;
    tokens.clear();
    start = 0;
    current = 0;
}

std::vector<std::shared_ptr<Token>>&
Lexer::scan(const std::shared_ptr<CodeFile>& file) {
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
            "Expected '" + std::string(1, grouping_token_stack.back()) +
                "' before end of file."
        );
    }
    tokens.push_back(eof_token);

    return tokens;
}
