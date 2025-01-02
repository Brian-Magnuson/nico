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

    /**
     * @brief Checks if the lexer has reached the end of the source code.
     *
     * The lexer's current position is compared to the length of the source code.
     *
     * @return True if the lexer is at or past the end of the source code. False otherwise.
     */
    bool is_at_end() const;

    /**
     * @brief Adds a token to the list of tokens.
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
