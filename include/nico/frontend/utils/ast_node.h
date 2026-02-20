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
 * @brief A statement in the AST that is allowed in a region that is strictly
 * declaration space.
 *
 * This class adds no additional members to Stmt.
 * It is used for organizational purposes.
 */
class Stmt::IDeclAllowed : virtual public Stmt {};

/**
 * @brief A statement in the AST that is allowed in a region that is strictly
 * execution space.
 *
 * This class adds no additional members to Stmt.
 * It is used for organizational purposes.
 */
class Stmt::IExecAllowed : virtual public Stmt {};

/**
 * @brief An expression statement.
 *
 * Expression statements are statements that consist of an expression.
 */
class Stmt::Expression : public Stmt::IExecAllowed {
public:
    // The expression in the statement.
    std::shared_ptr<Expr> expression;

    Expression(std::shared_ptr<Expr> expression)
        : expression(expression) {
        location = expression->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A let statement.
 *
 * Let statements introduce an execution-space variable into the current scope.
 */
class Stmt::Let : public Stmt::IExecAllowed {
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

    Let(std::shared_ptr<Token> start_token,
        std::shared_ptr<Token> identifier,
        std::optional<std::shared_ptr<Expr>> expression,
        bool has_var,
        std::optional<std::shared_ptr<Annotation>> annotation)
        : identifier(identifier),
          expression(expression),
          has_var(has_var),
          annotation(annotation) {
        location = &start_token->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A static variable declaration statement.
 *
 * Static statements introduce a declaration-space variable into the current
 * scope.
 */
class Stmt::Static : public Stmt::IDeclAllowed {
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

    Static(
        std::shared_ptr<Token> start_token,
        std::shared_ptr<Token> identifier,
        std::optional<std::shared_ptr<Expr>> expression,
        bool has_var,
        std::optional<std::shared_ptr<Annotation>> annotation
    )
        : identifier(identifier),
          expression(expression),
          has_var(has_var),
          annotation(annotation) {
        location = &start_token->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A function declaration statement.
 *
 * Function declarations introduce a new function into the current scope.
 */
class Stmt::Func : public Stmt::IDeclAllowed {
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
    std::optional<std::shared_ptr<Expr::Block>> body;
    // A weak pointer to the field entry in the symbol table.
    std::weak_ptr<Node::FieldEntry> field_entry;

    Func(
        std::shared_ptr<Token> start_token,
        std::shared_ptr<Token> identifier,
        std::optional<std::shared_ptr<Annotation>> annotation,
        std::vector<Param>&& parameters,
        std::optional<std::shared_ptr<Expr::Block>> body
    )
        : identifier(identifier),
          annotation(annotation),
          parameters(std::move(parameters)),
          body(body) {
        location = &start_token->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A print statement.
 *
 * Since a proper print function is not yet implemented, this is a temporary
 * statement for development and will be removed in the future.
 */
class Stmt::Print : public Stmt::IExecAllowed {
public:
    // The expressions to print.
    std::vector<std::shared_ptr<Expr>> expressions;

    Print(
        std::shared_ptr<Token> start_token,
        std::vector<std::shared_ptr<Expr>>&& expressions
    )
        : expressions(std::move(expressions)) {

        location = &start_token->location;
    }

    Print(std::vector<std::shared_ptr<Expr>>&& expressions)
        : expressions(std::move(expressions)) {
        if (this->expressions.empty()) {
            panic("Stmt::Print::Print: expressions cannot be empty.");
        }
        location = this->expressions.at(0)->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A deallocation statement.
 *
 * Deallocation statements free memory allocated for a given expression.
 */
class Stmt::Dealloc : public Stmt::IExecAllowed {
public:
    // The expression to deallocate.
    std::shared_ptr<Expr> expression;

    Dealloc(
        std::shared_ptr<Token> start_token, std::shared_ptr<Expr> expression
    )
        : expression(expression) {
        location = &start_token->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A pass statement.
 *
 * Pass statements do nothing and may be used in places where a statement is
 * required but no action is desired.
 *
 * Even if `pass` is supposed to do nothing, we do treat it as a real
 * statement to uphold the principles of consistency and extensibility in
 * the compiler.
 *
 * Pass is allowed in both declaration and execution spaces.
 */
class Stmt::Pass : public Stmt::IExecAllowed, public Stmt::IDeclAllowed {
public:
    Pass(std::shared_ptr<Token> pass_token) {
        location = &pass_token->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A yield statement.
 *
 * Yield statements set the value to be yielded by a block expression.
 * They may also be used to break out of loops or return from functions.
 */
class Stmt::Yield : public Stmt::IExecAllowed {
public:
    // The token representing the kind of yield (yield, break, return).
    std::shared_ptr<Token> yield_token;
    // The expression to yield.
    std::shared_ptr<Expr> expression;
    // A weak pointer to the target block expression.
    std::weak_ptr<Expr::Block> target_block;

    Yield(std::shared_ptr<Token> yield_token, std::shared_ptr<Expr> expression)
        : yield_token(yield_token), expression(expression) {
        location = &yield_token->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A continue statement.
 *
 * Continue statements skip the current iteration of a loop and proceed to
 * the next iteration.
 */
class Stmt::Continue : public Stmt::IExecAllowed {
public:
    // The token representing the continue statement.
    std::shared_ptr<Token> continue_token;

    Continue(std::shared_ptr<Token> continue_token)
        : continue_token(continue_token) {
        location = &continue_token->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief A namespace declaration statement.
 *
 * Namespace declarations introduce a new namespace into the current scope
 * and contain a block of statements that are part of the namespace.
 */
class Stmt::Namespace : public Stmt::IDeclAllowed {
public:
    // The name of the namespace.
    std::shared_ptr<Token> identifier;
    // Whether this namespace is meant to span the entire file (should only
    // be allowed if the current scope is the root scope).
    bool is_file_spanning;
    // The statements in the namespace block.
    std::vector<std::shared_ptr<Stmt::IDeclAllowed>> stmts;
    // A weak pointer to the namespace node in the symbol tree.
    std::weak_ptr<Node::Namespace> namespace_node;

    Namespace(
        std::shared_ptr<Token> start_token,
        std::shared_ptr<Token> identifier,
        bool is_file_spanning,
        std::vector<std::shared_ptr<Stmt::IDeclAllowed>>&& stmts
    )
        : identifier(identifier),
          is_file_spanning(is_file_spanning),
          stmts(std::move(stmts)) {
        location = &start_token->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief An extern declaration namespace statement.
 *
 * Extern declaration statements introduce a new namespace for external
 * declarations and contain a block of statements that are part of the
 * extern namespace.
 */
class Stmt::Extern : public Stmt::IDeclAllowed {
public:
    // An ABI enumeration for different calling conventions.
    enum class ABI {
        C,
    };

    // The name of the extern block.
    std::shared_ptr<Token> identifier;
    // The ABI for the extern declaration block.
    ABI abi;
    // The declarations in the extern block.
    std::vector<std::shared_ptr<Stmt::IDeclAllowed>> stmts;

    Extern(
        std::shared_ptr<Token> start_token,
        std::shared_ptr<Token> identifier,
        std::vector<std::shared_ptr<Stmt::IDeclAllowed>>&& stmts,
        ABI abi = ABI::C
    )
        : identifier(identifier), abi(abi), stmts(std::move(stmts)) {
        location = &start_token->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }
};

/**
 * @brief An EOF statement.
 *
 * The EOF statement represents the end of the file.
 *
 * EOF is allowed in both declaration and execution spaces.
 */
class Stmt::Eof : public Stmt::IDeclAllowed, public Stmt::IExecAllowed {
public:
    Eof(std::shared_ptr<Token> eof_token) { location = &eof_token->location; }

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
 * Note: This class should not be used by the parser to catch lvalue errors
 * as some lvalue errors can only be caught during type checking.
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
 * Although structurally similar to binary expressions, a separate class is
 * used for organization.
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
 * Although structurally similar to binary expressions, a separate class is
 * used due to the additional short-circuiting semantics required during
 * codegen.
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
 * Unary expressions are expressions with a single operand and prefix
 * operator.
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
 * operations and carry an extra boolean field for when `var` is included in
 * the expression.
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
 * Dereference expressions are used to dereference pointer and reference
 * types.
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
    /**
     * @brief The type of cast operation to be performed.
     */
    enum class Operation {
        // The cast operation is not yet determined.
        Null,
        // No operation (used when the LLVM type is unchanged) e.g. ptr ->
        // ptr
        NoOp,
        // Sign extend integer (sext) e.g. i8 -> i16
        SignExt,
        // Zero extend integer (zext) e.g. u8 -> u16, bool -> u8, bool -> i8
        ZeroExt,
        // Floating point extend (fpext) e.g. f32 -> f64
        FPExt,
        // Integer truncate (trunc) e.g. i16 -> i8, u16 -> u8
        IntTrunc,
        // Floating point truncate (fptrunc) e.g. f64 -> f32
        FPTrunc,
        // Floating point to signed integer (fptosi) e.g. f32 -> i32
        FPToSInt,
        // Floating point to unsigned integer (fptoui) e.g. f32 -> u32
        FPToUInt,
        // Signed integer to floating point (sitofp) e.g. i32 -> f32
        SIntToFP,
        // Unsigned integer to floating point (uitofp) e.g. u32 -> f32, bool
        // ->
        // f32
        UIntToFP,
        // Integer to boolean (icmp with zero)
        IntToBool,
        // Floating point to boolean (fcmp with zero)
        FPToBool,
        // Reinterpret bits (bitcast)
        ReinterpretBits
    };

    // The expression being cast.
    std::shared_ptr<Expr> expression;
    // The 'as' keyword token.
    std::shared_ptr<Token> as_token;
    // The target type annotation.
    std::shared_ptr<Annotation> annotation;
    // The target type in the expression; to be filled in by the type
    // checker.
    std::shared_ptr<Type> target_type;
    // The cast operation to be performed; to be filled in by the type
    // checker.
    Operation operation = Operation::Null;

    Cast(
        std::shared_ptr<Expr> expression,
        std::shared_ptr<Token> as_token,
        std::shared_ptr<Annotation> annotation
    )
        : expression(expression), as_token(as_token), annotation(annotation) {
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
 * The right token can be either an identifier token (for member access) or
 * a tuple index token (for tuple element access).
 *
 * Although structurally similar to binary expressions, a separate class is
 * used for organization.
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
 * @brief A subscript expression.
 *
 * Subscript expressions are used to access elements of arrays using square
 * brackets.
 *
 * They consist of a base expression and an index expression.
 * E.g., `arr[i]`
 */
class Expr::Subscript : public Expr::IPLValue {
public:
    // The base expression being subscripted.
    std::shared_ptr<Expr> left;
    // The left bracket token.
    std::shared_ptr<Token> lbracket;
    // The index expression.
    std::shared_ptr<Expr> index;

    Subscript(
        std::shared_ptr<Expr> left,
        std::shared_ptr<Token> lbracket,
        std::shared_ptr<Expr> index
    )
        : left(left), lbracket(lbracket), index(index) {
        location = &lbracket->location;
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
    // The opening parenthesis of the call.
    std::shared_ptr<Token> l_paren;
    // The positional arguments that were provided for the call.
    std::vector<std::shared_ptr<Expr>> provided_pos_args;
    // The named arguments that were provided for the call.
    Dictionary<std::string, std::shared_ptr<Expr>> provided_named_args;

    // The actual arguments to be used in the call; to be filled in by the
    // type checker.
    Dictionary<std::string, std::weak_ptr<Expr>> actual_args;

    Call(
        std::shared_ptr<Expr> callee,
        std::shared_ptr<Token> l_paren,
        std::vector<std::shared_ptr<Expr>>&& provided_pos_args,
        Dictionary<std::string, std::shared_ptr<Expr>>&& provided_named_args
    )
        : callee(callee),
          l_paren(l_paren),
          provided_pos_args(std::move(provided_pos_args)),
          provided_named_args(std::move(provided_named_args)) {
        location = &l_paren->location;
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
 * A sizeof expression consists of the `sizeof` keyword token followed by a
 * type annotation.
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
 * @brief An allocation expression.
 *
 * Allocation expressions are used to allocate heap memory for an expression
 * using the `alloc` keyword.
 *
 * They consist of the `alloc` keyword and either:
 * - A type annotation and an optional initialization expression, or
 * - `for` keyword, an amount expression, `of` keyword, and a type
 * annotation.
 */
class Expr::Alloc : public Expr {
public:
    // The 'alloc' keyword token.
    std::shared_ptr<Token> alloc_token;
    // The type annotation for the allocation.
    std::optional<std::shared_ptr<Annotation>> type_annotation;
    // The optional expression to initialize the allocated memory.
    std::optional<std::shared_ptr<Expr>> expression;
    // An optional expression for the amount to allocate (for dynamic
    // arrays).
    std::optional<std::shared_ptr<Expr>> amount_expr;

    Alloc(
        std::shared_ptr<Token> alloc_token,
        std::optional<std::shared_ptr<Annotation>> type_annotation =
            std::nullopt,
        std::optional<std::shared_ptr<Expr>> expression = std::nullopt,
        std::optional<std::shared_ptr<Expr>> amount_expr = std::nullopt
    )
        : alloc_token(alloc_token),
          type_annotation(type_annotation),
          expression(expression),
          amount_expr(amount_expr) {
        location = &alloc_token->location;
    }

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
};

class Expr::NameRef : public Expr::IPLValue {
public:
    std::shared_ptr<Name> name;
    std::weak_ptr<Node::FieldEntry> field_entry;

    NameRef(std::shared_ptr<Name> name)
        : name(name) {
        location = &name->identifier->location;
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
 * A subclass of `Expr::Tuple` with no elements. This class does not
 * override `Expr::Tuple::accept` and can thus be visited as a `Tuple`.
 */
class Expr::Unit : public Expr::Tuple {
public:
    Unit(std::shared_ptr<Token> token)
        : Tuple(token, {}) {}
};

/**
 * @brief An array expression.
 *
 * Array expressions are expressions that represent a fixed-size collection
 * of values of the same type.
 */
class Expr::Array : public Expr {
public:
    // The opening square bracket of the array.
    std::shared_ptr<Token> lsquare;
    // The elements of the array.
    std::vector<std::shared_ptr<Expr>> elements;

    Array(
        std::shared_ptr<Token> lsquare,
        std::vector<std::shared_ptr<Expr>>&& elements
    )
        : lsquare(lsquare), elements(std::move(elements)) {
        location = &lsquare->location;
    }

    std::any accept(Visitor* visitor, bool as_lvalue) override {
        return visitor->visit(this, as_lvalue);
    }
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
     * There are only 3 kinds: plain blocks, loop blocks, and function
     * blocks. Conditional blocks are considered plain blocks.
     *
     * Loop and function blocks are separate because they both have
     * associated statements (break and return) that can affect control
     * flow.
     */
    enum class Kind { Plain, Loop, Function };

    // The token that opened this block.
    std::shared_ptr<Token> opening_tok;
    // The statements contained within the block.
    std::vector<std::shared_ptr<Stmt::IExecAllowed>> statements;
    // An optional label for the block.
    std::optional<std::string> label;
    // The kind of block.
    Kind kind;
    // Whether this block is an unsafe block.
    bool is_unsafe;

    Block(
        std::shared_ptr<Token> opening_tok,
        std::vector<std::shared_ptr<Stmt::IExecAllowed>>&& statements,
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
    std::shared_ptr<Expr::Block> body;
    // The condition of the loop, if any.
    std::optional<std::shared_ptr<Expr>> condition;
    // Whether this loop is guaranteed to execute at least once.
    bool loops_once;

    Loop(
        std::shared_ptr<Token> loop_kw,
        std::shared_ptr<Expr::Block> body,
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
    const std::shared_ptr<Name> name;

    NameRef(std::shared_ptr<Name> name)
        : name(name) {
        location = &name->identifier->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }

    std::string to_string() const override { return name->to_string(); }
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
 * This annotation is used to represent the nullptr type. It is separate
 * from named annotations because `nullptr` is not an identifier. It is
 * separate from pointer annotations because it does not point to any type.
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
    const std::optional<std::shared_ptr<Annotation>> base;
    // The number of elements in the array, if known.
    const std::optional<size_t> size;

    Array(
        std::shared_ptr<Token> l_square_token,
        std::optional<std::shared_ptr<Annotation>> base = std::nullopt,
        std::optional<size_t> size = std::nullopt
    )
        : base(base), size(size) {
        location = &l_square_token->location;
    }

    std::any accept(Visitor* visitor) override { return visitor->visit(this); }

    std::string to_string() const override {
        if (!base.has_value()) {
            return "[]";
        }
        return "[" + base.value()->to_string() + "; " +
               (size ? std::to_string(size.value()) : "?") + "]";
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
 * When printed, a type-of annotation displays the location of the
 * expression it references.
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
