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

## Error Recovery in the Parsing Phase

The parsing phase is arguably where error recovery is most difficult.
The goal of the parser is to take a stream of tokens and produce an abstract syntax tree (AST) that represents the structure of the code.

If we want to hope to recover from errors in the parsing phase, we need to somehow maintain the validity of our AST's structure, even when we encounter errors.
If we don't do this, our parser may end up in an inconsistent state as it tries to continue parsing after an error, which may lead to confusing error messages about unexpected tokens or invalid syntax.

Error recovery is simpler during type checking since the AST is already built.
Even when an error occurs during type checking, the AST is still structurally valid, making it easy to skip over statements and problematic subtrees of the AST to continue type checking the remaining code.

When the parser encounters an error, we enter *panic mode*, a special mode that interrupts the normal parsing process and allows the parser to skip over tokens until it finds a suitable point to resume parsing.

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

Detecting when statements begin can be somewhat tricky.
Our language does not require statement separators, so we cannot simply look for a semicolon to determine when a statement ends.
Most statements in our language begin with a special keyword, like `let`, `func`, `typedef`, etc., so we can start by looking for these keywords to determine when a new statement begins.
Expression statements are a bit more difficult to detect, as they do not always begin with a special keyword.

```
let b = 10
error_statement
alloc for 5 of i32
[1, 2, 3 + 4]
12 * 34 + 56
some_function_call()
let c = 20
```

In this example, the second statement is an error statement.
After the error statement, we have multiple expression statements.

We could ignore expression statements after the error statement.
Expression statements only appear in execution space, like in a function body, so we can probably get away with skipping a few of them without causing too much confusion for the programmer.

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

Let's look at another example:
```
block {
    good_statement_1
    good_statement_2
    error_statement
}
good_statement_3
```

Here, we'll use the terms *good statement* and *error statement* to refer to statements that are valid and invalid, respectively.
The key idea here is that the error statement is the last statement in the block.
When we encounter the error statement, we *could* keep consuming tokens until we find the starting token of the next statement (`good_statement_3`), but then our parser might be in an inconsistent state if it doesn't realize it has exited the block, which may lead to confusing error messages about unexpected tokens.

So we have to be able to detect when a statement group ends while we are in panic mode, so that we can properly exit the statement group and continue processing from the next statement after the group.

Let's look at one more example:
```
block {
    good_statement_1
    block BAD_TOKEN {
        good_statement_2
    }
    good_statement_3
}
good_statement_4
```

In this example, we have a block that contains another block.
The inner block has an extra token `BAD_TOKEN` that is not expected in the syntax of a block.
We then enter panic mode, looking for either the next statement in the outer block or a closing token for the outer block.
We end up encountering a closing token (`}`) first and we mistake it for the closing token of the outer block.
Then, when we parse good_statement_3, we think we are outside the block, when in reality we are still inside the block, which may lead to confusing error messages about unexpected tokens.

This example illustrates that we cannot simply use the presence of a closing token to determine when we have exited a statement group, as we may encounter unexpected closing tokens while in panic mode.
We need to be able to recognize grouping token pairs, even while in panic mode, so that we can properly identify the token that closes our current statement group.

There are only two types of grouping tokens that are used to enclose statement groups in our language: `{` and `}`, and INDENT and DEDENT tokens.
Due to how our lexer is designed, all of these token types are paired and valid if present at the parsing stage.
Additionally, INDENT and DEDENT tokens never appear between brace tokens, though this may not help our final algorithm much.

Since these tokens are always paired and valid, we don't need to worry about which *type* of token we encounter.
Only the *nesting level* of the token matters.
When we encounter a grouping token while in panic mode, we can adjust our nesting level accordingly, and only consider the statement group to be closed when we encounter a closing token at the same nesting level as when we entered panic mode.

And remember: when we encounter a closing token that closes our current statement group, we do not consume that token.
We resume parsing and let normal parsing logic handle that token, which will allow us to properly exit the statement group and continue processing from the next statement after the group.
