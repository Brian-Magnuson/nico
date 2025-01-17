#ifndef NICO_STMT_H
#define NICO_STMT_H

#include <any>
#include <memory>

#include "../lexer/token.h"

// MARK: Base classes

/**
 * @brief A statement AST node.
 */
class Stmt {
public:
    class Expression;
    class Eof;

    virtual ~Stmt() {}

    /**
     * @brief A visitor class for statements.
     */
    class Visitor {
    public:
        virtual std::any visit(Expression* stmt) = 0;
        virtual std::any visit(Eof* stmt) = 0;
    };

    /**
     * @brief Accept a visitor.
     *
     * @param visitor The visitor to accept.
     * @return The return value from the visitor.
     */
    virtual std::any accept(Visitor* visitor) = 0;
};

/**
 * @brief An expression AST node.
 *
 * Expressions evaluate to a value.
 */
class Expr {
public:
    class Binary;
    class Unary;
    class Identifier;
    class Literal;

    virtual ~Expr() {}

    /**
     * @brief A visitor class for expressions.
     */
    class Visitor {
    public:
        virtual std::any visit(Binary* expr, bool as_lvalue) = 0;
        virtual std::any visit(Unary* expr, bool as_lvalue) = 0;
        virtual std::any visit(Identifier* expr, bool as_lvalue) = 0;
        virtual std::any visit(Literal* expr, bool as_lvalue) = 0;
    };

    /**
     * @brief Accept a visitor.
     *
     * @param visitor The visitor to accept.
     * @param as_lvalue Whether or not the expression should be treated as an lvalue.
     * @return The return value from the visitor.
     */
    virtual std::any accept(Visitor* visitor, bool as_lvalue) = 0;
};

// MARK: Statements

/**
 * @brief An expression statement.
 *
 * Expression statements are statements that consist of an expression.
 */
class Stmt::Expression : public Stmt {
public:
    // The expression in the statement
    std::shared_ptr<Expr> expression;

    Expression(std::shared_ptr<Expr> expression) : expression(expression) {}

    std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

/**
 * @brief An EOF statement.
 *
 * The EOF statement represents the end of the file.
 */
class Stmt::Eof : public Stmt {
public:
    std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

// MARK: Expressions

/**
 * @brief A binary expression.
 *
 * Binary expressions are expressions with two operands and an operator.
 */
class Expr::Binary : public Expr {
public:
    // The left operand expression
    std::shared_ptr<Expr> left;
    // The operator token
    std::shared_ptr<Token> op;
    // The right operand expression
    std::shared_ptr<Expr> right;

    Binary(std::shared_ptr<Expr> left, std::shared_ptr<Token> op, std::shared_ptr<Expr> right) : left(left), op(op), right(right) {}

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

/**
 * @brief A unary expression.
 *
 * Unary expressions are expressions with a single operand and prefix operator.
 */
class Expr::Unary : public Expr {
public:
    // The operator token
    std::shared_ptr<Token> op;
    // The operand expression
    std::shared_ptr<Expr> right;

    Unary(std::shared_ptr<Token> op, std::shared_ptr<Expr> right) : op(op), right(right) {}

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

/**
 * @brief An identifier expression.
 */
class Expr::Identifier : public Expr {
public:
    // The token representing the identifier
    std::shared_ptr<Token> token;

    Identifier(std::shared_ptr<Token> token) : token(token) {}

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

/**
 * @brief A literal expression.
 *
 * Literal expressions are expressions that represent a literal value like a number or string.
 */
class Expr::Literal : public Expr {
public:
    // The token representing the literal value
    std::shared_ptr<Token> token;

    Literal(std::shared_ptr<Token> token) : token(token) {}

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

#endif // NICO_STMT_H
