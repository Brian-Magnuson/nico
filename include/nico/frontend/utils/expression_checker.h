#ifndef NICO_EXPRESSION_CHECKER_H
#define NICO_EXPRESSION_CHECKER_H

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/symbol_tree.h"
#include "nico/frontend/utils/type_node.h"
#include "nico/shared/dictionary.h"

namespace nico {

/**
 * @brief A visitor for checking expressions in the AST.
 *
 * During expression checking, expressions are checked for type correctness and
 * other semantic errors.
 *
 * Visit functions in this class return shared pointers to type nodes.
 */
class ExpressionChecker : public Expr::Visitor, public Annotation::Visitor {
    // The symbol tree used for type checking.
    const std::shared_ptr<SymbolTree> symbol_tree;
    // The visitor for checking statements. This is used for checking
    // expressions that contain statements, such as blocks and loops.
    Stmt::Visitor* stmt_visitor;
    // Whether or not the expression checker is currently in REPL mode.
    bool repl_mode;

    /**
     * @brief Checks if the given expression is a pointer and fully dereferences
     * it if it is.
     *
     * The expression must be type-checked before calling this function.
     *
     * This is useful for implicit dereferencing of pointers in certain
     * contexts, such with postfix operators.
     *
     * If the expression is a raw pointer and is not within an unsafe context,
     * an error will be logged and nullptr will be returned.
     *
     * If the expression is not a pointer, its type will be returned as-is.
     *
     * @param expr The expression to check and potentially dereference. Should
     * already be type-checked.
     * @return The fully dereferenced type if the expression is a pointer, or
     * the original type if it is not a pointer. Returns nullptr if the
     * expression is a raw pointer in a safe context.
     */
    std::shared_ptr<Type>
    implicit_full_dereference(std::shared_ptr<Expr>& expr);

    /**
     * @brief Attempts to match the provided arguments to the function type's
     * parameters.
     *
     * All provided arguments must be type-checked before calling this function.
     *
     * If the arguments match the parameters, this function returns a complete
     * mapping of parameter names to argument expressions.
     *
     * If the arguments do not match the parameters, this function returns
     * std::nullopt, but an error will *not* be logged. The caller may try
     * calling this function again using a different overload.
     *
     * @param func_type The function type to match the arguments against.
     * @param pos_args The positional arguments to match to the function type's
     * parameters.
     * @param named_args The named arguments to match to the function type's
     * parameters.
     * @return A mapping of parameter names to argument expressions if the
     * arguments match the parameters, or std::nullopt if they do not match.
     */
    std::optional<Dictionary<std::string, std::weak_ptr<Expr>>>
    try_match_args_to_params(
        std::shared_ptr<Type::Function> func_type,
        const std::vector<std::shared_ptr<Expr>>& pos_args,
        const Dictionary<std::string, std::shared_ptr<Expr>>& named_args
    );

    /**
     * @brief Checks if a pointer cast is valid.
     *
     * There are three kinds of pointer casts: the nullptr cast, the array
     * pointer cast, and the class cast. This function is meant to handle all
     * three of these.
     *
     * This function does not handle the reinterpret cast, even if it involves
     * pointers.
     *
     * If the cast is not valid, an error will be logged and this function will
     * return false.
     *
     * @param expr_type The pointer type of the expression being cast.
     * @param target_type The pointer type of the target of the cast.
     * @param as_token The token representing the "as" keyword in the cast. Used
     * for error logging.
     * @return True if the cast is valid, false otherwise.
     */
    bool check_pointer_cast(
        std::shared_ptr<Type::IPointer> expr_type,
        std::shared_ptr<Type::IPointer> target_type,
        std::shared_ptr<Token> as_token
    );

    std::any visit(Expr::Assign* expr, bool as_lvalue) override;
    std::any visit(Expr::Logical* expr, bool as_lvalue) override;
    std::any visit(Expr::Binary* expr, bool as_lvalue) override;
    std::any visit(Expr::Unary* expr, bool as_lvalue) override;
    std::any visit(Expr::Address* expr, bool as_lvalue) override;
    std::any visit(Expr::Deref* expr, bool as_lvalue) override;
    std::any visit(Expr::Cast* expr, bool as_lvalue) override;
    std::any visit(Expr::Access* expr, bool as_lvalue) override;
    std::any visit(Expr::Subscript* expr, bool as_lvalue) override;
    std::any visit(Expr::Call* expr, bool as_lvalue) override;
    std::any visit(Expr::SizeOf* expr, bool as_lvalue) override;
    std::any visit(Expr::Alloc* expr, bool as_lvalue) override;
    std::any visit(Expr::NameRef* expr, bool as_lvalue) override;
    std::any visit(Expr::Literal* expr, bool as_lvalue) override;
    std::any visit(Expr::Tuple* expr, bool as_lvalue) override;
    std::any visit(Expr::Array* expr, bool as_lvalue) override;
    std::any visit(Expr::Block* expr, bool as_lvalue) override;
    std::any visit(Expr::Conditional* expr, bool as_lvalue) override;
    std::any visit(Expr::Loop* expr, bool as_lvalue) override;

    std::any visit(Annotation::NameRef* annotation) override;
    std::any visit(Annotation::Pointer* annotation) override;
    std::any visit(Annotation::Nullptr* annotation) override;
    std::any visit(Annotation::Reference* annotation) override;
    std::any visit(Annotation::Array* annotation) override;
    std::any visit(Annotation::Object* annotation) override;
    std::any visit(Annotation::Tuple* annotation) override;
    std::any visit(Annotation::TypeOf* annotation) override;

public:
    ExpressionChecker(
        std::shared_ptr<SymbolTree> symbol_tree,
        Stmt::Visitor* stmt_visitor,
        bool repl_mode = false
    )
        : symbol_tree(symbol_tree),
          stmt_visitor(stmt_visitor),
          repl_mode(repl_mode) {}

    // TODO: These should return std::optional<std::shared_ptr<Type>> instead;
    // we avoid the use of nullptr to indicate the absense of a type.

    /**
     * @brief Checks the given expression and returns its type if it is valid.
     *
     * This function can also check if the expression is an unsized rvalue.
     * Unsized rvalues imply the expression is being loaded into memory without
     * a known size. If this is detected, an error will be logged and nullptr
     * will be returned.
     *
     * To skip this check, set `allow_unsized_rvalue` to true.
     *
     * @param expr The expression to check.
     * @param as_lvalue Whether or not the expression should be treated as an
     * lvalue.
     * @param allow_unsized_rvalue Whether or not to allow rvalues of unsized
     * types. Default is false.
     * @return The type of the expression if it is valid, or nullptr if it is
     * not valid.
     */
    std::shared_ptr<Type> expr_check(
        std::shared_ptr<Expr> expr,
        bool as_lvalue,
        bool allow_unsized_rvalue = false
    );

    /**
     * @brief Checks the given annotation and returns its type if it is valid.
     *
     * @param annotation The annotation to check.
     * @return The type of the annotation if it is valid, or nullptr if it is
     * not valid.
     */
    std::shared_ptr<Type>
    annotation_check(std::shared_ptr<Annotation> annotation);
};

} // namespace nico

#endif // NICO_EXPRESSION_CHECKER_H
