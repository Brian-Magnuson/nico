#ifndef NICO_LEXER_H
#define NICO_LEXER_H

#include <memory>
#include <vector>

#include "../compiler/code_file.h"
#include "token.h"

/**
 * @brief A lexer for scanning source code into a list of tokens.
 */
class Lexer {
    // The file being scanned.
    std::shared_ptr<CodeFile> file;
    // The tokens scanned from the file.
    std::vector<std::shared_ptr<Token>> tokens;
    // The index of the first character of the current token.
    size_t start = 0;
    // The index of the character from the source currently being considered.
    size_t current = 0;
    // The line number of the current token.
    size_t line = 1;

    /**
     * @brief Checks if the lexer has reached the end of the source code.
     *
     * The lexer's current position is compared to the length of the source code.
     *
     * @return True if the lexer is at or past the end of the source code. False otherwise.
     */
    bool is_at_end() const;

    /**
     * @brief Creates a new token with the provided type.
     *
     * The lexer is unaffected; useful for creating tokens for error messages.
     * The token's location is set based on the lexer's current position.
     *
     * @param tok_type The type of token to create.
     * @return A shared pointer to the new token.
     */
    std::shared_ptr<Token> make_token(Tok tok_type) const;

    /**
     * @brief Creates a new token with the provided type and adds it to the list of tokens.
     *
     * The token's location is set based on the lexer's current position.
     *
     * @param tok_type The type of token to add.
     */
    void add_token(Tok tok_type);

    /**
     * @brief Advances the lexer by one character, returning the character that was scanned.
     *
     * E.g. if the current character is 'a', calling advance() will advance the lexer to the next character and return 'a'.
     * If the lexer is at the end of the source code, '\0' will be returned and the lexer will not advance.
     *
     * @return The character that was scanned. '\0' if the lexer is at the end of the source code.
     */
    char advance();

    /**
     * @brief Checks if the current character matches the expected character and advances the lexer if it does.
     *
     * If the character is not a match, the lexer will not advance.
     * If the lexer is at the end of the source code, this function will return false.
     *
     * @param expected The character to match.
     * @return True if the current character matches the expected character. False otherwise.
     */
    bool match(char expected);

    /**
     * @brief Checks if the given character is a digit within the bounds of the provided base.
     *
     * If base 16 is used, uppercase (A-F) and lowercase (a-f) letters are both accepted.
     *
     * @param c The character to check.
     * @param base The base of the number. Defaults to 10. Should be 2, 8, 10, or 16.
     * @return True if the character is a digit within the bounds of the base. False otherwise.
     * @warning If an invalid base is provided, the program will abort.
     */
    bool is_digit(char c, int base = 10) const;

    /**
     * @brief Checks if the given character is an alphabetic character or an underscore.
     *
     * Characters include all in the class `[A-Za-z_]`.
     *
     * @param c The character to check.
     * @return True if the character is alphabetic or an underscore. False otherwise.
     */
    bool is_alpha(char c) const;

    /**
     * @brief Scans a token from the source code and adds it to the list of tokens.
     *
     * The start position of the lexer should be updated before calling this function.
     */
    void scan_token();

public:
    /**
     * @brief Resets the lexer.
     *
     * The lexer will be reset to its initial state.
     */
    void reset();

    /**
     * @brief Scans the provided file for tokens.
     *
     * The lexer will be reset before scanning the file.
     * After scanning, the lexer will hold onto the tokens and file pointer.
     *
     * @param file The file to scan.
     * @return A vector of tokens scanned from the file.
     */
    std::vector<std::shared_ptr<Token>> scan(const std::shared_ptr<CodeFile>& file);
};

#endif // NICO_LEXER_H
