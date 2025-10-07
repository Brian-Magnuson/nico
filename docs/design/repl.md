# Read-Eval-Print-Loop (REPL)

This document explains a possible design for a Read-Eval-Print-Loop (REPL) for the Nico programming language.
A REPL is a common feature in many programming languages that allows users to interactively enter and evaluate code snippets, providing immediate feedback.

> [!WARNING]
> This is a design document and does not represent a finalized implementation. The actual REPL may differ from the design described here.

## Introduction

There are different ways to "compile and run" code. Here are three common approaches:
- **Ahead-of-Time (AOT) Compilation**: The entire program is compiled into machine code before execution, resulting in object files that can be linked to create an executable. This approach is common in languages like C and C++.
- **Just-in-Time (JIT) Compilation**: The program is compiled into machine code and executed at runtime. This approach is used in languages like Java (after bytecode compilation) and JavaScript.
- **Interpretation**: The program is executed directly from its source code without prior compilation. This approach is common in scripting languages like Python and Ruby.

Languages that are interpreted often provide a REPL. 
A **REPL** is an interactive programming environment that allows users to enter code, evaluate it, and see the results immediately. 
This is particularly useful for learning, experimentation, and debugging.

The Nico programming language is designed to be flexible; its syntax encourages readability and ease of use, a feature desirable in high-level interpreted languages. 
Additionally, the language has features that are already suited for a REPL, such as:
- **Type inference**: Although Nico is statically-typed, it supports type inference, which can simplify code entry in a REPL.
- **Top-level statements**: Nico allows most statements to be used at the top level, making it easier to write and test code snippets interactively.
- **Top-level variables global by default**: Variables declared at the top level are global, which is convenient for a REPL environment where variables need to persist across multiple inputs.

We want to make sure that when users enter code into the REPL, they have a similar experience to writing code in a file.
And based on the features mentioned above, we can design a REPL without changing much of the existing language.

This document outlines how the Nico language and compiler would have to change to support a REPL without interfering with AOT and JIT compilation features.

## REPL Design and Challenges

The basic design of a REPL is reflected in its name:
1. **Read**: The REPL reads a line of input from the user.
2. **Eval**: If the input forms a complete statement, it is compiled and evaluated.
3. **Print**: If the statement is an expression statement, the result is printed. This is in addition to any output the statement may produce.
4. **Loop**: The process repeats, and variables and functions defined in previous inputs are available in subsequent inputs.

Users may be interested in "resetting" the REPL state, which would clear all defined variables and functions.
Some REPLs don't provide an explicit reset command; users can simply restart the REPL process to achieve this effect.
We may be interested in creating a reset command in the future, but it is not a priority.

Implementing a REPL comes with a few challenges:
- Each line is compiled separately
  - Variables and functions defined in previous lines must be available in subsequent lines.
- Pressing the Enter/Return key normally triggers the evaluation step.
  - When newlines are insignificant, the REPL must be able to recognize when a statement is complete and wait for more input if it is not.
    - The REPL must also be able to distinguish statements that are incomplete from statements that contain errors.
  - When entering input that spans multiple lines, evaluation must trigger at some point, but not too early.

Additionally, we also want the REPL to perform almost as well as the AOT and JIT compilers.
Because of this, we try and avoid using a "tree-walking interpreter", i.e., a visitor that directly interprets the AST and does not produce any intermediate representation (IR) or machine code.

## Standardizing the Front End

Although some parts will need to be changed for a REPL, many stay the same.
This gives us the opportunity to create an interface that is consistent for all three compilation modes: AOT, JIT, and REPL.

The AOT compiler uses the following stages:
1. Lexer
2. Parser
3. Type Checker
4. Code Generator
5. Optimizer
6. Emitter

The JIT compiler uses the following stages:
1. Lexer
2. Parser
3. Type Checker
4. Code Generator
5. Optimizer (optional)
6. JIT

And the REPL would use the following stages:
1. Lexer
2. Parser
3. Type Checker
4. Code Generator
5. JIT

Note that steps 1-4 are the same for all three modes.
This gives us the opportunity to create a common interface for the front end of the compiler, which includes the lexer, parser, type checker, and code generator.

## REPL Mode and Statuses

Because certain parts of the compiler behave differently in a REPL, it is best to think of the REPL as a separate compilation mode.
The relevant parts of the compiler will store a boolean flag to indicate whether they are in REPL mode or not.

Our REPL will also need to keep track of its current status. There are at least two possible statuses:
- `OK`: The REPL is ready to read a new line of input. This new line will be read and evaluated separately from any previous lines.
- `Pause`: The REPL was not able to evaluate the previous input because it was incomplete. The REPL will read a new line of input and append it to the previous input before attempting to evaluate it again.
  - Note: Resuming from a pause doesn't actually mean the lexer and parser will continue from where they left off. They will start over from the beginning of the combined input.

As will be explained later, the lexer and parser are the compiler stages capable of pausing the REPL.
Thus, they will need some way to communicate to the rest of the front end that the REPL should pause.
Additionally, when they pause the REPL, they will be unable to provide their respective outputs (tokens and AST) as those outputs are incomplete.

## Changes to the Compiler

We will now discuss more precisely how the lexer, parser, and code generator will need to change to support a REPL.

Not only do we want the REPL to perform as well as the AOT and JIT compilers, we also want the user experience to be as similar as possible to writing code in a file.

### Lexer

- The file path name in a `CodeFile` object can now accept any string, not just a file path.
  - In a REPL, there is no file path name since there is no file.
- In REPL mode, the lexer will no longer automatically add dedent tokens at the end of input.
  - Normally, we automatically add dedent tokens in case user chooses not to add an empty line at the end of a file.
    - Empty lines have a left spacing of 0, which adds the necessary dedent tokens to close all open blocks.
  - In REPL mode, an empty line is needed to exit multi-line input anyway, so this automatic behavior is unnecessary.
- In REPL mode, the following cases will trigger a pause:
  - An unclosed multi-line comment.
  - An unclosed grouping token, such as `(`, `{`, or `[`.
  - An indent token at the end of input.
    - This is not an error outside of REPL mode because we automatically add dedent tokens at the end of input outside of REPL mode.
  - A colon token at the end of input.
    - A colon token indicates the start of an indented block, so we expect an indent token to follow it.

An incomplete token error, such as `UnexpectedEndOfNumber` or `UnterminatedStr` will still be treated as an error and not a pause.
For our REPL, we expect all tokens to be complete.

### Parser

- In REPL mode, it is now an error for an indented block to be empty.
  - Because the lexer no longer requires an increase in left spacing for an indent token, it is possible to write a block that immediately closes on the next line.
  - We consider this an error as it may look confusing to see nothing under a block.
  - The parser will instead catch this error by checking to see if an indent token is immediately followed by a dedent token.
  - If the user wishes to create an empty block, they must use the `pass` statement.
    - Comments cannot be used to create empty blocks; only the lexer can detect comments.
- In REPL mode, the following error will be changed to trigger a pause:
  - `NotAnExpression`: This error occurs when the parser expects an expression but does not find one.
- When using a REPL, all inputs add to the same AST until the REPL is reset.

### Type Checkers

- When using a REPL, the global checker and local checker will reuse the same AST, including the AST nodes and symbol tree, across all lines of input until the REPL is reset.
- When using a REPL, the type checkers will only visit statements that have been added since the last line of input was evaluated.
  - If we check every statement, resetting the symbol tree each time, the REPL would get progressively slower as more lines of input are added.
    - Admittedly, this slowdown would be negligible in most cases, but it is still best to avoid it.
  - If we check every statement without resetting the symbol tree, we would get errors about redefined variables and functions.
  - Ideally, we want to check every statement once.

### Code Generator

- In REPL mode, all global variables and functions will have external linkage.
  - The REPL will function by compiling and executing each line of input as its own module.
  - For variables and functions to be accessible across multiple lines of input, they must have external linkage.
- When using a REPL, the code generator will reuse the same AST, including the AST nodes and symbol tree, across all lines of input until the REPL is reset.
- In REPL mode, instead of creating a `script` function, the code generator will instead create a `repl_N` function for each line of input, where N is a counter that increments with each new line of input.
  - This allows each line of input to be executed separately while still having access to the same global variables and functions.
- In REPL mode, expression statements will be treated as print statements.
  - The expression is visited the same in both cases, but the result is printed in addition to being evaluated.
  - If the expression already prints something, those prints will still occur as normal.
