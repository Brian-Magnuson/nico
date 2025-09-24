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

```cpp
class FrontEnd {
    Lexer lexer;
    Parser parser;
    GlobalChecker global_checker;
    LocalChecker local_checker;
    CodeGenerator codegen;
};
