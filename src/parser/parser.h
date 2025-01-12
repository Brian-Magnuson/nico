#ifndef NICO_PARSER_H
#define NICO_PARSER_H

#include <memory>
#include <optional>
#include <vector>

#include "../lexer/token.h"
#include "stmt.h"

/**
 * @brief A parser to parse a vector of tokens into an abstract syntax tree.
 */
class Parser {
    // The vector of tokens to parse.
    std::vector<std::shared_ptr<Token>> tokens;
    // The current token index.
    unsigned current = 0;

    /**
     * @brief Checks if the parser has reached the end of the tokens list.
     *
     * The end of the tokens list is reached when `current >= tokens.size()`.
     *
     * @return True if the parser has reached the end of the tokens list. False otherwise.
     */
    bool is_at_end() const;

    /**
     * @brief Peeks at the current token.
     * @return A pointer to the current token. If the parser has reached the end of the tokens list, the last token will be returned instead.
     */
    const std::shared_ptr<Token>& peek() const;

    /**
     * @brief Advances the parser to the next token, returning the token that was consumed.
     *
     * E.g. if the current token is a `let` token, calling advance() will advance the parser to the next token and return the `let` token.
     *
     * @return A pointer to the token that was consumed. If the parser has reached the end of the tokens list, the last token will be returned instead.
     */
    const std::shared_ptr<Token>& advance();

    /**
     * @brief Consumes tokens until a safe token is reached. Used to recover from errors.
     */
    void synchronize();

    /**
     * @brief Parses a statement.
     *
     * A statement is the most basic construct in the language. Includes all declarations, expressions, and control flow.
     *
     * @return A shared pointer to the parsed statement, or nullopt if the statement could not be parsed.
     */
    std::optional<std::shared_ptr<Stmt>> statement();

public:
    /**
     * Resets the parser.
     *
     * The parser will be reset to its initial state.
     */
    void reset();

    /**
     * Parses the vector of tokens into an abstract syntax tree.
     *
     * The parser will be reset before parsing.
     *
     * @param tokens (Requires move) The vector of tokens to parse.
     * @return std::vector<std::shared_ptr<Stmt>> A vector of AST statements.
     */
    std::vector<std::shared_ptr<Stmt>> parse(const std::vector<std::shared_ptr<Token>>&& tokens);
};

#endif // NICO_PARSER_H
