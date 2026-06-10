# Error Recovery

In a compiler, error handling is crucial for providing meaningful feedback to developers and allowing the compilation process to continue even when errors are encountered. Error recovery is perhaps most difficult in the parsing phase, where the parser must be able to determine what tokens must be skipped to allow the parser to continue processing the remaining input. 

This document explains the concept of error recovery, giving special attention to the parsing phase, and describes the error recovery strategy used in the compiler.

> [!WARNING]
> This document is a work in progress and is subject to change

## Introduction

Suppose a programmer is writing code in a large codebase and makes a syntax error.
Perhaps they forget to close a parenthesis or misspell a keyword. 
The compiler identifies the error and reports it to the programmer.

However, if the compiler simply stops at the first error, it may not provide enough information for the programmer to fix all the issues in their code.
This is especially true when working with large codebases, where multiple errors may be present and re-compilation can be time-consuming.
We want our compiler to be able to recover from errors and continue processing the remaining code, revealing more errors if they are present.

This is where error recovery comes in.
**Error recovery** enables the compiler to skip over erroneous code and continue parsing the rest of the input, allowing it to report multiple errors in a single compilation run.

But there is a balance to strike here.
If the compiler skips too much code, it may miss important errors or produce misleading error messages.
On the other hand, if it skips too little code, it may observe many cascading errors that are a consequence of the initial error, making it harder for the programmer to identify the root cause.

Unfortunately, effective error recovery is an easy problem to ignore.
There's a few reasons for this:
- Cascading errors are difficult to predict. Even if we report more errors after the first, it may be difficult to test for these additional errors.
- We only need to report the first error to the programmer and stop compilation for our compiler to be considered properly functioning. There is not much incentive to implement error recovery, especially if it is a non-trivial amount of work.
- Implementing error recovery can be complex and may require significant changes to the compiler's architecture, especially if the compiler was not designed with error recovery in mind.

For our project, we want to try and improve our error recovery capabilities while keeping the implementation as simple as possible.
We want to prefer reporting more errors to the programmer, though we want any cascading errors to make sense and be helpful for the programmer in identifying the root cause of the initial error.
We especially want to focus on improving error recovery in the parsing phase, as this is where most syntax errors are likely to occur and where our current error recovery capabilities are weakest.

## Error Recovery in Various Phases

In our language, most errors are reported during the first four stages of the compiler:
- Lexing
- Parsing
- Declaration-space type checking
- Execution-space type checking

After these stages, compilation proceeds to the code generation phase, when the user's code is generally assumed to be correct, and any errors that occur are likely to be internal compiler errors rather than user errors.

Currently, our error handling strategy is designed such that, when an error is encountered in any stage, compilation shall not proceed to the next stage.
For example, if a lexer error occurs, the parser will not be invoked.
More lexer errors may be reported, but the parser will not be invoked until all lexer errors have been resolved.

There are pros and cons to this approach:
- Pros:
    - It is simple to implement and understand.
    - It prevents cascading errors that may be a consequence of the initial error, making it easier for the programmer to identify the root cause.
- Cons:
    - It may not provide enough information for the programmer to fix all the issues in their code, especially in large codebases where multiple errors may be present.
    - It may require multiple compilation runs to identify and fix all errors, which can be time-consuming.
    - Once the user fixes all their issues in one stage, they may encounter new errors in the next stage, which can be frustrating.

### Importance of Statements for Error Recovery

When allowing the compiler to continue after an error is encountered, *statements* provide a natural boundary for error recovery.
Code is generally organized into lists of statements.
In fact, our AST is currently described as an `std::vector<std::shared_ptr<Stmt>>`, which reflects this organization.
Blocks in our language are also defined as lists of statements.

When a statement is problematic, we can try and move on to the next statement and continue processing from there.
Consider this example:
```
let a = 5
let b = 10
let c
let d = 20
```

The third statement here is problematic; it is an immutable variable declaration without an initializer, which is not allowed in our language.
However, all the surrounding statements are perfectly valid.
We should be free to continue processing from the fourth statement, which may reveal more errors if they are present.

### Sensible Cascading Errors

Revisiting the example in the previous section, suppose we add a statement after the problematic statement:
```
let c
let d = 20
let e = c + d
```

The statement `let e = c + d` is also problematic, as `c` is not properly declared.
This error is a consequence of the initial error in the statement `let c`.
Is it okay to report this error as well?

In this case, it is reasonable to report the error in `let e = c + d` as well, as it provides additional information to the programmer about the consequences of the initial error.
The programmer can see this error and trace it back to the initial error in `let c`, which may help them identify and fix the root cause more quickly.

The more errors we can report to the programmer, the more information they have to work with when trying to fix their code.
And as we can see here, cascading errors *can* be helpful for the programmer in identifying the root cause of the initial error, as long as they are sensible and not overwhelming.

### Importance of Statement Groups

Our language features different constructs that allow statements to be grouped together, including blocks, functions, and classes.
In the context of error recovery, we may call these constructs *statement groups*.

When we parse statement groups, we need to be careful about how we handle errors within these groups.

Let's look at an example:
```
block {
    let a = 5
    let b
    let c = a + b
}
let d = 20
```

This example is similar to the previous one, but now we have a block that contains the statements.
The statement `let b` is problematic, as it is an immutable variable declaration without an initializer.

We need to make sure that we handle this error properly in the context of the block.
If our parser exists the block when it encounters the error in `let b`, it may find an additional `}` token that it does not expect, which may lead to a confusing error message about an unexpected token.

When we encounter the error, we need to recognize that we are within a statement group, and that the next statement may be part of the same group.
In this case, the next statement, `let c = a + b`, is indeed part of the same block.

It's true that the block expression as a whole will be considered problematic, as it contains the error in `let b`, but we need to recognize the boundaries of the block for proper error recovery.
We need to recognize that the declaration for `c` is part of the block and the declaration for `d` is not, so that we can report the error in `let c = a + b` as a consequence of the error in `let b`, while still allowing the parser to continue processing from `let d = 20`.
