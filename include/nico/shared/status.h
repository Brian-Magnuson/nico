#ifndef NICO_STATUS_H
#define NICO_STATUS_H

#include <variant>

namespace nico {

/**
 * @brief Macro to check if an std::variant holds a specific type.
 */
#define IS_VARIANT(variant, type) std::holds_alternative<type>(variant)

/**
 * @brief Macro to get a pointer to the value of a specific type in an
 * std::variant and store it in a new variable.
 */
#define WITH_VARIANT(variant, type, variable)                                  \
    auto variable = std::get_if<type>(&variant)

/**
 * @brief Enum class for requests from the REPL.
 *
 * When the REPL status is `Pause`, the REPL can use this enum to indicate
 * how it wants to proceed.
 */
enum class Request {
    // No request; the REPL should simply continue.
    None,
    // The REPL needs more input from the user.
    Input,
    // The REPL should discard the last input.
    Discard,
    // The REPL should discard the last input, warning the user that some
    // statements were processed.
    DiscardWarn,
};

struct Status {
    struct Ok {};
    struct Error {};
    struct Pause {
        Request request;
        Pause(Request req)
            : request(req) {}
    };
};

using VariantStatus = std::variant<Status::Ok, Status::Error, Status::Pause>;

} // namespace nico

#endif // NICO_STATUS_H
