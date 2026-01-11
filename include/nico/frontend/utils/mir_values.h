#ifndef NICO_MIR_VALUES_H
#define NICO_MIR_VALUES_H

#include "nico/frontend/utils/mir.h"

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/symbol_node.h"

namespace nico {

/**
 * @brief A literal value in the MIR.
 *
 * Literal values reference a literal expression from the AST.
 */
class MIRValue::Literal : public MIRValue {
public:
    // The literal value expression.
    std::shared_ptr<Expr::Literal> literal_expr;

    Literal(
        std::shared_ptr<Type> type, std::shared_ptr<Expr::Literal> literal_expr
    )
        : MIRValue(type), literal_expr(literal_expr) {}

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

/**
 * @brief A variable value in the MIR.
 *
 * Variable values reference an entry in the symbol tree.
 */
class MIRValue::Variable : public MIRValue {
public:
    // The field entry node representing the variable.
    std::shared_ptr<Node::FieldEntry> field_entry;

    Variable(std::shared_ptr<Node::FieldEntry> field_entry)
        : MIRValue(field_entry->field.type), field_entry(field_entry) {}

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

/**
 * @brief A temporary value in the MIR.
 *
 * Temporary values are intermediate values created during code generation.
 *
 * If a name is not given, a simple unique name will be generated based on a
 * counter.
 */
class MIRValue::Temporary : public MIRValue {
    // Static counter for generating unique temporary names.
    static int temp_counter;

public:
    // A name for the temporary value.
    const std::string name;

    Temporary(
        std::shared_ptr<Type> type,
        std::optional<std::string_view> name = std::nullopt
    )
        : MIRValue(type),
          name(
              name.has_value() ? std::string(*name)
                               : "@" + std::to_string(temp_counter++)
          ) {}

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

} // namespace nico

#endif // NICO_MIR_VALUES_H
