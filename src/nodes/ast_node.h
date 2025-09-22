#ifndef NICO_AST_NODE_H
#define NICO_AST_NODE_H

#include "nodes.h"

#include <optional>

#include "../common/dictionary.h"

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

    Expression(std::shared_ptr<Expr> expression)
        : expression(expression) {}

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
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
    // A weak pointer to the field entry in the symbol table.
    std::weak_ptr<Node::FieldEntry> field_entry;

    Let(std::shared_ptr<Token> identifier,
        std::optional<std::shared_ptr<Expr>> expression, bool has_var,
        std::optional<std::shared_ptr<Annotation>> annotation)
        : identifier(identifier),
          expression(expression),
          has_var(has_var),
          annotation(annotation) {}

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A print statement.
 *
 * Since a proper print function is not yet implemented, this is a temporary
 * statement for development and will be removed in the future.
 */
class Stmt::Print : public Stmt {
public:
    // The expressions to print.
    std::vector<std::shared_ptr<Expr>> expressions;

    Print(std::vector<std::shared_ptr<Expr>> expressions)
        : expressions(expressions) {}

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A pass statement.
 *
 * Pass statements do nothing and may be used in places where a statement is
 * required but no action is desired.
 *
 * Even if `pass` is supposed to do nothing, we do treat it as a real statement
 * to uphold the principles of consistency and extensibility in the compiler.
 */
class Stmt::Pass : public Stmt {
public:
    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A yield statement.
 *
 * Yield statements set the value to be yielded by a block expression.
 */
class Stmt::Yield : public Stmt {
public:
    // The expression to yield.
    std::shared_ptr<Expr> expression;

    Yield(std::shared_ptr<Expr> expression)
        : expression(expression) {}

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief An EOF statement.
 *
 * The EOF statement represents the end of the file.
 */
class Stmt::Eof : public Stmt {
public:
    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

// MARK: Expressions

/**
 * @brief An interface for possible lvalue expressions.
 *
 * A possible lvalue expression is an expression associated with a memory
 * location and can, thus, be used as an lvalue. A possible lvalue is not an
 * lvalue until it is used/visited as one.
 *
 * Only certain types of expressions may be possible lvalues, including
 * NameRef, Access, and Deref expressions.
 *
 * Note: This class should not be used by the parser to catch lvalue errors as
 * some lvalue errors can only be caught during type checking.
 */
class Expr::IPLValue : public Expr {
public:
    // Whether or not this expression is assignable.
    bool assignable = false;
    // The location to report errors at if this is not assignable.
    const Location* error_location = nullptr;

    virtual ~IPLValue() = default;
};

/**
 * @brief An assignment expression.
 *
 * Assignment expressions assign an rvalue to an lvalue.
 * Although structurally similar to binary expressions, a separate class is used
 * for organization.
 */
class Expr::Assign : public Expr {
public:
    // The left operand expression.
    std::shared_ptr<Expr> left;
    // The operator token.
    std::shared_ptr<Token> op;
    // The right operand expression.
    std::shared_ptr<Expr> right;

    Assign(
        std::shared_ptr<Expr> left, std::shared_ptr<Token> op,
        std::shared_ptr<Expr> right
    )
        : left(left), op(op), right(right) {
        location = &op->location;
    }

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

/**
 * @brief A logical expression.
 *
 * Logical expressions are expressions with two operands and a logical
 * operator (and, or).
 *
 * Although structurally similar to binary expressions, a separate class is used
 * due to the additional short-circuiting semantics required during codegen.
 */
class Expr::Logical : public Expr {
public:
    // The left operand expression.
    std::shared_ptr<Expr> left;
    // The operator token.
    std::shared_ptr<Token> op;
    // The right operand expression.
    std::shared_ptr<Expr> right;

    Logical(
        std::shared_ptr<Expr> left, std::shared_ptr<Token> op,
        std::shared_ptr<Expr> right
    )
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

    Binary(
        std::shared_ptr<Expr> left, std::shared_ptr<Token> op,
        std::shared_ptr<Expr> right
    )
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
 * @brief A dereference expression.
 *
 * Dereference expressions are used to dereference pointer and reference types.
 */
class Expr::Deref : public Expr::IPLValue {
public:
    // The operator token.
    std::shared_ptr<Token> op;
    // The operand expression.
    std::shared_ptr<Expr> right;

    Deref(std::shared_ptr<Token> op, std::shared_ptr<Expr> right)
        : op(op), right(right) {
        location = &op->location;
    }

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

/**
 * @brief An access expression.
 *
 * Access expressions are used to access members of objects or elements of
 * tuples.
 *
 * The right expression can only be an Expr::NameRef or Expr::Literal where
 * the literal is an integer.
 *
 * Although structurally similar to binary expressions, a separate class is used
 * for organization.
 */
class Expr::Access : public Expr::IPLValue {
public:
    // The base expression being accessed.
    std::shared_ptr<Expr> left;
    // The token representing the access operator (e.g., dot).
    std::shared_ptr<Token> op;
    // The token representing the member or index being accessed.
    std::shared_ptr<Token> right_token;

    Access(
        std::shared_ptr<Expr> left, std::shared_ptr<Token> op,
        std::shared_ptr<Token> right_token
    )
        : left(left), op(op), right_token(right_token) {
        location = &op->location;
    }
    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

/**
 * @brief A name reference expression.
 *
 * Name reference expressions refer to variables or functions by name.
 *
 * Note: This class used to be called Expr::Identifier, but was changed to
 * use more consistent name terminology.
 */
class Expr::NameRef : public Expr::IPLValue {
public:
    // The token representing the identifier.
    Name name;
    // The field entry associated with the identifier.
    std::weak_ptr<Node::FieldEntry> field_entry;

    NameRef(std::shared_ptr<Token> token)
        : name(token) {
        location = &token->location;
    }

    NameRef(Name name)
        : name(name) {
        location = &name.parts[0].token->location;
    }

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

/**
 * @brief A literal expression.
 *
 * Literal expressions are expressions that represent a literal value like a
 * number or string.
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

/**
 * @brief A tuple expression.
 *
 * Tuple expressions are expressions that represent a fixed-size collection
 * of values. The values may be of different types. A tuple must either have
 * at least one comma or be an empty pair of parentheses (also known as the
 * unit tuple).
 */
class Expr::Tuple : public Expr {
public:
    // The opening parenthesis of the tuple.
    std::shared_ptr<Token> lparen;
    // The elements of the tuple.
    std::vector<std::shared_ptr<Expr>> elements;

    Tuple(
        std::shared_ptr<Token> lparen,
        std::vector<std::shared_ptr<Expr>> elements
    )
        : lparen(lparen), elements(std::move(elements)) {
        location = &lparen->location;
    }

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

/**
 * @brief A unit value expression.
 *
 * Used to represent the unit value `()`.
 *
 * A subclass of `Expr::Tuple` with no elements. This class does not override
 * `Expr::Tuple::accept` and can thus be visited as a `Tuple`.
 */
class Expr::Unit : public Expr::Tuple {
public:
    Unit(std::shared_ptr<Token> token)
        : Tuple(token, {}) {}
};

/**
 * @brief A block expression.
 *
 * Block expressions are used to group statements together.
 * They may or may not yield a value.
 * Block expressions, in addition to being a valid expression on its own,
 * can also be a part of conditional and loop constructs.
 */
class Expr::Block : public Expr {
public:
    // The token that opened this block.
    std::shared_ptr<Token> opening_tok;
    // The statements contained within the block.
    std::vector<std::shared_ptr<Stmt>> statements;

    Block(
        std::shared_ptr<Token> opening_tok,
        std::vector<std::shared_ptr<Stmt>> statements
    )
        : opening_tok(opening_tok), statements(std::move(statements)) {
        location = &opening_tok->location;
    }

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

/**
 * @brief A conditional expression.
 *
 * Conditional expressions are used to represent if-else-if-else constructs.
 */
class Expr::Conditional : public Expr {
public:
    // The 'if' keyword token.
    std::shared_ptr<Token> if_kw;
    // The condition expression.
    std::shared_ptr<Expr> condition;
    // The 'then' branch expression.
    std::shared_ptr<Expr> then_branch;
    // The 'else' branch expression, if any.
    std::shared_ptr<Expr> else_branch;
    // Whether the else branch was implicit (i.e., not explicitly provided).
    bool implicit_else = false;

    Conditional(
        std::shared_ptr<Token> if_kw, std::shared_ptr<Expr> condition,
        std::shared_ptr<Expr> then_branch, std::shared_ptr<Expr> else_branch,
        bool implicit_else
    )
        : if_kw(if_kw),
          condition(condition),
          then_branch(then_branch),
          else_branch(else_branch),
          implicit_else(implicit_else) {
        location = &if_kw->location;
    }

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

// MARK: Annotations

/**
 * @brief An annotation consisting of a name.
 *
 * This annotation is used to represent named types, such as classes or
 * interfaces.
 */
class Annotation::NameRef : public Annotation {
public:
    // The name in the name reference annotation.
    const Name name;

    NameRef(Name name)
        : name(std::move(name)) {}

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }

    std::string to_string() const override { return name.to_string(); }
};

/**
 * @brief An annotation representing a pointer type.
 *
 * This annotation is used to represent pointer types, which can be either
 * mutable or immutable.
 */
class Annotation::Pointer : public Annotation {
public:
    // The base annotation that this pointer points to.
    const std::shared_ptr<Annotation> base;
    // Whether the object pointed to by this pointer is mutable.
    const bool is_mutable;

    Pointer(std::shared_ptr<Annotation> base, bool is_mutable = false)
        : base(std::move(base)), is_mutable(is_mutable) {}

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }

    std::string to_string() const override {
        return (is_mutable ? "var" : "") + std::string("*") + base->to_string();
    }
};

/**
 * @brief An annotation representing a reference type.
 *
 * This annotation is used to represent reference types, which can be either
 * mutable or immutable.
 */
class Annotation::Reference : public Annotation {
public:
    // The base annotation that this reference points to.
    const std::shared_ptr<Annotation> base;
    // Whether the object pointed to by this reference is mutable.
    const bool is_mutable;

    Reference(std::shared_ptr<Annotation> base, bool is_mutable = false)
        : base(std::move(base)), is_mutable(is_mutable) {}

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }

    std::string to_string() const override {
        return (is_mutable ? "var" : "") + std::string("&") + base->to_string();
    }
};

/**
 * @brief An annotation representing an array type.
 *
 * This annotation is used to represent array types, which can be either
 * sized or unsized.
 */
class Annotation::Array : public Annotation {
public:
    // The base annotation that this array contains.
    const std::shared_ptr<Annotation> base;
    // The number of elements in the array, if known.
    const std::optional<size_t> size;

    Array(
        std::shared_ptr<Annotation> base,
        std::optional<size_t> size = std::nullopt
    )
        : base(std::move(base)), size(size) {}

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }

    std::string to_string() const override {
        return "[" + base->to_string() +
               (size ? "; " + std::to_string(*size) : "") + "]";
    }
};

/**
 * @brief An annotation representing an object type.
 *
 * This annotation is used to represent objects with properties, similar to
 * dictionaries.
 */
class Annotation::Object : public Annotation {
public:
    // A dictionary of properties, where keys are property names and values
    // are annotations.
    const Dictionary<std::string, std::shared_ptr<Annotation>> properties;

    Object(Dictionary<std::string, std::shared_ptr<Annotation>> properties)
        : properties(std::move(properties)) {}

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }

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
 * This annotation is used to represent a fixed-size collection of
 * annotations.
 */
class Annotation::Tuple : public Annotation {
public:
    // A vector of annotations representing the elements of the tuple.
    const std::vector<std::shared_ptr<Annotation>> elements;

    Tuple(std::vector<std::shared_ptr<Annotation>> elements)
        : elements(std::move(elements)) {}

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }

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

#endif // NICO_AST_NODE_H
