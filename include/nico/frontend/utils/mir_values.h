#ifndef NICO_MIR_VALUES_H
#define NICO_MIR_VALUES_H

#include "nico/frontend/utils/mir.h"

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

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

    virtual std::string to_string() const override {
        return "(" + type->to_string() +
               std::string(literal_expr->token->lexeme) + ")";
    }

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
    // A name for the variable.
    std::string name;
    // The field entry node representing the variable.
    std::shared_ptr<Node::FieldEntry> field_entry;

    Variable(std::shared_ptr<Node::FieldEntry> field_entry)
        : MIRValue(field_entry->field.type),
          name(field_entry->symbol),
          field_entry(field_entry) {}

    virtual std::string to_string() const override {
        return "(" + type->to_string() + " " + name + ")";
    }

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
    // A static map to keep track of temporary name counters for unique naming.
    static std::unordered_map<std::string, int> mir_temp_name_counters;

public:
    // A name for the temporary value.
    const std::string name;

    Temporary(
        std::shared_ptr<Type> type,
        std::optional<std::string_view> name = std::nullopt
    )
        : MIRValue(type),
          name(
              name.has_value()
                  ? std::string(*name) + "#" +
                        std::to_string(
                            mir_temp_name_counters[std::string(*name)]++
                        )
                  : std::string("#") +
                        std::to_string(
                            mir_temp_name_counters[std::string("")]++
                        )
          ) {}

    virtual std::string to_string() const override {
        return "(" + type->to_string() + " " + name + ")";
    }

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

} // namespace nico

#endif // NICO_MIR_VALUES_H
