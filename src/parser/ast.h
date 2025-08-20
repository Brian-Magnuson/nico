#ifndef NICO_AST_H
#define NICO_AST_H

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../common/dictionary.h"
#include "../lexer/token.h"
#include "ident.h"
#include "type.h"

// MARK: Base classes

/**
 * @brief A statement AST node.
 *
 * Statements are pieces of code that do not evaluate to a value.
 * Includes the expression statement, declarations, and non-declaring statements.
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

    // The type of the expression.
    std::shared_ptr<Type> type;
    // The location of the expression.
    const Location* location;

    /**
     * @brief Accept a visitor.
     *
     * @param visitor The visitor to accept.
     * @param as_lvalue Whether or not the expression should be treated as an lvalue.
     * @return The return value from the visitor.
     */
    virtual std::any accept(Visitor* visitor, bool as_lvalue) = 0;
};

/**
 * @brief An annotation AST node.
 *
 * An annotation object is used in the AST to organize parts of the type annotation.
 * Annotations are effectively unresolved types, which can be resolved to proper type objects in the type checker.
 * It should not be confused with a type object, which represents the resolved type of an expression.
 *
 * Type annotations are not designed to be compared with each other; comparing types should only be done after resolution.
 */
class Annotation {
public:
    class Named;

    class Pointer;
    class Reference;

    class Array;
    class Object;
    class Tuple;

    virtual ~Annotation() = default;

    /**
     * @brief A visitor class for annotations.
     */
    class Visitor {
    public:
        virtual std::any visit(Named* annotation) = 0;
        virtual std::any visit(Pointer* annotation) = 0;
        virtual std::any visit(Reference* annotation) = 0;
        virtual std::any visit(Array* annotation) = 0;
        virtual std::any visit(Object* annotation) = 0;
        virtual std::any visit(Tuple* annotation) = 0;
    };

    /**
     * @brief Accept a visitor.
     *
     * @param visitor The visitor to accept.
     * @return The return value from the visitor.
     */
    virtual std::any accept(Visitor* visitor) = 0;

    /**
     * @brief Convert the annotation to a string representation.
     *
     * This method is used for debugging and logging purposes.
     * The string representation is not unique and should not be used to compare types.
     *
     * @return A string representation of the annotation.
     */
    virtual std::string
    to_string() const = 0;
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
    std::optional<std::shared_ptr<Annotation>> annotation;

    Let(
        std::shared_ptr<Token> identifier,
        std::optional<std::shared_ptr<Expr>> expression,
        bool has_var,
        std::optional<std::shared_ptr<Annotation>> annotation
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

    Assign(std::shared_ptr<Expr> left, std::shared_ptr<Token> op, std::shared_ptr<Expr> right)
        : left(left), op(op), right(right) {
        location = &op->location;
    }

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

    Binary(std::shared_ptr<Expr> left, std::shared_ptr<Token> op, std::shared_ptr<Expr> right)
        : left(left), op(op), right(right) {
        location = &op->location;
    }

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

    Unary(std::shared_ptr<Token> op, std::shared_ptr<Expr> right)
        : op(op), right(right) {
        location = &op->location;
    }

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
    Ident ident;
    // The field entry associated with the identifier.
    std::weak_ptr<Node::FieldEntry> field_entry;

    Identifier(std::shared_ptr<Token> token)
        : ident(token) {
        location = &token->location;
    }

    Identifier(Ident ident)
        : ident(ident) {
        location = &ident.parts[0].token->location;
    }

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

    Literal(std::shared_ptr<Token> token)
        : token(token) {
        location = &token->location;
    }

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

// MARK: Annotations

/**
 * @brief An annotation consisting of an identifier.
 *
 * This annotation is used to represent named types, such as classes or interfaces.
 */
class Annotation::Named : public Annotation {
public:
    // The identifier of the named annotation.
    const Ident ident;

    Named(Ident ident) : ident(std::move(ident)) {}

    std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    std::string to_string() const override {
        return ident.to_string();
    }
};

/**
 * @brief An annotation representing a pointer type.
 *
 * This annotation is used to represent pointer types, which can be either mutable or immutable.
 */
class Annotation::Pointer : public Annotation {
public:
    // The base annotation that this pointer points to.
    const std::shared_ptr<Annotation> base;
    // Whether the object pointed to by this pointer is mutable.
    const bool is_mutable;

    Pointer(std::shared_ptr<Annotation> base, bool is_mutable = false)
        : base(std::move(base)), is_mutable(is_mutable) {}

    std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    std::string to_string() const override {
        return (is_mutable ? "var" : "") + std::string("*") + base->to_string();
    }
};

/**
 * @brief An annotation representing a reference type.
 *
 * This annotation is used to represent reference types, which can be either mutable or immutable.
 */
class Annotation::Reference : public Annotation {
public:
    // The base annotation that this reference points to.
    const std::shared_ptr<Annotation> base;
    // Whether the object pointed to by this reference is mutable.
    const bool is_mutable;

    Reference(std::shared_ptr<Annotation> base, bool is_mutable = false)
        : base(std::move(base)), is_mutable(is_mutable) {}

    std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    std::string to_string() const override {
        return (is_mutable ? "var" : "") + std::string("&") + base->to_string();
    }
};

/**
 * @brief An annotation representing an array type.
 *
 * This annotation is used to represent array types, which can be either sized or unsized.
 */
class Annotation::Array : public Annotation {
public:
    // The base annotation that this array contains.
    const std::shared_ptr<Annotation> base;
    // The number of elements in the array, if known.
    const std::optional<size_t> size;

    Array(std::shared_ptr<Annotation> base, std::optional<size_t> size = std::nullopt)
        : base(std::move(base)), size(size) {}

    std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    std::string to_string() const override {
        return "[" + base->to_string() + (size ? "; " + std::to_string(*size) : "") + "]";
    }
};

/**
 * @brief An annotation representing an object type.
 *
 * This annotation is used to represent objects with properties, similar to dictionaries.
 */
class Annotation::Object : public Annotation {
public:
    // A dictionary of properties, where keys are property names and values are annotations.
    const Dictionary<std::string, std::shared_ptr<Annotation>> properties;

    Object(Dictionary<std::string, std::shared_ptr<Annotation>> properties)
        : properties(std::move(properties)) {}

    std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    std::string to_string() const override {
        std::string result = "{";
        for (const auto& [key, value] : properties) {
            result += key + ": " + value->to_string() + ", ";
        }
        if (!properties.empty()) {
            result.pop_back();
            result.pop_back();
        }
        result += "}";

        return result;
    }
};

/**
 * @brief An annotation representing a tuple type.
 *
 * This annotation is used to represent a fixed-size collection of annotations.
 */
class Annotation::Tuple : public Annotation {
public:
    // A vector of annotations representing the elements of the tuple.
    const std::vector<std::shared_ptr<Annotation>> elements;

    Tuple(std::vector<std::shared_ptr<Annotation>> elements)
        : elements(std::move(elements)) {}

    std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }

    std::string to_string() const override {
        std::string result = "(";
        for (const auto& element : elements) {
            result += element->to_string() + ", ";
        }
        if (!elements.empty()) {
            result.pop_back();
            result.pop_back();
        }
        result += ")";
        return result;
    }
};

#endif // NICO_AST_H
