#ifndef NICO_PARSER_H
#define NICO_PARSER_H

#include <memory>
#include <optional>
#include <type_traits>
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
     * @brief Checks if the current token is any of the given types and advances the parser if it is.
     * @tparam ...Args Must be of type Tok.
     * @param ...args The token types to check.
     * @return True if the current token is any of the given types. False otherwise.
     */
    template <typename... Args>
    bool match(const Args&... args) {
        static_assert(sizeof...(args) > 0, "Parser::match: requires at least one argument");
        static_assert((std::is_same_v<Args, Tok> && ...), "Parser::match: requires all arguments to be of type Tok");

        if (((peek()->tok_type == args) || ...)) {
            advance();
            return true;
        }
        return false;
    }

    /**
     * @brief Consumes tokens until a safe token is reached. Used to recover from errors.
     */
    void synchronize();

    // MARK: Expressions

    std::optional<std::shared_ptr<Expr>> primary();

    std::optional<std::shared_ptr<Expr>> postfix();

    std::optional<std::shared_ptr<Expr>> unary();

    std::optional<std::shared_ptr<Expr>> factor();

    std::optional<std::shared_ptr<Expr>> term();

    std::optional<std::shared_ptr<Expr>> comparison();

    std::optional<std::shared_ptr<Expr>> equality();

    std::optional<std::shared_ptr<Expr>> logical_and();

    std::optional<std::shared_ptr<Expr>> logical_or();

    std::optional<std::shared_ptr<Expr>> assignment();

    /**
     * @brief Parses an expression.
     *
     * An expression is a construct that evaluates to a value.
     *
     * @return A shared pointer to the parsed expression, or nullopt if the expression could not be parsed.
     */
    std::optional<std::shared_ptr<Expr>> expression();

    // MARK: Statements

    /**
     * @brief Parses an expression statement.
     *
     * An expression statement is a statement that consists of an expression.
     *
     * @return A shared pointer to the parsed statement, or nullopt if the statement could not be parsed.
     */
    std::optional<std::shared_ptr<Stmt>> expression_statement();

    /**
     * @brief Parses an EOF statement.
     *
     * An EOF statement represents the end of the file.
     *
     * @return A shared pointer to the parsed statement.
     */
    std::shared_ptr<Stmt> eof_statement();

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
