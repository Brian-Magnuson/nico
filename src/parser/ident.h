#ifndef NICO_IDENT_H
#define NICO_IDENT_H

#include "../lexer/token.h"
#include <memory>
#include <string>
#include <vector>

class Ident {
public:
    struct Part {
        std::shared_ptr<Token> token;
        std::vector<std::shared_ptr<Ident>> args;
    };

    std::vector<Part> parts;

    Ident(std::shared_ptr<Token> token)
        : parts({{token, {}}}) {}

    Ident(std::vector<Part> elements)
        : parts(elements) {}

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
