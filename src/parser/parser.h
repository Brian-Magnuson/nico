#ifndef NICO_PARSER_H
#define NICO_PARSER_H

#include <memory>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../lexer/token.h"
#include "../nodes/ast_node.h"
#include "ast.h"

/**
 * @brief A parser to parse a vector of tokens into an abstract syntax tree.
 */
class Parser {
    // The vector of tokens to parse.
    std::vector<std::shared_ptr<Token>> tokens;
    // The current token index.
    unsigned current = 0;
    // A table of basic types.
    // static std::unordered_map<std::string, std::shared_ptr<Type>> type_table;

    /**
     * @brief Checks if the parser has reached the end of the tokens list.
     *
     * The end of the tokens list is reached when `current >= tokens.size()`.
     *
     * @return True if the parser has reached the end of the tokens list. False
     * otherwise.
     */
    bool is_at_end() const;

    /**
     * @brief Peeks at the current token.
     *
     * @return A pointer to the current token. If the parser has reached the end
     * of the tokens list, the last token will be returned instead.
     */
    const std::shared_ptr<Token>& peek() const;

    /**
     * @brief Peeks at the previous token.
     *
     * @return A pointer to the previous token.
     * @warning If there is no previous token, the program will abort.
     */
    const std::shared_ptr<Token>& previous() const;

    /**
     * @brief Advances the parser to the next token, returning the token that
     * was consumed.
     *
     * E.g. if the current token is a `let` token, calling advance() will
     * advance the parser to the next token and return the `let` token.
     *
     * @return A pointer to the token that was consumed. If the parser has
     * reached the end of the tokens list, the last token will be returned
     * instead.
     */
    const std::shared_ptr<Token>& advance();

    /**
     * @brief Checks if the current token's type matches any of the provided
     * types, and advances the parser if it does.
     *
     * @param types The types to match.
     * @return True if the current token's type matches any of the provided
     * types. False otherwise.
     */
    bool match(const std::vector<Tok>& types);

    /**
     * @brief Consumes tokens until a safe token is reached. Used to recover
     * from errors.
     */
    void synchronize();

    // MARK: Expressions

    /**
     * @brief Parses a block expression.
     *
     * A block expression is a sequence of statements enclosed in braces.
     *
     * @param opening_kw The keyword that opened the block.
     * @return A shared pointer to the parsed expression, or nullopt if the
     * expression could not be parsed.
     */
    std::optional<std::shared_ptr<Expr>>
    block(std::shared_ptr<Token> opening_kw);

    /**
     * @brief Parses a primary expression.
     *
     * Primary expressions include literals, identifiers, and grouping
     * expressions.
     *
     * @return A shared pointer to the parsed expression, or nullopt if the
     * expression could not be parsed.
     */
    std::optional<std::shared_ptr<Expr>> primary();

    /**
     * @brief Parses a postfix expression.
     *
     * Postfix expressions include dot access, call expressions, and subscript
     * access.
     *
     * There is no specific class for postfix expressions, but they all have
     * higher precedence than unary expressions, and, thus, are handled together
     * in this function.
     *
     * @return A shared pointer to the parsed expression, or nullopt if the
     * expression could not be parsed.
     */
    std::optional<std::shared_ptr<Expr>> postfix();

    /**
     * @brief Parses a unary expression.
     *
     * Includes `-a`
     *
     * @return A shared pointer to the parsed expression, or nullopt if the
     * expression could not be parsed.
     */
    std::optional<std::shared_ptr<Expr>> unary();

    /**
     * @brief Parses a factor expression.
     *
     * Includes `a * b`, `a / b`, `a % b`
     *
     * @return A shared pointer to the parsed expression, or nullopt if the
     * expression could not be parsed.
     */
    std::optional<std::shared_ptr<Expr>> factor();

    /**
     * @brief Parses a term expression.
     *
     * Includes `a + b`, `a - b`
     *
     * @return A shared pointer to the parsed expression, or nullopt if the
     * expression could not be parsed.
     */
    std::optional<std::shared_ptr<Expr>> term();

    std::optional<std::shared_ptr<Expr>> comparison();

    std::optional<std::shared_ptr<Expr>> equality();

    /**
     * @brief Parses a logical and expression.
     *
     * Logical and expressions include `a && b`.
     *
     * @return A shared pointer to the parsed expression, or nullopt if the
     * expression could not be parsed.
     */
    std::optional<std::shared_ptr<Expr>> logical_and();

    /**
     * @brief Parses a logical or expression.
     *
     * Logical or expressions include `a || b`.
     *
     * @return A shared pointer to the parsed expression, or nullopt if the
     * expression could not be parsed.
     */
    std::optional<std::shared_ptr<Expr>> logical_or();

    /**
     * @brief Parses an assignment expression.
     *
     * Assignment expressions assign an rvalue to an lvalue.
     * Unlike other binary expressions, assignment expressions are
     * right-associative and have their own class.
     *
     * @return A shared pointer to the parsed expression, or nullopt if the
     * expression could not be parsed.
     */
    std::optional<std::shared_ptr<Expr>> assignment();

    /**
     * @brief Parses an expression.
     *
     * An expression is a construct that evaluates to a value.
     *
     * @return A shared pointer to the parsed expression, or nullopt if the
     * expression could not be parsed.
     */
    std::optional<std::shared_ptr<Expr>> expression();

    // MARK: Statements

    /**
     * @brief Parses a let statement.
     *
     * A let statement introduces a new variable into the current scope.
     *
     * @return A shared pointer to the parsed statement, or nullopt if the
     * statement could not be parsed.
     */
    std::optional<std::shared_ptr<Stmt>> let_statement();

    /**
     * @brief Parses a print statement.
     *
     * Print statements print a series of expressions to the console.
     * Print statements are temporary and will be removed in the future.
     *
     * @return A shared pointer to the parsed statement, or nullopt if the
     * statement could not be parsed.
     */
    std::optional<std::shared_ptr<Stmt>> print_statement();

    /**
     * @brief Parses a yield statement.
     *
     * Yield statements set the value to be yielded by a block expression.
     *
     * @return A shared pointer to the parsed statement, or nullopt if the
     * statement could not be parsed.
     */
    std::optional<std::shared_ptr<Stmt>> yield_statement();

    /**
     * @brief Parses an expression statement.
     *
     * An expression statement is a statement that consists of an expression.
     *
     * @return A shared pointer to the parsed statement, or nullopt if the
     * statement could not be parsed.
     */
    std::optional<std::shared_ptr<Stmt>> expression_statement();

    /**
     * @brief Parses a statement.
     *
     * A statement is the most basic construct in the language. Includes all
     * declarations, expressions, and control flow.
     *
     * @return A shared pointer to the parsed statement, or nullopt if the
     * statement could not be parsed.
     */
    std::optional<std::shared_ptr<Stmt>> statement();

    // MARK: Annotations

    std::optional<std::shared_ptr<Annotation>> annotation();

public:
    // MARK: Interface

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
     * @return Ast The parsed abstract syntax tree.
     */
    Ast parse(const std::vector<std::shared_ptr<Token>>&& tokens);
};

#endif // NICO_PARSER_H
