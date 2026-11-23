#include "nico_core/frontend/utils/ast_node.h"

#include <string>

#include "nico_core/shared/diagnostics.h"
#include "nico_core/shared/error_code.h"

namespace nico {

bool Stmt::IBindingDecl::apply_modifier(const Modifier& modifier) {
    // Linkage modifier: specifies the linkage type for this declaration
    // (internal or external).
    if (modifier.identifier == "linkage") {
        if (linkage_opt.has_value()) {
            Diagnostics::inst().emit_error(
                Err::ModifierAlreadyApplied,
                "Linkage modifier has already been set by a previous modifier.",
                modifier.location
            );
        }
        if (modifier.args.size() != 1 ||
            modifier.args.at(0)->tok_type != Tok::Str) {
            Diagnostics::inst().emit_error(
                Err::ModifierInvalidArguments,
                "Modifier `linkage` requires a string literal argument "
                "specifying the linkage type (\"internal\" or \"external\").",
                modifier.location
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

        Diagnostics::inst().emit_error(
            Err::ModifierInvalidArguments,
            "Invalid argument for `linkage` modifier: expected "
            "\"internal\" or \"external\", got " +
                std::string(modifier.args.at(0)->lexeme),
            modifier.location
        );
        return false;
    }

    // Symbol modifier: specifies a custom symbol for this declaration.
    if (modifier.identifier == "symbol") {
        if (custom_symbol_opt.has_value()) {
            Diagnostics::inst().emit_error(
                Err::ModifierAlreadyApplied,
                "Symbol modifier has already been set by a previous modifier.",
                modifier.location
            );
        }
        if (modifier.args.size() != 1 ||
            modifier.args.at(0)->tok_type != Tok::Str) {
            Diagnostics::inst().emit_error(
                Err::ModifierInvalidArguments,
                "Modifier `symbol` requires a string literal argument "
                "specifying the custom symbol for this declaration.",
                modifier.location
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
