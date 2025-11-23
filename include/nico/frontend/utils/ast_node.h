#ifndef NICO_AST_NODE_H
#define NICO_AST_NODE_H

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "nico/frontend/utils/nodes.h"
#include "nico/shared/dictionary.h"

namespace nico {

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
        std::optional<std::shared_ptr<Expr>> expression,
        bool has_var,
        std::optional<std::shared_ptr<Annotation>> annotation)
        : identifier(identifier),
          expression(expression),
          has_var(has_var),
          annotation(annotation) {}

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A function declaration statement.
 *
 * Function declarations introduce a new function into the current scope.
 */
class Stmt::Func : public Stmt {
public:
    /**
     * @brief A parameter in a function declaration.
     */
    struct Param {
        // Whether the parameter is declared with `var` or not.
        bool has_var;
        // The identifier token.
        std::shared_ptr<Token> identifier;
        // The type annotation, always required.
        std::shared_ptr<Annotation> annotation;
        // An optional expression for the default value.
        std::optional<std::shared_ptr<Expr>> expression;
        // A weak pointer to the parameter's field entry in the symbol table.
        std::weak_ptr<Node::FieldEntry> field_entry;

        Param(
            bool has_var,
            std::shared_ptr<Token> identifier,
            std::shared_ptr<Annotation> annotation,
            std::optional<std::shared_ptr<Expr>> expression
        )
            : has_var(has_var),
              identifier(identifier),
              annotation(annotation),
              expression(expression) {}
    };
    // The function name token.
    std::shared_ptr<Token> identifier;
    // The annotation for the return type.
    std::optional<std::shared_ptr<Annotation>> annotation;
    // The parameters of the function.
    std::vector<Param> parameters;
    // The body of the function.
    std::shared_ptr<Expr::Block> body;
    // A weak pointer to the field entry in the symbol table.
    std::weak_ptr<Node::FieldEntry> field_entry;

    Func(
        std::shared_ptr<Token> identifier,
        std::optional<std::shared_ptr<Annotation>> annotation,
        std::vector<Param>&& parameters,
        std::shared_ptr<Expr::Block> body
    )
        : identifier(identifier),
          annotation(annotation),
          parameters(std::move(parameters)),
          body(body) {}

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

    Print(std::vector<std::shared_ptr<Expr>>&& expressions)
        : expressions(std::move(expressions)) {}

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
 * They may also be used to break out of loops or return from functions.
 */
class Stmt::Yield : public Stmt {
public:
    // The token representing the kind of yield (yield, break, return).
    std::shared_ptr<Token> yield_token;
    // The expression to yield.
    std::shared_ptr<Expr> expression;
    // A weak pointer to the target block expression.
    std::weak_ptr<Expr::Block> target_block;

    Yield(std::shared_ptr<Token> yield_token, std::shared_ptr<Expr> expression)
        : yield_token(yield_token), expression(expression) {}

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A continue statement.
 *
 * Continue statements skip the current iteration of a loop and proceed to the
 * next iteration.
 */
class Stmt::Continue : public Stmt {
public:
    // The token representing the continue statement.
    std::shared_ptr<Token> continue_token;

    Continue(std::shared_ptr<Token> continue_token)
        : continue_token(continue_token) {}

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
        std::shared_ptr<Expr> left,
        std::shared_ptr<Token> op,
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
        std::shared_ptr<Expr> left,
        std::shared_ptr<Token> op,
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
        std::shared_ptr<Expr> left,
        std::shared_ptr<Token> op,
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
 * @brief An address-of expression.
 *
 * Address-of expressions are used to get the address of a variable using
 * the `@` or `&` operator.
 * They are similar to unary expressions but specifically for address-of
 * operations and carry an extra boolean field for when `var` is included in the
 * expression.
 */
class Expr::Address : public Expr {
public:
    // The operator token.
    std::shared_ptr<Token> op;
    // The operand expression.
    std::shared_ptr<Expr> right;
    // Whether the address is of a variable.
    bool has_var;

    Address(
        std::shared_ptr<Token> op, std::shared_ptr<Expr> right, bool has_var
    )
        : op(op), right(right), has_var(has_var) {
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
 * @brief A cast expression.
 *
 * Cast expressions are used to cast an expression to a different type using
 * the `as` keyword.
 */
class Expr::Cast : public Expr {
public:
    // The expression being cast.
    std::shared_ptr<Expr> expression;
    // The 'as' keyword token.
    std::shared_ptr<Token> as_token;
    // The target type annotation.
    std::shared_ptr<Annotation> target_type;

    Cast(
        std::shared_ptr<Expr> expression,
        std::shared_ptr<Token> as_token,
        std::shared_ptr<Annotation> target_type
    )
        : expression(expression), as_token(as_token), target_type(target_type) {
        location = &as_token->location;
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
        std::shared_ptr<Expr> left,
        std::shared_ptr<Token> op,
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
 * @brief A call expression.
 *
 * Call expressions are used to make function calls.
 *
 * They consist of a callee (the callable object) and a list of arguments.
 * Arguments can be either positional or named as long as there are no
 * positional arguments after any named arguments.
 */
class Expr::Call : public Expr {
public:
    // The callee expression, usually a NameRef for a function.
    std::shared_ptr<Expr> callee;
    // The positional arguments that were provided for the call.
    std::vector<std::shared_ptr<Expr>> provided_pos_args;
    // The named arguments that were provided for the call.
    Dictionary<std::string, std::shared_ptr<Expr>> provided_named_args;

    // The actual arguments to be used in the call; to be filled in by the type
    // checker.
    Dictionary<std::string, std::weak_ptr<Expr>> actual_args;

    Call(
        std::shared_ptr<Expr> callee,
        std::vector<std::shared_ptr<Expr>>&& provided_pos_args,
        Dictionary<std::string, std::shared_ptr<Expr>>&& provided_named_args
    )
        : callee(callee),
          provided_pos_args(std::move(provided_pos_args)),
          provided_named_args(std::move(provided_named_args)) {
        location = callee->location;
    }

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

/**
 * @brief A sizeof expression.
 *
 * Sizeof expressions are used to get the size of a type in bytes using the
 * `sizeof` keyword.
 *
 * A sizeof expression consists of the `sizeof` keyword token followed by a type
 * annotation.
 */
class Expr::SizeOf : public Expr {
public:
    // The 'sizeof' keyword token.
    std::shared_ptr<Token> sizeof_token;
    // The type annotation whose size is to be determined.
    std::shared_ptr<Annotation> annotation;
    // The type in the expression; to be filled in by the type checker.
    std::shared_ptr<Type> inner_type;

    SizeOf(
        std::shared_ptr<Token> sizeof_token,
        std::shared_ptr<Annotation> annotation
    )
        : sizeof_token(sizeof_token), annotation(annotation) {
        location = &sizeof_token->location;
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
        std::vector<std::shared_ptr<Expr>>&& elements
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
    /**
     * @brief Enumeration of block kinds.
     *
     * There are only 3 kinds: plain blocks, loop blocks, and function blocks.
     * Conditional blocks are considered plain blocks.
     *
     * Loop and function blocks are separate because they both have associated
     * statements (break and return) that can affect control flow.
     */
    enum class Kind { Plain, Loop, Function };

    // The token that opened this block.
    std::shared_ptr<Token> opening_tok;
    // The statements contained within the block.
    std::vector<std::shared_ptr<Stmt>> statements;
    // An optional label for the block.
    std::optional<std::string> label;
    // The kind of block.
    Kind kind;
    // Whether this block is an unsafe block.
    bool is_unsafe;

    Block(
        std::shared_ptr<Token> opening_tok,
        std::vector<std::shared_ptr<Stmt>>&& statements,
        Kind kind,
        bool is_unsafe = false
    )
        : opening_tok(opening_tok),
          statements(std::move(statements)),
          kind(kind),
          is_unsafe(is_unsafe) {
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
        std::shared_ptr<Token> if_kw,
        std::shared_ptr<Expr> condition,
        std::shared_ptr<Expr> then_branch,
        std::shared_ptr<Expr> else_branch,
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

/**
 * @brief A loop expression.
 *
 * Loop expressions are used to represent infinite loops or loops with a
 * condition (similar to while loops).
 */
class Expr::Loop : public Expr {
public:
    // The 'loop' keyword token.
    std::shared_ptr<Token> loop_kw;
    // The body of the loop.
    std::shared_ptr<Expr> body;
    // The condition of the loop, if any.
    std::optional<std::shared_ptr<Expr>> condition;
    // Whether this loop is guaranteed to execute at least once.
    bool loops_once;

    Loop(
        std::shared_ptr<Token> loop_kw,
        std::shared_ptr<Expr> body,
        std::optional<std::shared_ptr<Expr>> condition,
        bool loops_once
    )
        : loop_kw(loop_kw),
          body(body),
          condition(condition),
          loops_once(loops_once) {
        location = &loop_kw->location;
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
        : name(std::move(name)) {
        if (this->name.parts.empty()) {
            panic("Annotation::NameRef::NameRef: name has no parts.");
        }
        location = &this->name.parts.back().token->location;
    }

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

    Pointer(
        std::shared_ptr<Annotation> base,
        std::shared_ptr<Token> at_token,
        bool is_mutable = false
    )
        : base(std::move(base)), is_mutable(is_mutable) {
        location = &at_token->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }

    std::string to_string() const override {
        return (is_mutable ? "var" : "") + std::string("@") + base->to_string();
    }
};

/**
 * @brief An annotation representing a nullptr type.
 *
 * This annotation is used to represent the nullptr type. It is separate from
 * named annotations because `nullptr` is not an identifier. It is separate from
 * pointer annotations because it does not point to any type.
 */
class Annotation::Nullptr : public Annotation {
public:
    Nullptr(std::shared_ptr<Token> nullptr_token) {
        location = &nullptr_token->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }

    std::string to_string() const override { return "nullptr"; }
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

    Reference(
        std::shared_ptr<Annotation> base,
        std::shared_ptr<Token> amp_token,
        bool is_mutable = false
    )
        : base(std::move(base)), is_mutable(is_mutable) {
        location = &amp_token->location;
    }

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
        std::shared_ptr<Token> l_bracket_token,
        std::shared_ptr<Annotation> base,
        std::optional<size_t> size = std::nullopt
    )
        : base(std::move(base)), size(size) {
        location = &l_bracket_token->location;
    }

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

    Object(
        std::shared_ptr<Token> l_brace_token,
        Dictionary<std::string, std::shared_ptr<Annotation>>&& properties
    )
        : properties(std::move(properties)) {
        location = &l_brace_token->location;
    }

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

    Tuple(
        std::shared_ptr<Token> l_paren_token,
        std::vector<std::shared_ptr<Annotation>>&& elements
    )
        : elements(std::move(elements)) {
        location = &l_paren_token->location;
    }

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

/**
 * @brief A type-of annotation.
 *
 * Type-of annotations are used to create annotations based on the type of
 * another expression.
 * A type-of annotation is an annotation, meaning it can only appear where
 * annotations are expected.
 *
 * When printed, a type-of annotation displays the location of the expression it
 * references.
 */
class Annotation::TypeOf : public Annotation {
public:
    // The expression whose type is being referenced.
    const std::shared_ptr<Expr> expression;

    TypeOf(
        std::shared_ptr<Token> typeof_token, std::shared_ptr<Expr> expression
    )
        : expression(std::move(expression)) {
        location = &typeof_token->location;
    }
    std::any accept(Visitor* visitor) override { return visitor->visit(this); }

    std::string to_string() const override {
        auto [_, line, col] = expression->location->to_tuple();
        return "typeof(<expr@" + std::to_string(line) + ":" +
               std::to_string(col) + ">)";
    }
};

} // namespace nico

#endif // NICO_AST_NODE_H
