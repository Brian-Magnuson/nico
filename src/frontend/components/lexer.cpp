#include "nico/frontend/components/lexer.h"

#include <stdexcept>
#include <string>

#include "nico/shared/error_code.h"
#include "nico/shared/logger.h"
#include "nico/shared/status.h"
#include "nico/shared/utils.h"

namespace nico {

std::unordered_map<std::string_view, Tok> Lexer::keywords = {
    // Literals

    {"inf", Tok::FloatDefault},
    {"inf32", Tok::Float32},
    {"inf64", Tok::Float64},
    {"nan", Tok::FloatDefault},
    {"nan32", Tok::Float32},
    {"nan64", Tok::Float64},
    {"true", Tok::Bool},
    {"false", Tok::Bool},
    {"nullptr", Tok::Nullptr},

    // Keywords

    {"and", Tok::KwAnd},
    {"or", Tok::KwOr},
    {"not", Tok::KwNot},
    {"as", Tok::KwAs},
    {"is", Tok::KwIs},

    {"block", Tok::KwBlock},
    {"unsafe", Tok::KwUnsafe},
    {"if", Tok::KwIf},
    {"then", Tok::KwThen},
    {"else", Tok::KwElse},
    {"loop", Tok::KwLoop},
    {"while", Tok::KwWhile},
    {"do", Tok::KwDo},
    {"for", Tok::KwFor},
    {"of", Tok::KwOf},
    {"with", Tok::KwWith},
    {"sizeof", Tok::KwSizeof},
    {"alloc", Tok::KwAlloc},
    {"transmute", Tok::KwTransmute},

    {"typeof", Tok::KwTypeof},

    {"let", Tok::KwLet},
    {"var", Tok::KwVar},
    {"func", Tok::KwFunc},

    {"pass", Tok::KwPass},
    {"yield", Tok::KwYield},
    {"break", Tok::KwBreak},
    {"continue", Tok::KwContinue},
    {"return", Tok::KwReturn},
    {"dealloc", Tok::KwDealloc},

    {"printout", Tok::KwPrintout},
};

std::unordered_set<std::string_view> Lexer::reserved_words = {
    "module",
    "import",
    "public",
    "private",
    "using",
    "from",
    "async",
    "await",
    "overloadedfn"
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

bool Lexer::match_all(std::string_view expected) {
    if (current + expected.length() > file->src_code.length()) {
        return false;
    }
    for (size_t i = 0; i < expected.length(); i++) {
        if (file->src_code[current + i] != expected[i]) {
            return false;
        }
    }
    current += expected.length();
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

    // Here, we handle a unique edge case.
    // In most cases, we initialize `is_left_spacing` to false, and only set it
    // to true once we reach a newline character.
    // We set it to false because this function is almost never called at the
    // start of a line (only in the middle or at the end of a line).
    // The lone exception is when the first character of input is whitespace.
    // In this case, we want to treat the whitespace as left spacing.
    bool is_left_spacing = current == 0;

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
            is_left_spacing = true;
            line++;
            start = current + 1;
        }
        else {
            break;
        }
        advance();
        // If a line contains only whitespace characters, that line is
        // effectively ignored.
    }

    // If within grouping tokens or not on a newline...
    if (!grouping_token_stack.empty() || !is_left_spacing) {
        // This spacing is not left spacing.
        // Return early.
        return;
    }
    // Otherwise, the current spacing is left spacing.

    // If user tried to mix left spacing...
    if (current_spaces > 0 && current_tabs > 0) {
        auto token = make_token(Tok::Unknown);
        Logger::inst().log_error(
            Err::MixedLeftSpacing,
            token->location,
            "Line left spacing contains both tabs and spaces."
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

    // Set curr_line_left_spacing to whichever isn't 0.
    unsigned curr_line_left_spacing = current_spaces | current_tabs;

    // Update left_spacing_type.
    if (curr_line_left_spacing == 0) {
        left_spacing_type = '\0';
    }
    else {
        left_spacing_type = current_spaces ? ' ' : '\t';
    }

    // Handle indents.
    if (!tokens.empty() && tokens.back()->tok_type == Tok::Colon) {
        // If the last token was a colon...
        if (curr_line_left_spacing <= prev_line_left_spacing) {
            // If the current left spacing is not greater than the previous
            // line's spacing, log an error.
            Logger::inst().log_error(
                Err::MalformedIndent,
                tokens.back()->location,
                "Expected indent with left-spacing greater than " +
                    std::to_string(prev_line_left_spacing) + "."

            );
            Logger::inst().log_note(
                make_token(Tok::Unknown)->location,
                "Next line only has left-spacing of " +
                    std::to_string(curr_line_left_spacing) +
                    ". If this is meant to be an empty block, add a `pass` "
                    "statement."
            );
        }
        // Change the colon token to an indent token.
        tokens.back()->tok_type = Tok::Indent;
        left_spacing_stack.push_back(prev_line_left_spacing);
    }

    // Handle dedents.
    while (!left_spacing_stack.empty() &&
           curr_line_left_spacing <= left_spacing_stack.back()) {
        left_spacing_stack.pop_back();
        add_token(Tok::Dedent);
    }

    prev_line_left_spacing = curr_line_left_spacing;
}

void Lexer::identifier() {
    while (is_alpha_numeric(peek())) {
        advance();
    }
    auto token = make_token(Tok::Identifier);
    std::string_view text = token->lexeme;

    if (auto it = keywords.find(text); it != keywords.end()) {
        token->tok_type = it->second;
    }
    else if (reserved_words.find(text) != reserved_words.end()) {
        Logger::inst().log_error(
            Err::WordIsReserved,
            token->location,
            "'" + std::string(token->lexeme) +
                "' is a reserved word and cannot be used as an identifier."
        );
    }

    tokens.push_back(token);
}

void Lexer::tuple_index() {
    current--;
    std::string numeric_string;
    while (is_digit(peek())) {
        numeric_string += advance();
    }
    size_t num = 0;
    try {
        num = std::stoul(numeric_string);
    }
    catch (const std::out_of_range&) {
        Logger::inst().log_error(
            Err::TupleIndexOutOfRange,
            make_token(Tok::Unknown)->location,
            "Tuple index is too large."
        );
        return;
    }
    catch (...) {
        panic(
            "Lexer::tuple_index: Invalid argument when parsing "
            "tuple index."
        );
    }
    add_token(Tok::TupleIndex, num);
}

void Lexer::numeric_literal() {
    current--;
    uint8_t base = 10;
    bool has_dot = false;
    bool has_exp = false;
    bool has_digit = false;

    if (match_all("0b")) {
        base = 2;
    }
    else if (match_all("0o")) {
        base = 8;
    }
    else if (match_all("0x")) {
        base = 16;
    }

    while (is_digit(peek(), base, true)) {
        if (peek() == '_') {
            advance();
            continue;
        }
        advance();
        has_digit = true;

        if (peek() == '.') {
            advance();
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
            // numeric_string += advance();
            advance();
            // If the next character is a '+' or '-', advance.
            if (peek() == '+' || peek() == '-') {
                // numeric_string += advance();
                advance();
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
    if (!has_digit) {
        // This can only happen if the first part of the number is a base
        // prefix.
        auto prev_start = start;
        start = current;
        Logger::inst().log_error(
            Err::UnexpectedEndOfNumber,
            make_token(Tok::Unknown)->location,
            "Expected base " + std::to_string(base) + " digit."
        );
        start = prev_start;
        return;
    }

    add_token(Tok::Unknown);

    // Handle type suffixes.
    auto token = tokens.back();
    if (match_all("i8")) {
        token->tok_type = Tok::Int8;
    }
    else if (match_all("i16")) {
        token->tok_type = Tok::Int16;
    }
    else if (match_all("i32")) {
        token->tok_type = Tok::Int32;
    }
    else if (match_all("i64")) {
        token->tok_type = Tok::Int64;
    }
    else if (match_all("u8")) {
        token->tok_type = Tok::UInt8;
    }
    else if (match_all("u16")) {
        token->tok_type = Tok::UInt16;
    }
    else if (match_all("u32")) {
        token->tok_type = Tok::UInt32;
    }
    else if (match_all("u64")) {
        token->tok_type = Tok::UInt64;
    }
    else if (match_all("f32")) {
        token->tok_type = Tok::Float32;
    }
    else if (match_all("f64")) {
        token->tok_type = Tok::Float64;
    }
    else if (match('l') || match('L')) {
        token->tok_type = Tok::Int64;
    }
    else if (match('u') || match('U')) {
        token->tok_type = Tok::UInt32;
    }
    else if (match_all("ul") || match_all("UL")) {
        token->tok_type = Tok::UInt64;
    }
    else if (match('f') || match('F')) {
        token->tok_type = Tok::Float32;
    }
    else {
        if (has_dot || has_exp) {
            token->tok_type = Tok::FloatDefault;
        }
        else {
            token->tok_type = Tok::IntDefault;
        }
    }

    if (is_digit(peek(), 16)) {
        auto prev_start = start;
        start = current;
        Logger::inst().log_error(
            Err::DigitInWrongBase,
            make_token(Tok::Unknown)->location,
            "Digit not allowed in numbers of base " + std::to_string(base) + "."
        );
        start = prev_start;
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
            // If in REPL mode, request more input instead of erroring.
            if (repl_mode) {
                unclosed_comment = true;
                return;
            }

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
        if (grouping_token_stack.empty()) {
            Logger::inst().log_error(
                Err::ClosingUnopenedGrouping,
                t->location,
                "Found ')' without '('."
            );
        }
        else if (grouping_token_stack.back() != c) {
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
        if (grouping_token_stack.empty()) {
            Logger::inst().log_error(
                Err::ClosingUnopenedGrouping,
                t->location,
                "Found '}' without '{'."
            );
        }
        else if (grouping_token_stack.back() != c) {
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
        if (grouping_token_stack.empty()) {
            Logger::inst().log_error(
                Err::ClosingUnopenedGrouping,
                t->location,
                "Found ']' without '['."
            );
        }
        else if (grouping_token_stack.back() != c) {
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
    case '@':
        add_token(Tok::At);
        break;
    case '&':
        add_token(Tok::Amp);
        break;
    case '^':
        add_token(Tok::Caret);
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
    case '|':
        add_token(match('=') ? Tok::BarEq : Tok::Bar);
        break;
    case '!':
        add_token(match('=') ? Tok::BangEq : Tok::Bang);
        break;
    case '=':
        if (match('='))
            add_token(Tok::EqEq);
        else if (match('>'))
            add_token(Tok::DoubleArrow);
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
    case '?':
        add_token(Tok::Question);
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
                // If the previous token was a dot, scan as a tuple index.
                tuple_index();
            else
                numeric_literal();
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

void Lexer::run_scan(std::unique_ptr<FrontendContext>& context) {
    while (!is_at_end()) {
        start = current;
        scan_token();
    }

    if (repl_mode) {
        // If in REPL mode, and we need more input, pause.
        if (unclosed_comment || !grouping_token_stack.empty() ||
            !left_spacing_stack.empty() ||
            (!tokens.empty() && tokens.back()->tok_type == Tok::Colon)) {
            context->status = Status::Pause(Request::Input);
            return;
        }
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

    while (!left_spacing_stack.empty()) {
        left_spacing_stack.pop_back();
        add_token(Tok::Dedent);
    }

    tokens.push_back(eof_token);

    if (Logger::inst().get_errors().empty()) {
        context->status = Status::Ok();
        context->scanned_tokens = std::move(tokens);
    }
    else if (repl_mode) {
        context->status = Status::Pause(Request::Discard);
    }
    else {
        context->status = Status::Error();
    }
}

void Lexer::scan(
    std::unique_ptr<FrontendContext>& context,
    const std::shared_ptr<CodeFile>& file,
    bool repl_mode
) {
    if (IS_VARIANT(context->status, Status::Error)) {
        panic("Lexer::scan: Context is already in an error state.");
    }

    Lexer lexer(file, repl_mode);
    lexer.run_scan(context);
}

} // namespace nico
