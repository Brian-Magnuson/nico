#ifndef NICO_ERROR_CODE_H
#define NICO_ERROR_CODE_H

namespace nico {

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

    // Configuration error
    ConfigError = 1000,

    // Lexer error
    LexerError = 2000,
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
    // An attempt was made a tuple index that is out of range.
    TupleIndexOutOfRange,
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
    // An identifier was found that matches a reserved word.
    WordIsReserved,

    // Parser error
    ParserError = 3000,
    // An expression was expected but not found.
    NotAnExpression,
    // A statement was found where an identifier was expected but not found.
    NotAnIdentifier,
    // A numeric literal could not be parsed due to being out of range for the
    // expected type.
    NumberOutOfRange,
    // A negative sign was found before an unsigned integer literal.
    NegativeOnUnsignedInteger,
    // A type was expected but not found.
    NotAType,
    // An unexpected token was found where specific token was expected.
    UnexpectedToken,
    // A let statement was found without a type or value.
    LetWithoutTypeOrValue,
    // A typeof annotation was found without an opening parenthesis.
    TypeofWithoutOpeningParen,
    // A function statement was found without an opening parenthesis.
    FuncWithoutOpeningParen,
    // A closing parenthesis was found without a matching opening parenthesis.
    UnexpectedClosingParen,
    // A block keyword was found without a proper opening token.
    NotABlock,
    // A dot was not followed by an identifier or integer.
    UnexpectedTokenAfterDot,
    // A conditional expression was found without a `then` keyword or block.
    ConditionalWithoutThenOrBlock,
    // A while-loop expression was found without a `do` keyword or block.
    WhileLoopWithoutDoOrBlock,
    // A do-while loop was found without a `while` keyword.
    DoWhileLoopWithoutWhile,
    // A function declaration was found without an arrow or block.
    FuncWithoutArrowOrBlock,
    // A `var` keyword was found without an address-of operator.
    UnexpectedVarInExpression,
    // A variable annotation contained an unexpected `var` keyword.
    UnexpectedVarInAnnotation,
    // A positional argument was found after a named argument.
    PosArgumentAfterNamedArgument,

    // Parser warning
    ParserWarning = 3500,
    // A loop condition was found with the literal `true` as its condition.
    LoopWithTrueCondition,

    // Global type check error
    GlobalTypeError = 4000,
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
    // An annotation referenced a name that could not be found.
    UnknownAnnotationName,
    // A type-of annotation has no way to type-check the inner expression.
    UncheckableTypeofAnnotation,
    // A function declaration contained duplicate parameter names.
    DuplicateFunctionParameterName,
    // An attempt was made to declare a function that conflicts with an existing
    // overload.
    FunctionOverloadConflict,

    // Local type check error
    LocalTypeError = 5000,
    // An expression was visited as an lvalue, but cannot be an lvalue.
    NotAPossibleLValue,
    // A type mismatch was found in an expression.
    YieldTypeMismatch,
    // A type mismatch was found in a variable declaration.
    LetTypeMismatch,
    // A type mismatch was found in an assignment expression.
    AssignmentTypeMismatch,
    // A type mismatch was found between a function's default argument and its
    // declared parameter type.
    DefaultArgTypeMismatch,
    // A type mismatch was found between a function's body expression and its
    // declared return type.
    FunctionReturnTypeMismatch,
    // An name was not found in the symbol table.
    UndeclaredName,
    // An name was matched to a non-FieldEntry node where one was expected.
    NotAVariable,
    // An attempt was made to call a non-callable type.
    NotACallable,
    // An name was visited as an lvalue, but was not declared with `var`.
    AssignToImmutable,
    // An attempt was made to create a mutable pointer/reference to an
    // immutable value.
    AddressOfImmutable,
    // An operator was found that cannot be used with the given expression type.
    OperatorNotValidForExpr,
    // An operator was found that cannot be used with the given types.
    NoOperatorOverload,
    // A pointer dereference was attempted on a non-pointer type.
    DereferenceNonPointer,
    // An attempt was made to dereference a pointer whose type is `nullptr`.
    DereferenceNullptr,
    // An access expression on a tuple had an index that was out of bounds.
    TupleIndexOutOfBounds,
    // An access expression was found where the right side was not an integer
    // literal.
    InvalidTupleAccess,
    // A control structure was found with a condition that does not have a
    // boolean type.
    ConditionNotBool,
    // A while loop was found that yields a non-unit type.
    WhileLoopYieldingNonUnit,
    // The branches of a conditional expression have mismatched types.
    ConditionalBranchTypeMismatch,
    // A yield statement was found outside of a local scope.
    YieldOutsideLocalScope,
    // A break statement was found outside of a loop.
    BreakOutsideLoop,
    // A continue statement was found outside of a loop.
    ContinueOutsideLoop,
    // A return statement was found outside of a function.
    ReturnOutsideFunction,
    // A pointer dereference was found outside of an unsafe block.
    PtrDerefOutsideUnsafeBlock,
    // No matching function overload was found for a function call.
    NoMatchingFunctionOverload,
    // Multiple matching function overloads were found for a function call.
    MultipleMatchingFunctionOverloads,

    // Local type check warning
    LocalTypeWarning = 5500,
    // A statement was found that is unreachable.
    UnreachableStatement,
    // A regular yield statement was found targeting a loop.
    YieldTargetingLoop,
    // An unsafe block was found that does not contain any unsafe statements.
    UnsafeBlockWithoutUnsafeStmt,

    // Backend error
    BackendError = 7000,
    // SimpleJit could not create an LLJIT instance.
    JitCannotInstantiate,
    // The JIT compiler could not find an entry point for the module to run.
    JitMissingEntryPoint,
    // The JIT compiler found a symbol for `main`, but could not cast it to a
    // function pointer.
    JitBadMainPointer,
    // The emitter cannot look up a target machine.
    EmitterCannotLookupTarget,
    // The emitter cannot create a target machine.
    EmitterCannotCreateTargetMachine,
    // The emitter cannot open the output file.
    FileIO,
    // The emitter failed to emit the intended file.
    EmitterCannotEmitFile,

    // Post-processing error
    PostProcessingError = 8000,

    // Post-processing warning
    PostProcessingWarning = 8500,
    // The symbol tree is in an inconsistent state with the JIT-compiled module.
    // This can be caused by REPL input being partially processed before being
    // discarded.
    SymbolTreeInInconsistentState,

    // Compiler malfunction error
    Malfunction = 9000,
    // An unknown error occurred.
    UnknownError,
    // Error for testing purposes.
    TestError,
};

} // namespace nico

#endif // ERROR_CODE_H
