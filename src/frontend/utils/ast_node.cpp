#include "nico/frontend/utils/ast_node.h"

#include <string>

#include "nico/shared/error_code.h"
#include "nico/shared/logger.h"

namespace nico {

bool Stmt::IBindingDecl::apply_modifier(const Modifier& modifier) {
    // TODO: Issue errors when modifiers are applied multiple times to the same
    // declaration (e.g. multiple `linkage` modifiers).

    // Linkage modifier: specifies the linkage type for this declaration
    // (internal or external).
    if (modifier.identifier == "linkage") {
        if (linkage_opt.has_value()) {
            Logger::inst().log_error(
                Err::ModifierAlreadyApplied,
                *modifier.location,
                "Linkage modifier has already been set by a previous modifier."
            );
        }
        if (modifier.args.size() != 1 ||
            modifier.args.at(0)->tok_type != Tok::Str) {
            Logger::inst().log_error(
                Err::ModifierInvalidArguments,
                *modifier.location,
                "Modifier `linkage` requires a string literal argument "
                "specifying the linkage type (\"internal\" or \"external\")."
            );
            return false;
        }

        auto linkage_arg =
            std::any_cast<std::string>(modifier.args.at(0)->literal);

        if (linkage_arg == "internal") {
            linkage_opt = Linkage::Internal;
            return true;
        }
        if (linkage_arg == "external") {
            linkage_opt = Linkage::External;
            return true;
        }

        Logger::inst().log_error(
            Err::ModifierInvalidArguments,
            *modifier.location,
            "Invalid argument for `linkage` modifier: expected "
            "\"internal\" or \"external\", got " +
                std::string(modifier.args.at(0)->lexeme)
        );
        return false;
    }

    // Symbol modifier: specifies a custom symbol for this declaration.
    if (modifier.identifier == "symbol") {
        if (custom_symbol_opt.has_value()) {
            Logger::inst().log_error(
                Err::ModifierAlreadyApplied,
                *modifier.location,
                "Symbol modifier has already been set by a previous modifier."
            );
        }
        if (modifier.args.size() != 1 ||
            modifier.args.at(0)->tok_type != Tok::Str) {
            Logger::inst().log_error(
                Err::ModifierInvalidArguments,
                *modifier.location,
                "Modifier `symbol` requires a string literal argument "
                "specifying the custom symbol for this declaration."
            );
            return false;
        }

        custom_symbol_opt =
            std::any_cast<std::string>(modifier.args.at(0)->literal);
        return true;
    }

    return Stmt::IDeclAllowed::apply_modifier(modifier);
}

} // namespace nico
