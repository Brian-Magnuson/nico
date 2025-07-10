#ifndef NICO_IDENT_H
#define NICO_IDENT_H

#include <memory>
#include <string>
#include <vector>

#include "../common/utils.h"
#include "../lexer/token.h"

/**
 * @brief An identifier class used to represent identifiers with multiple parts.
 *
 * Ident should only be used where multi-part identifiers are allowed.
 * Multi-part identifiers are not allowed in declarations, but are in identifier expressions and annotations.
 *
 * Idents should not be compared directly as different identifiers may refer to the same thing and similar identifiers may refer to different things.
 * Instead, search for the identifier in the symbol tree and resolve it to a node.
 */
class Ident {
public:
    /**
     * @brief A part of an identifier.
     *
     * Consists of the token representing the part and a vector of arguments.
     *
     * E.g. `example::object<with, args>` would have two parts:
     * - The first part would be `example` with no arguments.
     * - The second part would be `object` with two arguments: `with` and `args`.
     */
    struct Part {
        // The token representing this part of the identifier.
        std::shared_ptr<Token> token;
        // The arguments for this part of the identifier, if any.
        std::vector<std::shared_ptr<Ident>> args;
    };

    // The parts of the identifier.
    std::vector<Part> parts;

    Ident(std::shared_ptr<Token> token)
        : parts({{token, {}}}) {}

    Ident(std::vector<Part> elements)
        : parts(elements) {
        if (parts.empty()) {
            panic("Ident::Ident: parts cannot be empty");
        }
    }

    /**
     * @brief Converts this identifier to a string representation.
     *
     * @return std::string The string representation of the identifier.
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

#endif // NICO_IDENT_H
