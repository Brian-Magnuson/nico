#include "parser.h"

bool Parser::is_at_end() const {
    return current >= tokens.size();
}

const std::shared_ptr<Token>& Parser::peek() const {
    if (is_at_end()) {
        return tokens.back();
    }
    return tokens.at(current);
}

const std::shared_ptr<Token>& Parser::advance() {
    if (!is_at_end()) {
        current++;
    }

    return tokens.at(current - 1);
}

void Parser::synchronize() {
    advance();

    while (!is_at_end()) {
        switch (peek()->tok_type) {
        case Tok::Eof:
        case Tok::KwLet:
            return;
        default:
            break;
        }

        advance();
    }
}

// MARK: Expressions

std::optional<std::shared_ptr<Expr>> Parser::expression() {
    return std::nullopt;
}

// MARK: Statements

std::optional<std::shared_ptr<Stmt>> Parser::expression_statement() {
    auto expr = expression();
    if (!expr) {
        return std::nullopt;
    }

    return std::make_shared<Stmt::Expression>(*expr);
}

std::optional<std::shared_ptr<Stmt>> Parser::statement() {
    return expression_statement();
}

// MARK: Interface

void Parser::reset() {
    tokens.clear();
    current = 0;
}

std::vector<std::shared_ptr<Stmt>> Parser::parse(const std::vector<std::shared_ptr<Token>>&& tokens) {
    reset();
    this->tokens = std::move(tokens);

    std::vector<std::shared_ptr<Stmt>> statements;

    while (!is_at_end()) {
        auto stmt = statement();
        if (stmt) {
            statements.push_back(*stmt);
        } else {
            synchronize();
        }
    }

    return statements;
}
