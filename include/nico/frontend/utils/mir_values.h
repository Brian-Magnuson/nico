#ifndef NICO_MIR_VALUES_H
#define NICO_MIR_VALUES_H

#include "nico/frontend/utils/mir.h"

#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "nico/frontend/utils/symbol_node.h"
#include "nico/frontend/utils/type_node.h"
#include "nico/shared/token.h"

namespace nico {

/**
 * @brief A constant value in the MIR.
 *
 * "Constant" in this context, does not mean immutable, but rather that the
 * value is known at compile time. It is analogous to the concept of a
 * `constexpr` in C++ and can be used for compile-time optimizations.
 *
 * Constant values can be used to initialize global variables.
 */
class MIRValue::IConstant : public MIRValue {
public:
    IConstant(Private, std::shared_ptr<Type> type)
        : MIRValue(Private(), type) {}
};

/**
 * @brief A zero value in the MIR.
 *
 * A zero value is a constant value that represents the zero value of a given
 * type.
 */
class MIRValue::ZeroValue : public MIRValue::IConstant {
public:
    ZeroValue(Private, std::shared_ptr<Type> type)
        : MIRValue::IConstant(Private(), type) {}

    /**
     * @brief Creates a new zero value in the MIR.
     *
     * @param type The type of the zero value.
     * @return std::shared_ptr<ZeroValue> A shared pointer to the newly created
     * zero value.
     */
    static std::shared_ptr<ZeroValue> create(std::shared_ptr<Type> type) {
        return std::make_shared<ZeroValue>(Private(), type);
    }

    virtual std::string to_string() const override {
        return "(" + type->to_string() + " zerovalue)";
    }

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

/**
 * @brief A custom integer value in the MIR.
 *
 * Occasionally, it is useful for the MIR builder to create MIRValues that have
 * specific constant integer values, like 0 or 1. Since the MIR builder cannot
 * create the Expr required for an MIRValue::Literal, we can instead use this
 * class to create a constant integer value.
 */
class MIRValue::CustomInt : public MIRValue::IConstant {
public:
    // The value of the custom integer.
    const uint64_t value;

    CustomInt(Private, std::shared_ptr<Type> type, uint64_t value)
        : MIRValue::IConstant(Private(), type), value(value) {}

    static std::shared_ptr<CustomInt>
    create(std::shared_ptr<Type> type, uint64_t value) {
        return std::make_shared<CustomInt>(Private(), type, value);
    }

    virtual std::string to_string() const override {
        return "(" + type->to_string() + " " + std::to_string(value) + ")";
    }

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

/**
 * @brief A literal value in the MIR.
 *
 * Literal values reference a literal expression from the AST.
 */
class MIRValue::Literal : public MIRValue::IConstant {
public:
    // The token representing the literal value.
    std::shared_ptr<Token> token;

    Literal(Private, std::shared_ptr<Type> type, std::shared_ptr<Token> token)
        : MIRValue::IConstant(Private(), type), token(token) {}

    /**
     * @brief Creates a new literal value in the MIR.
     *
     * @param type The type of the literal value.
     * @param token The token representing the literal value.
     * @return std::shared_ptr<Literal> A shared pointer to the newly created
     * literal value.
     */
    static std::shared_ptr<Literal>
    create(std::shared_ptr<Type> type, std::shared_ptr<Token> token) {
        return std::make_shared<Literal>(Private(), type, token);
    }

    virtual std::string to_string() const override {
        return "(" + type->to_string() + " " + std::string(token->lexeme) + ")";
    }

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

/**
 * @brief A constant array value in the MIR.
 *
 * An array is constant if all of its elements are constant values.
 * If you mean to create a non-constant array, use the `Instr::Array`
 * instruction instead.
 */
class MIRValue::Array : public MIRValue::IConstant {
public:
    // The elements of the array.
    std::vector<std::shared_ptr<MIRValue::IConstant>> elements;

    Array(
        Private,
        std::shared_ptr<Type::Array> type,
        std::vector<std::shared_ptr<MIRValue::IConstant>> elements
    )
        : MIRValue::IConstant(Private(), type), elements(elements) {}

    /**
     * @brief Creates a new array value in the MIR.
     *
     * @param type The type of the array.
     * @param elements The elements of the array.
     * @return std::shared_ptr<Array> A shared pointer to the newly created
     * array value.
     */
    static std::shared_ptr<Array> create(
        std::shared_ptr<Type::Array> type,
        std::vector<std::shared_ptr<MIRValue::IConstant>> elements
    ) {
        return std::make_shared<Array>(Private(), type, elements);
    }

    virtual std::string to_string() const override {
        std::string result = "(" + type->to_string() + " [";
        for (const auto& element : elements) {
            result += " " + element->to_string();
        }
        result += " ])";
        return result;
    }

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

/**
 * @brief A constant struct value in the MIR.
 *
 * A struct is constant if all of its elements are constant values.
 * If you mean to create a non-constant struct or tuple, use the `Instr::Struct`
 * or `Instr::Tuple` instructions instead.
 */
class MIRValue::Struct : public MIRValue::IConstant {
public:
    // The fields of the struct.
    std::vector<std::shared_ptr<MIRValue::IConstant>> elements;

    Struct(
        Private,
        std::shared_ptr<Type> type,
        std::vector<std::shared_ptr<MIRValue::IConstant>> elements
    )
        : MIRValue::IConstant(Private(), type), elements(elements) {}

    /**
     * @brief Creates a new struct value in the MIR.
     *
     * @param type The type of the struct.
     * @param elements The elements of the struct.
     * @return std::shared_ptr<Struct> A shared pointer to the newly created
     * struct value.
     */
    static std::shared_ptr<Struct> create(
        std::shared_ptr<Type> type,
        std::vector<std::shared_ptr<MIRValue::IConstant>> elements
    ) {
        return std::make_shared<Struct>(Private(), type, elements);
    }

    virtual std::string to_string() const override {
        std::string result = "(" + type->to_string();
        for (const auto& element : elements) {
            result += " " + element->to_string();
        }
        result += ")";
        return result;
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

/**
 * @brief A global variable value in the MIR.
 *
 * An extension of MIRValue::Variable that represents a global variable in the
 * MIR.
 */
class MIRValue::Global : public MIRValue::Variable {
public:
    Linkage linkage;

    Global(
        Private,
        std::shared_ptr<Node::BindingEntry> binding_entry,
        Linkage linkage
    )
        : MIRValue::Variable(Private(), binding_entry), linkage(linkage) {}

    /**
     * @brief Creates a new global variable value in the MIR.
     *
     * @param binding_entry The binding entry from the AST on which this MIR
     * value is based.
     * @return std::shared_ptr<Global> A shared pointer to the newly created
     * global variable.
     */
    static std::shared_ptr<Global>
    create(std::shared_ptr<Node::BindingEntry> binding_entry) {
        return std::make_shared<Global>(
            Private(),
            binding_entry,
            binding_entry->linkage
        );
    }

    virtual std::any accept(Visitor* visitor) override {
        return visitor->visit(this);
    }
};

} // namespace nico

#endif // NICO_MIR_VALUES_H
