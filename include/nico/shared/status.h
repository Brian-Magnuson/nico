#ifndef NICO_STATUS_H
#define NICO_STATUS_H

/**
 * @brief Enum class for the status of the front end.
 */
enum class Status {
    // The front end is in a valid state and can continue processing.
    OK,
    // The front end has stopped, but can continue processing.
    Pause,
    // The front end has stopped due to an error and cannot continue.
    Error
};

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
    // The REPL should reset its state.
    Reset,
    // The REPL should exit.
    Exit,
    // The REPL should display a help message.
    Help
};

#endif // NICO_STATUS_H
