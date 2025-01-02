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
