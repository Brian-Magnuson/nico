#include "parser.h"

#include <cstdlib>
#include <iostream>

#include "../logger/logger.h"

bool Parser::is_at_end() const {
    return current >= tokens.size();
}

const std::shared_ptr<Token>& Parser::peek() const {
    if (is_at_end()) {
        return tokens.back();
    }
    return tokens.at(current);
}

const std::shared_ptr<Token>& Parser::previous() const {
    if (current == 0) {
        std::cerr << "Parser::previous: No previous token." << std::endl;
        std::abort();
    }
    return tokens.at(current - 1);
}

const std::shared_ptr<Token>& Parser::advance() {
    if (!is_at_end()) {
        current++;
    }
    return tokens.at(current - 1);
}

bool Parser::match(const std::vector<Tok>& types) {
    for (const auto& type : types) {
        if (peek()->tok_type == type) {
            advance();
            return true;
        }
    }
    return false;
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

std::optional<std::shared_ptr<Expr>> Parser::primary() {
    if (match({Tok::Int, Tok::Float, Tok::Bool, Tok::Str})) {
        return std::make_shared<Expr::Literal>(previous());
    }
    if (match({Tok::Identifier})) {
        return std::make_shared<Expr::Identifier>(previous());
    }

    Logger::inst().log_error(Err::NotAnExpression, peek()->location, "Expected expression.");
    return std::nullopt;
}

std::optional<std::shared_ptr<Expr>> Parser::postfix() {
    return primary();
}

std::optional<std::shared_ptr<Expr>> Parser::unary() {
    if (match({Tok::Minus})) {
        auto token = previous();
        auto right = unary();
        if (!right)
            return std::nullopt;
        return std::make_shared<Expr::Unary>(token, *right);
    }
    return postfix();
}

std::optional<std::shared_ptr<Expr>> Parser::factor() {
    return unary();
}

std::optional<std::shared_ptr<Expr>> Parser::term() {
    return factor();
}

std::optional<std::shared_ptr<Expr>> Parser::comparison() {
    return term();
}

std::optional<std::shared_ptr<Expr>> Parser::equality() {
    return comparison();
}

std::optional<std::shared_ptr<Expr>> Parser::logical_and() {
    return equality();
}

std::optional<std::shared_ptr<Expr>> Parser::logical_or() {
    return logical_and();
}

std::optional<std::shared_ptr<Expr>> Parser::assignment() {
    return logical_or();
}

std::optional<std::shared_ptr<Expr>> Parser::expression() {
    return assignment();
}

// MARK: Statements

std::optional<std::shared_ptr<Stmt>> Parser::expression_statement() {
    auto expr = expression();
    if (!expr) {
        return std::nullopt;
    }

    return std::make_shared<Stmt::Expression>(*expr);
}

std::shared_ptr<Stmt> Parser::eof_statement() {
    return std::make_shared<Stmt::Eof>();
}

std::optional<std::shared_ptr<Stmt>> Parser::statement() {
    if (match({Tok::Eof})) {
        return eof_statement();
    }
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
