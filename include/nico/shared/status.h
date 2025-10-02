#ifndef NICO_STATUS_H
#define NICO_STATUS_H

/**
 * @brief Enum class for the status of the front end.
 */
enum class Status {
    // The front end is in a valid state and can continue processing.
    OK,
    // The front end cannot continue because the input is incomplete.
    Pause,
    // The front end has encountered an error and cannot continue.
    Error
};

#endif // NICO_STATUS_H
