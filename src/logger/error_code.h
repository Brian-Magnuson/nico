#ifndef NICO_ERROR_CODE_H
#define NICO_ERROR_CODE_H

/**
 * @brief An error code that can be issued by the compiler.
 *
 * Error codes are named based on what the compiler *observes*, not what is
 * disallowed.
 */
enum class Err {
    // Null error; may be issued if there is no error.
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
    // A multi-line comment was not closed at the end of the file.
    UnclosedComment,
    // A token was found to close a multi-line comment without an opening token.
    ClosingUnopenedComment,
    // A line was found with left spacing consisting of both tabs and spaces.
    MixedLeftSpacing,
    // A line was found with left spacing that is inconsistent with previous
    // lines.
    InconsistentLeftSpacing,
    // An indent was detected with an improper number of spaces.
    MalformedIndent,
    // An attempt was made to parse a number that would require too many bits to
    // represent.
    NumberOutOfRange,
    // An unexpected dot was found in a numeric literal.
    UnexpectedDotInNumber,
    // An unexpected exponent was found in a numeric literal.
    UnexpectedExpInNumber,
    // A digit was found in a number that did not allow digits of this base.
    DigitInWrongBase,
    // The end of a number literal was reached unexpectedly.
    UnexpectedEndOfNumber,
    // A number token is followed by an alphanumeric character.
    InvalidCharAfterNumber,
    // A string literal was not closed at the end of the line or file.
    UnterminatedStr,
    // An escape sequence was found that is not valid.
    InvalidEscSeq,

    // Parser error
    Parser = 3000,
    // An expression was expected but not found.
    NotAnExpression,
    // A statement was found where an identifier was expected but not found.
    NotAnIdentifier,
    // A type was expected but not found.
    NotAType,
    // A let statement was found without a type or value.
    LetWithoutTypeOrValue,
    // A grouping was found without a matching closing parenthesis.
    UnmatchedParen,
    // A closing parenthesis was found without a matching opening parenthesis.
    UnexpectedClosingParen,
    // A block keyword was found without a proper opening token.
    NotABlock,
    // A dot was not followed by an identifier or integer.
    UnexpectedTokenAfterDot,
    // A conditional expression was found without a `then` keyword or block.
    ConditionalWithoutThenOrBlock,

    // Global type check error.
    GlobalType = 4000,
    // An attempt was made to declare a namespace in a local scope.
    NamespaceInLocalScope,
    // An attempt was made to declare a namespace in a struct definition.
    NamespaceInStructDef,
    // An attempt was made to declare a struct in a local scope.
    StructInLocalScope,
    // An attempt was made to introduce a new name in a scope where the same
    // name already exists.
    NameAlreadyExists,
    // An attempt was made to shadow a reserved name.
    NameIsReserved,

    // Local type check error.
    LocalType = 5000,
    // An expression was visited as an lvalue, but cannot be an lvalue.
    NotAPossibleLValue,
    // A type mismatch was found in an expression.
    YieldTypeMismatch,
    // A type mismatch was found in a variable declaration.
    LetTypeMismatch,
    // A type mismatch was found in an assignment expression.
    AssignmentTypeMismatch,
    // An name was not found in the symbol table.
    UndeclaredName,
    // An name was matched to a non-FieldEntry node where one was expected.
    NotAVariable,
    // An name was visited as an lvalue, but was not declared with `var`.
    AssignToImmutable,
    // An operator was found that cannot be used with the given expression type.
    OperatorNotValidForExpr,
    // An operator was found that cannot be used with the given types.
    NoOperatorOverload,
    // A yield statement was found outside of a local scope.
    YieldOutsideLocalScope,
    // An attempt was made to declare a function in a local scope.
    FunctionScopeInLocalScope,
    // An access expression on a tuple had an index that was out of bounds.
    TupleIndexOutOfBounds,
    // An access expression was found where the right side was not an integer
    // literal.
    InvalidTupleAccess,

    // Code generation error.
    CodeGen = 6000,
    // The JIT compiler could not find an entry point for the module to run.
    JitMissingEntryPoint,
    // The emitter cannot look up a target machine.
    EmitterCannotLookupTarget,
    // The emitter cannot create a target machine.
    EmitterCannotCreateTargetMachine,
    // The emitter cannot open the output file.
    FileIO,
    // The emitter failed to emit the intended file.
    EmitterCannotEmitFile,

    // Post-processing error.
    PostProcess = 8000,

    // Compiler malfunction error.
    Malfunction = 9000,
    // An unknown error occurred.
    Unknown,
    // Statement was reached that should be unreachable. Typically used when a
    // series of conditional checks do not catch every case.
    Unreachable,
    // Statement was reached that should be impossible. Typically used when a
    // function does not behave as expected.
    Impossible,
    // Statement was reached that should be unimplemented.
    Unimplemented,
    // A bad conversion was attempted.
    BadConversion,
    // Error for testing purposes.
    TestError,
    // The generated LLVM IR failed verification.
    ModuleVerificationFailed,
    // SimpleJit could not create an LLJIT instance.
    JitCannotInstantiate,
    // The JIT compiler found a symbol for `main`, but could not cast it to a
    // function pointer.
    JitBadMainPointer,
};

#endif // ERROR_CODE_H
