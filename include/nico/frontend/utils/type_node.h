#ifndef NICO_TYPE_NODE_H
#define NICO_TYPE_NODE_H

#include <cinttypes>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>

#include <llvm/IR/IRBuilder.h>

#include "nico/frontend/utils/nodes.h"
#include "nico/frontend/utils/symbol_node.h"
#include "nico/shared/dictionary.h"
#include "nico/shared/utils.h"

namespace nico {

// MARK: Numeric types

/**
 * @brief A base class for all numeric types.
 *
 * Includes `Type::Int` and `Type::Float`.
 */
class Type::INumeric : virtual public Type {
public:
    virtual ~INumeric() = default;

    virtual std::string to_string() const = 0;
};

/**
 * @brief An integer type.
 *
 * Can be signed or unsigned, and can have any width.
 * To save space, the width is stored as a uint8_t.
 * Additionally, it is recommended only widths of 8, 16, 32, or 64 are used.
 */
class Type::Int : public Type::INumeric {
public:
    // Whether the integer is signed or unsigned.
    const bool is_signed;
    // The width of the integer in bits. Can be any number, but should be 8, 16,
    // 32, or 64.
    const uint8_t width;

    virtual ~Int() = default;

    Int(bool is_signed, uint8_t width)
        : is_signed(is_signed), width(width) {}

    std::string to_string() const override {
        return (is_signed ? "i" : "u") + std::to_string(width);
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_int = dynamic_cast<const Int*>(&other)) {
            return is_signed == other_int->is_signed &&
                   width == other_int->width;
        }
        return false;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        return llvm::IntegerType::get(builder->getContext(), width);
    }

    virtual std::pair<std::string, std::vector<llvm::Value*>> to_print_args(
        std::unique_ptr<llvm::IRBuilder<>>& builder,
        llvm::Value* value,
        bool include_quotes = false
    ) const override {
        const char* format_chars;
        llvm::Value* print_value = value;
        auto int_32_type = llvm::Type::getInt32Ty(builder->getContext());

        if (is_signed) {
            switch (width) {
            case 8:
                format_chars = PRId8;
                print_value = builder->CreateSExt(value, int_32_type);
                break;
            case 16:
                format_chars = PRId16;
                print_value = builder->CreateSExt(value, int_32_type);
                break;
            case 32:
                format_chars = PRId32;
                break;
            case 64:
                format_chars = PRId64;
                break;
            default:
                print_value = builder->CreateSExt(
                    value,
                    llvm::Type::getInt64Ty(builder->getContext())
                );
                format_chars =
                    "lld"; // Default to long long for non-standard widths.
                break;
            }
        }
        else {
            switch (width) {
            case 8:
                format_chars = PRIu8;
                print_value = builder->CreateZExt(value, int_32_type);
                break;
            case 16:
                format_chars = PRIu16;
                print_value = builder->CreateZExt(value, int_32_type);
            case 32:
                format_chars = PRIu32;
                break;
            case 64:
                format_chars = PRIu64;
                break;
            default:
                print_value = builder->CreateZExt(
                    value,
                    llvm::Type::getInt64Ty(builder->getContext())
                );
                format_chars = "llu"; // Default to unsigned long long for
                                      // non-standard widths.
                break;
            }
        }
        std::string format_str = "%" + std::string(format_chars);
        return {format_str, {print_value}};
    }

    /**
     * @brief Returns the maximum value for this integer type and stores it in
     * an unsigned 64-bit integer.
     *
     * @return The maximum value for this integer type.
     *
     * @warning Will panic if the width is not one of the supported widths (8,
     * 16, 32, 64).
     */
    uint64_t get_max_value() const {
        if (is_signed) {
            switch (width) {
            case 8:
                return 127;
            case 16:
                return 32767;
            case 32:
                return 2147483647;
            case 64:
                return 9223372036854775807ULL;
            default:
                panic("Type::Int::get_max_value: Unsupported width");
            }
        }
        else {
            switch (width) {
            case 8:
                return 255;
            case 16:
                return 65535;
            case 32:
                return 4294967295U;
            case 64:
                return 18446744073709551615ULL;
            default:
                panic("Type::Int::get_max_value: Unsupported width");
            }
        }
    }

    /**
     * @brief Returns the minimum value for this integer type.
     *
     * @return The minimum value for this integer type.
     *
     * @warning Will panic if the width is not one of the supported widths (8,
     * 16, 32, 64).
     */
    int64_t get_min_value() const {
        if (is_signed) {
            switch (width) {
            case 8:
                return -128;
            case 16:
                return -32768;
            case 32:
                return -2147483648;
            case 64:
                return -9223372036854775807LL - 1;
            default:
                panic("Type::Int::get_min_value: Unsupported width");
            }
        }
        else {
            return 0;
        }
    }
};

/**
 * @brief A floating-point type.
 *
 * Can be 32 or 64 bits wide.
 */
class Type::Float : public Type::INumeric {
public:
    // The width of the float in bits. Can be 32 or 64.
    const uint8_t width;

    virtual ~Float() = default;

    Float(uint8_t width)
        : width(width) {
        if (width != 32 && width != 64) {
            panic(
                "Type::Float: Invalid width " + std::to_string(width) +
                ". Must be 32 or 64."
            );
        }
    }

    std::string to_string() const override {
        return "f" + std::to_string(width);
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_float = dynamic_cast<const Float*>(&other)) {
            return width == other_float->width;
        }
        return false;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        switch (width) {
        case 32:
            return llvm::Type::getFloatTy(builder->getContext());
        case 64:
            return llvm::Type::getDoubleTy(builder->getContext());
        default:
            panic(
                "Type::Float: Invalid width " + std::to_string(width) +
                ". Must be 32 or 64."
            );
        }
    }

    virtual std::pair<std::string, std::vector<llvm::Value*>> to_print_args(
        std::unique_ptr<llvm::IRBuilder<>>& builder,
        llvm::Value* value,
        bool include_quotes = false
    ) const override {
        llvm::Value* print_value = value;
        if (width == 32) {
            print_value = builder->CreateFPExt(
                value,
                llvm::Type::getDoubleTy(builder->getContext())
            );
        }
        return {"%g", {print_value}};
    }
};

// MARK: Boolean type

/**
 * @brief A boolean type.
 *
 * Boolean types have no additional state as there is no need;
 * All boolean types are the same.
 * In LLVM, booleans may be represented as an integer 1 bit wide (`i1`).
 */
class Type::Bool : public Type {
public:
    virtual ~Bool() = default;

    std::string to_string() const override { return "bool"; }

    bool operator==(const Type& other) const override {
        return dynamic_cast<const Bool*>(&other) != nullptr;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        return llvm::Type::getInt1Ty(builder->getContext());
    }

    virtual std::pair<std::string, std::vector<llvm::Value*>> to_print_args(
        std::unique_ptr<llvm::IRBuilder<>>& builder,
        llvm::Value* value,
        bool include_quotes = false
    ) const override {
        // Convert the boolean to an integer for printing.
        auto bool_value = builder->CreateSelect(
            value,
            builder->CreateGlobalStringPtr("true"),
            builder->CreateGlobalStringPtr("false")
        );
        return {"%s", {bool_value}};
    }
};

// MARK: Pointer types

class Type::IPointer : public Type {
public:
    // The type that the pointer points to.
    const std::shared_ptr<Type> base;
    // Whether object pointed to by this pointer is mutable.
    bool is_mutable;

    virtual ~IPointer() = default;

    IPointer(std::shared_ptr<Type> base, bool is_mutable)
        : base(base), is_mutable(is_mutable) {}

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        return llvm::PointerType::get(builder->getContext(), 0);
    }
};

/**
 * @brief A pointer type.
 *
 * Points to another type.
 * Note: Since LLVM 15, pointers do not store type information.
 * Keep this in mind before converting to the LLVM type.
 */
class Type::RawPointer : public Type::IPointer {
public:
    virtual ~RawPointer() = default;

    RawPointer(std::shared_ptr<Type> base, bool is_mutable)
        : IPointer(base, is_mutable) {}

    std::string to_string() const override {
        return std::string(is_mutable ? "var" : "") + "@" + base->to_string();
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_pointer =
                dynamic_cast<const RawPointer*>(&other)) {
            return *base == *other_pointer->base &&
                   is_mutable == other_pointer->is_mutable;
        }
        return false;
    }

    virtual std::pair<std::string, std::vector<llvm::Value*>> to_print_args(
        std::unique_ptr<llvm::IRBuilder<>>& builder,
        llvm::Value* value,
        bool include_quotes = false
    ) const override {
        return {"%p", {value}};
    }

    virtual bool is_assignable_to(const Type& other) const override {
        if (const auto* other_pointer =
                dynamic_cast<const RawPointer*>(&other)) {
            // You can assign to a pointer if the base types are the same and
            // the mutability is compatible.

            // not (not this->is_mutable and target->is_mutable)
            // !(!A & B) == A | !B
            return base->is_assignable_to(*other_pointer->base) &&
                   (is_mutable || !other_pointer->is_mutable);
            // If this pointer is mutable or the other pointer is not mutable,
            // we either have an equivalent type or a loss of mutability.
            // We explicitly allow a loss of mutability.
        }
        return false;
    }
};

/**
 * @brief A null pointer type.
 *
 * The type and its only value are both written as `nullptr`.
 * It extends the pointer type and behaves similarly, except it has no base type
 * and can be assigned to any pointer type.
 * It is only considered equal to other `Nullptr` types.
 */
class Type::Nullptr : public Type::RawPointer {
public:
    virtual ~Nullptr() = default;

    Nullptr()
        : Type::RawPointer(nullptr, false) {}

    std::string to_string() const override { return "nullptr"; }

    bool operator==(const Type& other) const override {
        return dynamic_cast<const Nullptr*>(&other) != nullptr;
    }

    virtual bool is_assignable_to(const Type& other) const override {
        // nullptr can be assigned to any pointer type.
        return dynamic_cast<const Type::RawPointer*>(&other) != nullptr;
    }
};

/**
 * @brief A reference type.
 *
 * References are pointers with special semantics.
 * Note: Since LLVM 15, pointers do not store type information.
 * Keep this in mind before converting to the LLVM type.
 */
class Type::Reference : public Type::IPointer {
public:
    // Whether object pointed to by this reference is mutable.
    bool is_mutable;
    // The type that the reference points to.
    const std::shared_ptr<Type> base;

    virtual ~Reference() = default;

    Reference(std::shared_ptr<Type> base, bool is_mutable)
        : IPointer(base, is_mutable) {}

    std::string to_string() const override {
        return std::string(is_mutable ? "var" : "") + "&" + base->to_string();
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_reference =
                dynamic_cast<const Reference*>(&other)) {
            return *base == *other_reference->base &&
                   is_mutable == other_reference->is_mutable;
        }
        return false;
    }

    virtual std::pair<std::string, std::vector<llvm::Value*>> to_print_args(
        std::unique_ptr<llvm::IRBuilder<>>& builder,
        llvm::Value* value,
        bool include_quotes = false
    ) const override {
        auto val = builder->CreateLoad(base->get_llvm_type(builder), value);
        return base->to_print_args(builder, val, include_quotes);
    }

    virtual bool is_assignable_to(const Type& other) const override {
        if (const auto* other_pointer =
                dynamic_cast<const Reference*>(&other)) {
            return base->is_assignable_to(*other_pointer->base) &&
                   (is_mutable || !other_pointer->is_mutable);
        }
        return false;
    }
};

/**
 * @brief A primitive string type.
 *
 * The primitive string type is a pointer to a sequence of characters in static
 * memory. Primitive strings are immutable and live for as long as the program
 * runs.
 *
 * It is similar to the `char *` type in C, but is kept a separate type for
 * safety purposes such as to prevent pointer casting.
 */
class Type::Str : public Type {
public:
    virtual ~Str() = default;

    std::string to_string() const override { return "str"; }

    bool operator==(const Type& other) const override {
        return dynamic_cast<const Str*>(&other) != nullptr;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        return llvm::PointerType::get(builder->getContext(), 0);
    }

    virtual std::pair<std::string, std::vector<llvm::Value*>> to_print_args(
        std::unique_ptr<llvm::IRBuilder<>>& builder,
        llvm::Value* value,
        bool include_quotes = false
    ) const override {
        if (include_quotes) {
            return {"\"%s\"", {value}};
        }
        else {
            return {"%s", {value}};
        }
    }
};

// MARK: Aggregate types

/**
 * @brief An array type.
 *
 * Contains a base type and a size.
 */
class Type::Array : public Type {
public:
    // The type of the elements in the array.
    const std::shared_ptr<Type> base;
    // The number of elements in the array.
    const std::optional<size_t> size;

    virtual ~Array() = default;

    Array(std::shared_ptr<Type> base)
        : base(base), size(std::nullopt) {}

    Array(std::shared_ptr<Type> base, size_t size)
        : base(
              size == 0 ? std::dynamic_pointer_cast<Type>(
                              std::make_shared<Type::Unit>()
                          )
                        : base
          ),
          size(size) {}

    std::string to_string() const override {
        if (size.has_value() && size.value() == 0) {
            return "[]";
        }
        return "[" + base->to_string() + "; " +
               (size ? std::to_string(*size) : "?") + "]";
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_array = dynamic_cast<const Array*>(&other)) {
            return *base == *other_array->base && size == other_array->size;
        }
        return false;
    }

    virtual bool is_assignable_to(const Type& other) const override {
        if (const auto* other_array = dynamic_cast<const Array*>(&other)) {
            bool sizes_compatible = false;
            /*
            We allow assignment in these cases:
            1. Both arrays have the same size.
            2. `other` is an unsized array.
            */
            if (size.has_value() && other_array->size.has_value()) {
                sizes_compatible = size.value() == other_array->size.value();
            }
            else if (!other_array->size.has_value()) {
                sizes_compatible = true;
            }

            return base->is_assignable_to(*other_array->base) &&
                   sizes_compatible;
        }
        return false;
    }

    virtual bool is_sized_type() const override {
        return size.has_value() && base->is_sized_type();
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        if (size.has_value()) {
            // Sized array, [T; N]
            return llvm::ArrayType::get(base->get_llvm_type(builder), *size);
        }
        else {
            // Unsized array, [T, ?]
            return llvm::ArrayType::get(base->get_llvm_type(builder), 0);
            // Base is always set for unsized arrays, because the only way to
            // create an array without a base is to write the literal `[]`,
            // which is a sized array of size 0.
        }
    }

    virtual std::pair<std::string, std::vector<llvm::Value*>> to_print_args(
        std::unique_ptr<llvm::IRBuilder<>>& builder,
        llvm::Value* value,
        bool include_quotes = false
    ) const override {
        if (!size.has_value()) {
            return {"[array]", {}};
        }
        std::string format_str = "[";
        std::vector<llvm::Value*> args;
        for (size_t i = 0; i < size.value(); ++i) {
            auto [fmt, vals] = base->to_print_args(
                builder,
                builder->CreateExtractValue(value, {static_cast<unsigned>(i)}),
                true
            );
            format_str += fmt;
            args.insert(args.end(), vals.begin(), vals.end());
            if (i < size.value() - 1) {
                format_str += ", ";
            }
        }
        format_str += "]";
        return {format_str, args};
    }
};

/**
 * @brief An empty array type.
 *
 * Represents an array with zero elements and an unspecified base type.
 * It is written as `[]` and its only value is also `[]`.
 * It can be assigned to any array type with size 0, regardless of base type.
 */
class Type::EmptyArray : public Type::Array {
public:
    virtual ~EmptyArray() = default;

    EmptyArray()
        : Type::Array(nullptr, 0) {}

    std::string to_string() const override { return "[]"; }

    bool operator==(const Type& other) const override {
        return dynamic_cast<const EmptyArray*>(&other) != nullptr;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        return llvm::ArrayType::get(
            llvm::Type::getInt8Ty(builder->getContext()),
            0
        );
    }

    virtual bool is_assignable_to(const Type& other) const override {
        if (const auto* other_array = dynamic_cast<const Array*>(&other)) {
            return other_array->size.has_value() &&
                   other_array->size.value() == 0;
        }
        return false;
    }
};

/**
 * @brief A tuple type.
 *
 * Used to represent a fixed-size collection of types.
 */
class Type::Tuple : public Type {
public:
    // The types of the elements in the tuple.
    const std::vector<std::shared_ptr<Type>> elements;

    virtual ~Tuple() = default;

    Tuple(std::vector<std::shared_ptr<Type>> elements)
        : elements(std::move(elements)) {}

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

    bool is_sized_type() const override {
        for (const auto& element : elements) {
            if (!element->is_sized_type()) {
                return false;
            }
        }
        return true;
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_tuple = dynamic_cast<const Tuple*>(&other)) {
            if (elements.size() != other_tuple->elements.size()) {
                return false;
            }
            for (size_t i = 0; i < elements.size(); ++i) {
                if (*(elements[i]) != *(other_tuple->elements[i])) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    virtual bool is_assignable_to(const Type& other) const override {
        if (const auto* other_tuple = dynamic_cast<const Tuple*>(&other)) {
            if (elements.size() != other_tuple->elements.size()) {
                return false;
            }
            for (size_t i = 0; i < elements.size(); ++i) {
                if (!elements[i]->is_assignable_to(
                        *(other_tuple->elements[i])
                    )) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        std::vector<llvm::Type*> element_types;
        for (const auto& element : elements) {
            element_types.push_back(element->get_llvm_type(builder));
        }
        return llvm::StructType::get(builder->getContext(), element_types);
    }

    virtual std::pair<std::string, std::vector<llvm::Value*>> to_print_args(
        std::unique_ptr<llvm::IRBuilder<>>& builder,
        llvm::Value* value,
        bool include_quotes = false
    ) const override {
        std::string format_str = "(";
        std::vector<llvm::Value*> args;
        for (unsigned i = 0; i < elements.size(); ++i) {
            auto [fmt, vals] = elements[i]->to_print_args(
                builder,
                builder->CreateExtractValue(value, {i}),
                true
            );
            format_str += fmt;
            args.insert(args.end(), vals.begin(), vals.end());
            if (i < elements.size() - 1) {
                format_str += ", ";
            }
        }
        format_str += ")";
        return {format_str, args};
    }
};

/**
 * @brief A unit type.
 *
 * A unit type is a special tuple type that has no elements and is equivalent to
 * a tuple type with no elements. It is written as `()` and named "unit" because
 * it has only one possible value, which is `()`. It may be used to represent
 * the absence of a type, like `void` in other languages.
 *
 * This class does not override `Type::Tuple::operator==` and, thus, will appear
 * equal to other instances of `Type::Tuple` that have no elements. That is to
 * say, `Type::Unit` may be used interchangably with `Type::Tuple` with no
 * elements.
 */
class Type::Unit : public Type::Tuple {
public:
    virtual ~Unit() = default;

    Unit()
        : Tuple({}) {}

    std::string to_string() const override { return "()"; }

    llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        return llvm::StructType::get(builder->getContext());
    }

    std::pair<std::string, std::vector<llvm::Value*>> to_print_args(
        std::unique_ptr<llvm::IRBuilder<>>& builder,
        llvm::Value* value,
        bool include_quotes = false
    ) const override {
        return {"()", {}};
    }
};

/**
 * @brief An object type.
 *
 * Used to represent objects with properties.
 */
class Type::Object : public Type {
public:
    // The fields of the object.
    Dictionary<std::string, Field> properties;

    virtual ~Object() = default;

    Object() = default;

    std::string to_string() const override {
        std::string result = "{";
        for (const auto& [key, value] : properties) {
            result += value.to_string() + ", ";
        }
        if (properties.size() > 0) {
            result.pop_back();
            result.pop_back();
        }
        result += "}";
        return result;
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_object = dynamic_cast<const Object*>(&other)) {
            return properties == other_object->properties;
        }
        return false;
    }

    bool is_sized_type() const override {
        for (const auto& [key, value] : properties) {
            if (!value.type->is_sized_type()) {
                return false;
            }
        }
        return true;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        std::vector<llvm::Type*> field_types;
        for (const auto& [key, value] : properties) {
            field_types.push_back(value.type->get_llvm_type(builder));
        }
        return llvm::StructType::get(builder->getContext(), field_types);
    }
};

// MARK: Callable types

class Type::ICallable : public Type {
public:
    virtual ~ICallable() = default;

    virtual std::pair<std::string, std::vector<llvm::Value*>> to_print_args(
        std::unique_ptr<llvm::IRBuilder<>>& builder,
        llvm::Value* value,
        bool include_quotes = false
    ) const override {
        return {"[function]", {}};
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        return llvm::PointerType::get(builder->getContext(), 0);
    }
};

/**
 * @brief A function type.
 *
 * Used to represent functions with parameters and return types.
 *
 * Note: Functions are special kinds of pointers. They cannot be directly
 * dereferenced, but they can be passed around and called.
 */
class Type::Function : public Type::ICallable {
    // The set of all parameter strings for this function; used for overload
    // conflict resolution; lazily initialized.
    mutable std::optional<std::unordered_set<std::string>> param_strings;
    // The set of all default parameter strings for this function; used for
    // overload conflict resolution; lazily initialized.
    mutable std::optional<std::unordered_set<std::string>>
        default_param_strings;

public:
    // The parameters of the function.
    Dictionary<std::string, Field> parameters;
    // The return type of the function.
    const std::shared_ptr<Type> return_type;

    virtual ~Function() = default;

    Function(
        Dictionary<std::string, Field> parameters,
        std::shared_ptr<Type> return_type
    )
        : parameters(std::move(parameters)),
          return_type(std::move(return_type)) {}

    std::string to_string() const override {
        std::string result = "func(";
        for (const auto& param : parameters) {
            result += param.second.to_string() + ", ";
        }
        if (!parameters.empty()) {
            result.pop_back();
            result.pop_back();
        }
        result += ") -> " + return_type->to_string();
        return result;
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_function =
                dynamic_cast<const Function*>(&other)) {
            return parameters == other_function->parameters &&
                   *return_type == *other_function->return_type;
        }
        return false;
    }

    llvm::FunctionType*
    get_llvm_function_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const {
        std::vector<llvm::Type*> param_types;
        for (const auto& param : parameters) {
            param_types.push_back(param.second.type->get_llvm_type(builder));
        }
        llvm::Type* return_llvm_type = return_type->get_llvm_type(builder);
        return llvm::FunctionType::get(return_llvm_type, param_types, false);
    }

    /**
     * @brief Get the sets of parameter strings for this function.
     *
     * These sets are used for overload conflict resolution.
     * The first set is the set of all parameter strings.
     * The second set is the set of all parameter strings for parameters with
     * default values.
     *
     * These sets are lazily initialized and cached for future use.
     *
     * @return The pair of parameter string set references (see description).
     */
    std::
        pair<std::unordered_set<std::string>&, std::unordered_set<std::string>&>
        get_param_sets() {
        if (!param_strings.has_value() || !default_param_strings.has_value()) {
            param_strings.emplace();
            default_param_strings.emplace();
            for (const auto& [name, field] : parameters) {
                auto param_str = name + ": " + field.type->to_string();
                param_strings->insert(param_str);
                if (field.default_expr.has_value()) {
                    default_param_strings->insert(param_str);
                }
            }
        }
        return {*param_strings, *default_param_strings};
    }
};

/**
 * @brief A special function type representing an overloaded function.
 *
 * Used to represent a group of overloaded functions.
 *
 * When compared against any type, it is always considered not equal.
 *
 * Cannot be converted to an LLVM type, as overloaded functions must be
 * resolved to a specific function before code generation.
 */
class Type::OverloadedFn : public Type::ICallable {
public:
    // The overload group this overloaded function belongs to.
    std::weak_ptr<Node::OverloadGroup> overload_group;

    virtual ~OverloadedFn() = default;

    OverloadedFn() = default;

    std::string to_string() const override { return "overloadedfn"; }

    bool operator==(const Type& other) const override { return false; }
};

// MARK: Special types

/**
 * @brief A named type.
 *
 * Used to represent types that have a name, such as complex types and aliased
 * types.
 *
 * Named types must point to a node in the symbol tree that is an instance of
 * Node::ITypeNode to be considered resolved. When converted to a string, the
 * unique name of the node is used.
 */
class Type::Named : public Type {
public:
    // The node associated with this named type; uses a weak pointer to avoid
    // circular references.
    const std::weak_ptr<Node::ITypeNode> node;

    virtual ~Named() = default;

    Named(std::weak_ptr<Node::ITypeNode> node)
        : node(node) {
        if (node.expired()) {
            panic("Type::Named: Node cannot be null.");
        }
    }

    std::string to_string() const override {
        if (auto node_ptr = node.lock()) {
            return node_ptr->symbol;
        }
        return "<expired>";
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_named = dynamic_cast<const Named*>(&other)) {
            return node.lock() == other_named->node.lock();
        }
        return false;
    }

    virtual llvm::Type*
    get_llvm_type(std::unique_ptr<llvm::IRBuilder<>>& builder) const override {
        if (auto node_ptr = node.lock()) {
            llvm::StructType* struct_ty = llvm::StructType::getTypeByName(
                builder->getContext(),
                node_ptr->symbol
            );
            if (!struct_ty) {
                struct_ty = llvm::StructType::create(
                    builder->getContext(),
                    node_ptr->symbol
                );
            }
            return struct_ty;
        }
        panic("Type::Named: Node is expired; LLVM type cannot be generated.");
    }
};

} // namespace nico

#endif // NICO_TYPE_NODE_H
