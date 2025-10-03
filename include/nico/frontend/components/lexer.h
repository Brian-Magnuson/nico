#ifndef NICO_LEXER_H
#define NICO_LEXER_H

#include <any>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "nico/frontend/utils/frontend_context.h"
#include "nico/shared/code_file.h"
#include "nico/shared/token.h"

/**
 * @brief A lexer for scanning source code into a list of tokens.
 */
class Lexer {
    // A map of keywords to their respective token types.
    static std::unordered_map<std::string_view, Tok> keywords;

    // The file being scanned.
    const std::shared_ptr<CodeFile> file;
    // Whether or not the lexer is in REPL mode.
    const bool repl_mode = false;

    // The tokens scanned from the file.
    std::vector<std::shared_ptr<Token>> tokens;
    // The index of the first character of the current token.
    size_t start = 0;
    // The index of the character from the source currently being considered.
    size_t current = 0;
    // The line number of the current token.
    size_t line = 1;
    // A stack for tracking open grouping tokens.
    std::vector<char> grouping_token_stack = {};
    // A stack for tracking left-spacing indentation levels.
    std::vector<unsigned> left_spacing_stack = {};
    // The current left spacing.
    unsigned current_left_spacing = 0;
    // The type of left spacing.
    char left_spacing_type = '\0';
    // Whether or not the lexer needs the REPL to pause for more input.
    bool repl_request_input = false;

    Lexer(std::shared_ptr<CodeFile> file, bool repl_mode = false)
        : file(file), repl_mode(repl_mode) {}

    /**
     * @brief Checks if the lexer has reached the end of the source code.
     *
     * The lexer's current position is compared to the length of the source
     * code.
     *
     * @return True if the lexer is at or past the end of the source code. False
     * otherwise.
     */
    bool is_at_end() const;

    /**
     * @brief Creates a new token with the provided type.
     *
     * The lexer is unaffected; useful for creating tokens for error messages.
     * The token's location is set based on the lexer's current position.
     *
     * @param tok_type The type of token to create.
     * @param literal The literal value of the token, if any. Default is an
     * empty std::any value.
     * @return A shared pointer to the new token.
     */
    std::shared_ptr<Token>
    make_token(Tok tok_type, std::any literal = std::any()) const;

    /**
     * @brief Creates a new token with the provided type and adds it to the list
     * of tokens.
     *
     * The token's location is set based on the lexer's current position.
     *
     * @param tok_type The type of token to add.
     * @param literal The literal value of the token, if any. Default is an
     * empty std::any value.
     */
    void add_token(Tok tok_type, std::any literal = std::any());

    /**
     * @brief Peeks at the current character without advancing the lexer.
     *
     * If the lexer is at the end of the source code, '\0' will be returned
     * instead.
     *
     * @return The current character. '\0' if the lexer is at the end of the
     * source code.
     */

    /**
     * @brief Peeks at the next character, plus lookahead, without advancing the
     * lexer.
     *
     * If the lexer is at the end of the source code, '\0' will be returned
     * instead.
     *
     * @param lookahead The number of characters to look ahead. Defaults to 0.
     * @return The character at the current position plus lookahead. '\0' if the
     * current index plus lookahead is at or past the end of the source code.
     */
    char peek(int lookahead = 0) const;

    /**
     * @brief Advances the lexer by one character, returning the character that
     * was scanned.
     *
     * E.g. if the current character is 'a', calling advance() will advance the
     * lexer to the next character and return 'a'. If the lexer is at the end of
     * the source code, '\0' will be returned and the lexer will not advance.
     *
     * @return The character that was scanned. '\0' if the lexer is at the end
     * of the source code.
     */
    char advance();

    /**
     * @brief Checks if the current character matches the expected character and
     * advances the lexer if it does.
     *
     * If the character is not a match, the lexer will not advance.
     * If the lexer is at the end of the source code, this function will return
     * false.
     *
     * @param expected The character to match.
     * @return True if the current character matches the expected character.
     * False otherwise.
     */
    bool match(char expected);

    /**
     * @brief Checks if the given character is a whitespace character.
     *
     * Only spaces, horizontal tabs, carriage returns, and newlines are
     * considered whitespace. Null characters are not considered whitespace.
     *
     * @param c The character to check.
     * @return True if the character is a whitespace character. False otherwise.
     */
    bool is_whitespace(char c) const;

    /**
     * @brief Checks if the given character is a digit within the bounds of the
     * provided base.
     *
     * If base 16 is used, uppercase (A-F) and lowercase (a-f) letters are both
     * accepted.
     *
     * If enabled, underscores may be accepted as digits.
     * Underscores may be used to separate digits for readability.
     * However, there are certain cases where a "real" digit is expected, such
     * as the first digit of a number part.
     *
     * @param c The character to check.
     * @param base The base of the number. Defaults to 10. Should be 2, 8, 10,
     * or 16.
     * @param allow_underscore Whether or not to accept underscores as digits.
     * Defaults to false.
     * @return True if the character is a digit within the bounds of the base.
     * False otherwise.
     * @warning If an invalid base is provided, the program will abort.
     */
    bool is_digit(char c, int base = 10, bool allow_underscore = false) const;

    /**
     * @brief Checks if the given character is an alphabetic character or an
     * underscore.
     *
     * Characters include all in the class `[A-Za-z_]`.
     *
     * @param c The character to check.
     * @return True if the character is alphabetic or an underscore. False
     * otherwise.
     */
    bool is_alpha(char c) const;

    /**
     * @brief Checks if the given character is an alphanumeric character or an
     * underscore.
     *
     * Characters include all in the class `[A-Za-z0-9_]`.
     * Equivalent to `is_alpha(c) || is_digit(c)`.
     *
     * @param c The character to check.
     * @return True if the character is alphanumeric or an underscore. False
     * otherwise.
     */
    bool is_alpha_numeric(char c) const;

    /**
     * @brief Consumes whitespace characters, handling indentation.
     *
     * The lexer should have advanced at least one character before calling this
     * function. All whitespace characters will be consumed until a
     * non-whitespace character is found. If the lexer is within grouping
     * tokens, the function will return here.
     *
     * If the lexer encounters mixed spacing, an error will be logged.
     * If the last token was a colon, the lexer will attempt to change it to an
     * indent token. If the last token wasn't a colon, the lexer will check if
     * dedent tokens are needed and insert them.
     */
    void consume_whitespace();

    /**
     * @brief Scans an identifier from the source code and adds it to the list
     * of tokens.
     *
     * If the token's lexeme is "true" or "false", the token type will be set to
     * `Tok::Bool`. If the token's lexeme is "inf" or "NaN", the token type will
     * be set to `Tok::Float`. If the token's lexeme is a keyword, the token
     * type will be set to the corresponding keyword.
     */
    void identifier();

    /**
     * @brief Scans a number from the source code and adds it to the list of
     * tokens.
     *
     * Hex, octal, and binary integers must start with their respective
     * prefixes: `0x`, `0o`, and `0b`. Numbers that begin with a base prefix may
     * not have any dots or exponent parts. Any number that ends with an `f`
     * (except for base 16) will be added as a float.
     *
     * Optionally, the caller can specify to only parse an integer. When
     * `integer_only` is true, dots, exponents, suffixes, underscores, and base
     * prefixes will not be included in the token. It is fast and simple.
     *
     * @param integer_only If true, the number will be parsed as an integer
     * only. If a dot is found, it will not be included in the token. Default is
     * false.
     */
    void numeric_literal(bool integer_only = false);

    /**
     * @brief Scans an str literal from the source code and adds it to the list
     * of tokens.
     *
     * Should be called after the first double quote is scanned.
     *
     * Single-line strings may span multiple lines.
     *
     * If a backslash is found, it must be followed by a character for a valid
     * escape sequence.
     */
    void str_literal();

    /**
     * @brief Scans a multi-line comment from the source code.
     *
     * The lexer should be just after the opening comment token.
     *
     * Nested comments are supported.
     */
    void multi_line_comment();

    /**
     * @brief Scans a token from the source code and adds it to the list of
     * tokens.
     *
     * The start position of the lexer should be updated before calling this
     * function.
     */
    void scan_token();

    /**
     * @brief Scans the input file, adding the tokens to
     * the provided context.
     *
     * @param context The context to add the tokens to.
     */
    void run_scan(std::unique_ptr<FrontendContext>& context);

public:
    /**
     * @brief Scans the input file, adding the tokens to the provided context.
     *
     * If the context is in an error state, this function will abort.
     *
     * @param context The context to add the tokens to.
     * @param file The file to scan.
     * @param repl_mode Whether or not the lexer is in REPL mode. Default is
     * false.
     */
    static void scan(
        std::unique_ptr<FrontendContext>& context,
        const std::shared_ptr<CodeFile>& file,
        bool repl_mode = false
    );
};

#endif // NICO_LEXER_H
