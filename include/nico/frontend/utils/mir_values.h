#ifndef NICO_MIR_VALUES_H
#define NICO_MIR_VALUES_H

#include "nico/frontend/utils/mir.h"

#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/symbol_node.h"
#include "nico/frontend/utils/type_node.h"

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
        Private,
        std::shared_ptr<Type> type,
        std::shared_ptr<Expr::Literal> literal_expr
    )
        : MIRValue(Private(), type), literal_expr(literal_expr) {}

    /**
     * @brief Creates a new literal value in the MIR.
     *
     * @param type The type of the literal value.
     * @param literal_expr The literal expression from the AST on which this MIR
     * value is based.
     * @return std::shared_ptr<Literal> A shared pointer to the newly created
     * literal value.
     */
    static std::shared_ptr<Literal> create(
        std::shared_ptr<Type> type, std::shared_ptr<Expr::Literal> literal_expr
    ) {
        return std::make_shared<Literal>(Private(), type, literal_expr);
    }

    virtual std::string to_string() const override {
        return "(" + type->to_string() + " " +
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

    Variable(Private, std::string_view name, std::shared_ptr<Type> type)
        : MIRValue(Private(), std::make_shared<Type::RawTypedPtr>(type, true)),
          name(
              std::string(name) + "#" +
              std::to_string(mir_temp_name_counters[std::string(name)]++)
          ) {}

    Variable(Private, std::shared_ptr<Node::BindingEntry> binding_entry)
        : MIRValue(
              Private(),
              std::make_shared<Type::RawTypedPtr>(
                  binding_entry->binding.type, true
              )
          ),
          name(binding_entry->symbol) {}

    /**
     * @brief Creates a new variable value in the MIR without a binding entry.
     *
     * Variables without a binding entry are typically for internal-use
     * variables, such as function return values.
     *
     * If this function is called with a name that has been provided previously,
     * the number will be appended to the name to ensure uniqueness.
     *
     * @param name A name for the variable.
     * @param type The type of the variable.
     * @return std::shared_ptr<Variable> A shared pointer to the newly created
     * variable value.
     */
    static std::shared_ptr<Variable>
    create(std::string_view name, std::shared_ptr<Type> type) {
        return std::make_shared<Variable>(Private(), name, type);
    }

    /**
     * @brief Creates a new variable value in the MIR with a binding entry.
     *
     * The name of the variable will be based on the symbol from the binding
     * entry node.
     *
     * @param binding_entry The binding entry from the AST on which this MIR
     * value is based.
     * @return std::shared_ptr<Variable> A shared pointer to the newly created
     * variable value.
     */
    static std::shared_ptr<Variable>
    create(std::shared_ptr<Node::BindingEntry> binding_entry) {
        auto variable = std::make_shared<Variable>(Private(), binding_entry);
        binding_entry->mir_variable =
            std::dynamic_pointer_cast<MIRValue::Variable>(
                variable->shared_from_this()
            );
        return variable;
    }

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
public:
    // A name for the temporary value.
    const std::string name;

    Temporary(Private, std::shared_ptr<Type> type, std::string_view name = "")
        : MIRValue(Private(), type),
          name(
              std::string(name) + "#" +
              std::to_string(mir_temp_name_counters[std::string(name)]++)
          ) {}

    /**
     * @brief Creates a new temporary value in the MIR.
     *
     * Names for temporaries are optional.
     * If a name is not provided, a unique name will be generated based on a
     * counter. If a name is provided, a number will be appended to the name to
     * ensure uniqueness.
     *
     * @param type The type of the temporary value.
     * @param name A name for the temporary value. Defaults to an
     * empty string.
     * @return std::shared_ptr<Temporary> A shared pointer to the newly created
     * temporary value.
     */
    static std::shared_ptr<Temporary>
    create(std::shared_ptr<Type> type, std::string_view name = "") {
        return std::make_shared<Temporary>(Private(), type, name);
    }

    virtual std::string to_string() const override {
        return "(" + type->to_string() + " " + name + ")";
    }

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

} // namespace nico

#endif // NICO_MIR_VALUES_H
