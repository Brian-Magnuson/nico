#ifndef NICO_PARSER_H
#define NICO_PARSER_H

#include <memory>
#include <vector>

#include "../lexer/token.h"
#include "stmt.h"

class Parser {
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
     * @param tokens The vector of tokens to parse.
     * @return std::vector<std::shared_ptr<Stmt>> A vector of AST statements.
     */
    std::vector<std::shared_ptr<Stmt>> parse(const std::vector<std::shared_ptr<Token>>& tokens);
};

#endif // NICO_PARSER_H
