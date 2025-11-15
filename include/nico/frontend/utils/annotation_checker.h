#ifndef NICO_ANNOTATION_CHECKER_H
#define NICO_ANNOTATION_CHECKER_H

#include <any>
#include <memory>

#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/symbol_tree.h"
#include "nico/frontend/utils/type_node.h"

namespace nico {

/**
 * @brief A visitor for checking annotations in the AST.
 *
 * During annotation checking, annotations, which are AST nodes, are translated
 * to proper type nodes.
 *
 * Visit functions in this class return shared pointers to type nodes.
 */
class AnnotationChecker : public Annotation::Visitor {
    // The symbol tree used for type checking.
    const std::shared_ptr<SymbolTree> symbol_tree;
    // An expression visitor to use for checking expressions within typeof
    // annotations.
    Expr::Visitor* expr_visitor;

public:
    AnnotationChecker(
        std::shared_ptr<SymbolTree> symbol_tree,
        Expr::Visitor* expr_visitor = nullptr
    )
        : symbol_tree(symbol_tree), expr_visitor(expr_visitor) {};

    std::any visit(Annotation::NameRef* annotation) override;
    std::any visit(Annotation::Pointer* annotation) override;
    std::any visit(Annotation::Nullptr* annotation) override;
    std::any visit(Annotation::Reference* annotation) override;
    std::any visit(Annotation::Array* annotation) override;
    std::any visit(Annotation::Object* annotation) override;
    std::any visit(Annotation::Tuple* annotation) override;
    std::any visit(Annotation::TypeOf* annotation) override;
};

} // namespace nico

#endif // NICO_ANNOTATION_CHECKER_H
