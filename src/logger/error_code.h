#ifndef NICO_ERROR_CODE_H
#define NICO_ERROR_CODE_H

/**
 * @brief An error code that can be issued by the compiler.
 *
 * Error codes are named based on what the compiler *observes*, not what is disallowed.
 */
enum class Err {
    // Null error; should never be issued under normal circumstances.
    Null = 0,
    // Default error; should never be issued under normal circumstances.
    Default,

    // Configuration error.
    Config = 1000,

    // Lexer error
    Lexer = 2000,
    // An unexpected character was found in the source code.
    UnexpectedChar,
    // A closing grouping token was found without a matching opening token.
    UnclosedGrouping,
    // A line was found with left spacing consisting of both tabs and spaces.
    MixedLeftSpacing,
    // A line was found with left spacing that is inconsistent with previous lines.
    InconsistentLeftSpacing,
    // An indent was detected with an improper number of spaces.
    MalformedIndent,
    // An unexpected dot was found in a numeric literal.
    UnexpectedDotInNumber,
    // An unexpected exponent was found in a numeric literal.
    UnexpectedExpInNumber,
    // A number token is followed by an alphanumeric character.
    InvalidCharAfterNumber,

    // Parser error
    Parser = 3000,

    // Global type check error.
    GlobalType = 4000,

    // Local type check error.
    LocalType = 5000,

    // Code generation error.
    CodeGen = 6000,

    // Post-processing error.
    PostProcess = 8000,

    // Compiler malfunction error.
    Malfunction = 9000,
    // An unknown error occurred.
    Unknown,
    // Statement was reached that should be unreachable. Typically used when a series of conditional checks do not catch every case.
    Unreachable,
    // Statement was reached that should be impossible. Typically used when a function does not behave as expected.
    Impossible,
    // Statement was reached that should be unimplemented.
    Unimplemented,
    // Error for testing purposes.
    TestError
};

#endif // ERROR_CODE_H
