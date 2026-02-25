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

    // TODO: Add doc comments for these functions.

    bool is_in_unsafe_context();

    std::shared_ptr<Type>
    implicit_full_dereference(std::shared_ptr<Expr>& expr);

    std::optional<Dictionary<std::string, std::weak_ptr<Expr>>>
    try_match_args_to_params(
        std::shared_ptr<Type::Function> func_type,
        const std::vector<std::shared_ptr<Expr>>& pos_args,
        const Dictionary<std::string, std::shared_ptr<Expr>>& named_args
    );

    bool check_pointer_cast(
        std::shared_ptr<Type> expr_type,
        std::shared_ptr<Type> target_type,
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
    std::shared_ptr<Type> expr_check(
        std::shared_ptr<Expr> expr,
        bool as_lvalue,
        bool allow_unsized_rvalue = false
    );

    std::shared_ptr<Type>
    annotation_check(std::shared_ptr<Annotation> annotation);
};

} // namespace nico

#endif // NICO_EXPRESSION_CHECKER_H
