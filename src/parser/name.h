#ifndef NICO_NAME_H
#define NICO_NAME_H

#include <memory>
#include <string>
#include <vector>

#include "../common/utils.h"
#include "../lexer/token.h"

/**
 * @brief A name class used to represent names with multiple parts.
 *
 * Name should only be used where multi-part names are allowed.
 * Multi-part names are not allowed in declarations, but are in name expressions
 * and annotations.
 *
 * Names should not be compared directly as different names may refer to the
 * same thing and similar names may refer to different things. Instead, search
 * for the name in the symbol tree and resolve it to a node.
 */
class Name {
public:
    /**
     * @brief A part of a name.
     *
     * Consists of the token representing the part and a vector of arguments.
     *
     * E.g. `example::object<with, args>` would have two parts:
     * - The first part would be `example` with no arguments.
     * - The second part would be `object` with two arguments: `with` and
     * `args`.
     */
    struct Part {
        // The token representing this part of the name.
        std::shared_ptr<Token> token;
        // The arguments for this part of the name, if any.
        std::vector<std::shared_ptr<Name>> args;
    };

    // The parts of the name.
    std::vector<Part> parts;

    Name(std::shared_ptr<Token> token) : parts({{token, {}}}) {}

    Name(std::vector<Part> elements) : parts(elements) {
        if (parts.empty()) {
            panic("Name::Name: parts cannot be empty");
        }
    }

    /**
     * @brief Converts this name to a string representation.
     *
     * @return std::string The string representation of the name.
     */
    std::string to_string() const {
        std::string result = "";

        // example::object<with, args>
        for (size_t i = 0; i < parts.size(); ++i) {
            const auto& part = parts[i];
            result += part.token->lexeme;
            if (!part.args.empty()) {
                result += "<";
                for (size_t j = 0; j < part.args.size(); ++j) {
                    result += part.args[j]->to_string();
                    if (j < part.args.size() - 1) {
                        result += ", ";
                    }
                }
                result += ">";
            }
            if (i < parts.size() - 1) {
                result += "::";
            }
        }
        return result;
    }
};

#endif // NICO_NAME_H
