#ifndef NICO_LOCAL_CHECKER_H
#define NICO_LOCAL_CHECKER_H

#include <any>
#include <memory>
#include <vector>

#include "nico/frontend/utils/annotation_checker.h"
#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/frontend_context.h"
#include "nico/frontend/utils/symbol_tree.h"
#include "nico/frontend/utils/type_node.h"

namespace nico {

/**
 * @brief A local type checker.
 *
 * The local type checker checks statements and expressions at the local level,
 * i.e., within functions, blocks, and the main script.
 */
class LocalChecker : public Stmt::Visitor, public Expr::Visitor {
    // The symbol tree used for type checking.
    const std::shared_ptr<SymbolTree> symbol_tree;
    // Whether or not the checker is running in REPL mode.
    const bool repl_mode = false;
    // The annotation checker used for checking type annotations.
    AnnotationChecker annotation_checker;

    LocalChecker(
        std::shared_ptr<SymbolTree> symbol_tree, bool repl_mode = false
    )
        : symbol_tree(symbol_tree),
          repl_mode(repl_mode),
          annotation_checker(symbol_tree, this) {};

    /**
     * @brief Checks the type of the expression and returns its type.
     *
     * @param expr The expression to check.
     * @param as_lvalue Whether the expression is being checked as an lvalue.
     * @return The type of the expression. Can be
     * nullptr.
     */
    std::shared_ptr<Type>
    expr_check(std::shared_ptr<Expr> expr, bool as_lvalue = false);

    /**
     * @brief Checks if the expression is a pointer and fully dereferences it if
     * it is.
     *
     * The expression must be type checked before calling this function.
     *
     * This is useful for implicit dereferencing in certain contexts, such as
     * with postfix operators.
     *
     * If the expression is a raw pointer and is not within an unsafe context,
     * an error will be logged and nullptr will be returned.
     *
     * @param expr The expression to check. Should be type checked.
     * @return The fully dereferenced type of the expression, or nullptr if an
     * error occurred.
     */
    std::shared_ptr<Type>
    implicit_full_dereference(std::shared_ptr<Expr>& expr);

    /**
     * @brief Checks if a pointer cast is valid.
     *
     * If the cast is not valid, an error message will be logged.
     *
     * The arguments should be pointer types. If they are not pointer types,
     * this function may see the cast as invalid.
     *
     * There are three kinds of pointer casts: the nullptr cast, the array
     * pointer cast, and the class cast. This function is meant to handle all 3
     * of these.
     *
     * This function does not handle the reinterpret cast, even though it may
     * involve pointers.
     *
     * @param expr_type The type of the expression being cast. Should be a
     * pointer type.
     * @param target_type The target type of the cast. Should be a pointer type.
     * @param as_token The 'as' token in the cast. Used for error reporting.
     * @return True if the cast is valid, false otherwise.
     */
    bool check_pointer_cast(
        std::shared_ptr<Type> expr_type,
        std::shared_ptr<Type> target_type,
        std::shared_ptr<Token> as_token
    );

    /**
     * @brief Attempts to match the provided arguments to the function
     * parameters.
     *
     * The arguments should be type checked *before* calling this function.
     *
     * If the arguments match the parameters, this function returns a complete
     * mapping of parameter names to argument expressions.
     *
     * @param func_type The function type to match against.
     * @param pos_args The positional arguments to match.
     * @param named_args The named arguments to match.
     * @return A mapping of parameter names to argument expressions, or an empty
     * optional if the arguments do not match.
     */
    std::optional<Dictionary<std::string, std::weak_ptr<Expr>>>
    try_match_args_to_params(
        std::shared_ptr<Type::Function> func_type,
        const std::vector<std::shared_ptr<Expr>>& pos_args,
        const Dictionary<std::string, std::shared_ptr<Expr>>& named_args
    );

    std::any visit(Stmt::Expression* stmt) override;
    std::any visit(Stmt::Let* stmt) override;
    std::any visit(Stmt::Func* stmt) override;
    std::any visit(Stmt::Print* stmt) override;
    std::any visit(Stmt::Pass* stmt) override;
    std::any visit(Stmt::Yield* stmt) override;
    std::any visit(Stmt::Continue* stmt) override;
    std::any visit(Stmt::Eof* stmt) override;

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

    /**
     * @brief Type checks the given context at the local level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param context The front end context containing the AST to type check.
     */
    void run_check(std::unique_ptr<FrontendContext>& context);

public:
    /**
     * @brief Type checks the given context at the local level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param context The front end context containing the AST to type check.
     * @param repl_mode Whether or not the checker is running in REPL mode.
     * Defaults to false.
     */
    static void
    check(std::unique_ptr<FrontendContext>& context, bool repl_mode = false);
};

} // namespace nico

#endif // NICO_LOCAL_CHECKER_H
