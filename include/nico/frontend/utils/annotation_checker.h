#ifndef NICO_ANNOTATION_CHECKER_H
#define NICO_ANNOTATION_CHECKER_H

#include <any>
#include <memory>

#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/expression_checker.h"

namespace nico {

/**
 * @brief A helper checker for checking type annotations in the AST.
 *
 * Note: Using `AnnotationChecker::create` will create a standalone annotation
 * checker without an expression checker.
 * For complete expression checking with annotation checking, use
 * `ExpressionChecker::create` instead.
 *
 * Visit functions in this class return shared pointers to type nodes.
 */
class AnnotationChecker : public Annotation::Visitor {
    // The symbol tree used for type checking.
    std::shared_ptr<SymbolTree> symbol_tree;
    // A weak pointer to the expression checker, used for checking
    // sub-expressions in annotations.
    std::weak_ptr<ExpressionChecker> expr_checker;

    /**
     * @brief A private struct used to restrict access to constructors.
     */
    struct Private {
        explicit Private() = default;
    };

    std::any visit(Annotation::NameRef* annotation) override;
    std::any visit(Annotation::Pointer* annotation) override;
    std::any visit(Annotation::Nullptr* annotation) override;
    std::any visit(Annotation::Void* annotation) override;
    std::any visit(Annotation::Reference* annotation) override;
    std::any visit(Annotation::Array* annotation) override;
    std::any visit(Annotation::Object* annotation) override;
    std::any visit(Annotation::Tuple* annotation) override;
    std::any visit(Annotation::TypeOf* annotation) override;

public:
    virtual ~AnnotationChecker() = default;

    AnnotationChecker(Private) {};

    /**
     * @brief Constructs a new annotation checker.
     *
     * An expression checker is optional for the annotation checker, but if it
     * is not provided, `typeof` annotations will not be checkable and will
     * result in an error if encountered.
     *
     * @param symbol_tree The symbol tree to use for type checking.
     * @param expr_checker An optional weak pointer to the expression checker to
     * use for checking sub-expressions in annotations.
     * @return std::shared_ptr<AnnotationChecker> A new annotation checker
     * instance.
     */
    static std::shared_ptr<AnnotationChecker> create(
        std::shared_ptr<SymbolTree> symbol_tree,
        std::weak_ptr<ExpressionChecker> expr_checker =
            std::weak_ptr<ExpressionChecker>()
    );

    /**
     * @brief Checks the given annotation and returns its type if it is valid.
     *
     * @param annotation The annotation to check.
     * @return The type of the annotation if it is valid, or nullopt if it is
     * not valid.
     */
    std::optional<std::shared_ptr<Type>>
    annotation_check(std::shared_ptr<Annotation> annotation);
};

} // namespace nico

#endif // NICO_ANNOTATION_CHECKER_H
