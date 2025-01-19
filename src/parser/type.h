#ifndef NICO_TYPE_H
#define NICO_TYPE_H

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "dictionary.h"

/**
 * @brief A base class for all types.
 */
class Type {
public:
    class Int;
    class Float;
    class Pointer;
    class Array;

    class NamedStruct;
    class Struct;

    Type() = default;
    virtual ~Type() = default;
};

/**
 * @brief An integer type.
 *
 * Can be signed or unsigned, and can have any width.
 * To save space, the width is stored as a uint8_t.
 * Additionally, it is recommended only widths of 8, 16, 32, or 64 are used.
 */
class Type::Int : public Type {
public:
    // Whether the integer is signed or unsigned.
    bool is_signed;
    // The width of the integer in bits. Can be any number, but should be 8, 16, 32, or 64.
    uint8_t width;

    Int(bool is_signed, uint8_t width) : is_signed(is_signed), width(width) {}
};

/**
 * @brief A floating-point type.
 *
 * Can be 32 or 64 bits wide.
 */
class Type::Float : public Type {
public:
    // The width of the float in bits. Can be 32 or 64.
    uint8_t width;

    Float(uint8_t width) : width(width) {
        if (width != 32 && width != 64) {
            std::cerr << "Type::Float: Invalid width " << width << ". Must be 32 or 64." << std::endl;
            std::abort();
        }
    }
};

/**
 * @brief A pointer type.
 *
 * Points to another type.
 * Note: Since LLVM 15, pointers do not store type information.
 * Keep this in mind before converting to the LLVM type.
 */
class Type::Pointer : public Type {
public:
    // The type that the pointer points to.
    std::shared_ptr<Type> base;

    Pointer(std::shared_ptr<Type> base) : base(base) {}
};

/**
 * @brief An array type.
 *
 * Contains a base type and a size.
 */
class Type::Array : public Type {
public:
    // The type of the elements in the array.
    std::shared_ptr<Type> base;
    // The number of elements in the array.
    size_t size;
    // Whether the array has a known size.
    bool is_sized;

    Array(std::shared_ptr<Type> base) : base(base), size(0), is_sized(false) {}

    Array(std::shared_ptr<Type> base, size_t size) : base(base), size(size), is_sized(true) {}
};

/**
 * @brief A named struct type.
 *
 * Named struct types are types that have a name.
 * They are usually not useful on their own, but are used to reference a struct defined elsewhere.
 *
 * Note: Since this language prefers nominal typing, use `NamedStruct` when possible.
 */
class Type::NamedStruct : public Type {
public:
    // The name of the struct type.
    std::string name;

    NamedStruct(const std::string& name) : name(name) {}
};

/**
 * @brief A struct type.
 *
 * Unlike `Type::NamedStruct`, this type contains the fields of the struct.
 * Since this language prefers nominal typing, use `NamedStruct` when possible.
 */
class Type::Struct : public Type {
public:
    // The fields of the struct.
    Dictionary<std::string, std::shared_ptr<Type>> fields;

    Struct() = default;
};

#endif // NICO_TYPE_H
