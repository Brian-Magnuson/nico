#ifndef NICO_ANNOTATION_H
#define NICO_ANNOTATION_H

#include <memory>

#include "dictionary.h"
#include "ident.h"

class Annotation {
public:
    class Named;

    class Pointer;
    class Reference;

    class Array;
    class Object;
    class Tuple;

    virtual ~Annotation() = default;

    virtual std::string to_string() const = 0;
};

class Annotation::Named : public Annotation {
public:
    Ident ident;

    Named(Ident ident) : ident(std::move(ident)) {}

    std::string to_string() const {
        return ident.to_string();
    }
};

class Annotation::Pointer : public Annotation {
public:
    std::shared_ptr<Annotation> base;
    bool is_mutable;

    Pointer(std::shared_ptr<Annotation> base, bool is_mutable = false)
        : base(std::move(base)), is_mutable(is_mutable) {}

    std::string to_string() const {
        return (is_mutable ? "var " : "") + std::string("*") + base->to_string();
    }
};

class Annotation::Reference : public Annotation {
public:
    std::shared_ptr<Annotation> base;
    bool is_mutable;

    Reference(std::shared_ptr<Annotation> base, bool is_mutable = false)
        : base(std::move(base)), is_mutable(is_mutable) {}

    std::string to_string() const {
        return (is_mutable ? "var " : "") + std::string("&") + base->to_string();
    }
};

class Annotation::Array : public Annotation {
public:
    std::shared_ptr<Annotation> base;
    size_t size;
    bool is_sized;

    Array(std::shared_ptr<Annotation> base, size_t size = 0, bool is_sized = false)
        : base(std::move(base)), size(size), is_sized(is_sized) {}
    std::string to_string() const {
        return "[" + base->to_string() + (is_sized ? "; " + std::to_string(size) : "") + "]";
    }
};

class Annotation::Object : public Annotation {
public:
    Dictionary<std::string, std::shared_ptr<Annotation>> properties;

    Object(Dictionary<std::string, std::shared_ptr<Annotation>> properties)
        : properties(std::move(properties)) {}

    std::string to_string() const {
        std::string result = "{";
        for (const auto& [key, value] : properties) {
            result += key + ": " + value->to_string() + ", ";
        }
        if (!properties.empty()) {
            result.pop_back();
            result.pop_back();
        }
        result += "}";

        return result;
    }
};

class Annotation::Tuple : public Annotation {
public:
    std::vector<std::shared_ptr<Annotation>> elements;

    Tuple(std::vector<std::shared_ptr<Annotation>> elements)
        : elements(std::move(elements)) {}

    std::string to_string() const {
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
};

#endif // NICO_ANNOTATION_H
