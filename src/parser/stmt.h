#ifndef NICO_STMT_H
#define NICO_STMT_H

#include <any>
#include <memory>
#include <optional>
#include <vector>

#include "../lexer/token.h"
#include "type.h"

// MARK: Base classes

/**
 * @brief A statement AST node.
 */
class Stmt {
public:
    class Expression;
    class Let;
    class Eof;

    class Print;

    virtual ~Stmt() {}

    /**
     * @brief A visitor class for statements.
     */
    class Visitor {
    public:
        virtual std::any visit(Expression* stmt) = 0;
        virtual std::any visit(Let* stmt) = 0;
        virtual std::any visit(Eof* stmt) = 0;
        virtual std::any visit(Print* stmt) = 0;
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
    class Assign;
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
        virtual std::any visit(Assign* expr, bool as_lvalue) = 0;
        virtual std::any visit(Binary* expr, bool as_lvalue) = 0;
        virtual std::any visit(Unary* expr, bool as_lvalue) = 0;
        virtual std::any visit(Identifier* expr, bool as_lvalue) = 0;
        virtual std::any visit(Literal* expr, bool as_lvalue) = 0;
    };

    std::shared_ptr<Type> type;

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
    // The expression in the statement.
    std::shared_ptr<Expr> expression;

    Expression(std::shared_ptr<Expr> expression) : expression(expression) {}

    std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

/**
 * @brief A let statement.
 *
 * Let statements introduce a new variable into the current scope.
 */
class Stmt::Let : public Stmt {
public:
    // The identifier token.
    std::shared_ptr<Token> identifier;
    // The expression in the statement; nullopt if absent.
    std::optional<std::shared_ptr<Expr>> expression;
    // Whether the variable is declared as mutable.
    bool has_var;
    // The type annotation; should be type-checked, even if not nullopt.
    std::optional<std::shared_ptr<Type>> annotation;

    // Let(std::shared_ptr<Token> identifier, std::optional<std::shared_ptr<Expr>> expression, bool has_var) : identifier(identifier), expression(expression), has_var(has_var) {}
    Let(
        std::shared_ptr<Token> identifier, std::optional<std::shared_ptr<Expr>> expression,
        bool has_var,
        std::optional<std::shared_ptr<Type>> annotation
    ) : identifier(identifier),
        expression(expression),
        has_var(has_var),
        annotation(annotation) {}

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

/**
 * @brief A print statement.
 *
 * Since a proper print function is not yet implemented, this is a temporary statement for development and will be removed in the future.
 */
class Stmt::Print : public Stmt {
public:
    // The expressions to print.
    std::vector<std::shared_ptr<Expr>> expressions;

    Print(std::vector<std::shared_ptr<Expr>> expressions) : expressions(expressions) {}

    std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

// MARK: Expressions

/**
 * @brief An assignment expression.
 *
 * Assignment expressions assign an rvalue to an lvalue.
 * Although structurally similar to binary expressions, a separate class is used for organization.
 */
class Expr::Assign : public Expr {
public:
    // The left operand expression.
    std::shared_ptr<Expr> left;
    // The operator token.
    std::shared_ptr<Token> op;
    // The right operand expression.
    std::shared_ptr<Expr> right;

    Assign(std::shared_ptr<Expr> left, std::shared_ptr<Token> op, std::shared_ptr<Expr> right) : left(left), op(op), right(right) {}

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

/**
 * @brief A binary expression.
 *
 * Binary expressions are expressions with two operands and an operator.
 * Does not include assignment expressions; use `Expr::Assign` instead.
 */
class Expr::Binary : public Expr {
public:
    // The left operand expression.
    std::shared_ptr<Expr> left;
    // The operator token.
    std::shared_ptr<Token> op;
    // The right operand expression.
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
    // The operator token.
    std::shared_ptr<Token> op;
    // The operand expression.
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
    // The token representing the identifier.
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
    // The token representing the literal value.
    std::shared_ptr<Token> token;

    Literal(std::shared_ptr<Token> token) : token(token) {}

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

#endif // NICO_STMT_H
