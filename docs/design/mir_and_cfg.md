# Mid-Level Intermediate Representation (MIR) and Control Flow Graph (CFG)

This document discusses the purpose and basic design of the Mid-Level Intermediate Representation (MIR) and Control Flow Graph (CFG) in the Nico programming language compiler.
MIR and CFG are powerful tools that can be used to solve complex problems related to control flow, including ownership transfer and borrowing.

> [!WARNING]
> This document is a work in progress and is subject to change.

## Introduction

**Control flow** refers to the order in which individual statements, instructions, or function calls are executed or evaluated in a program.
Control flow becomes non-linear when there are constructs such as loops, conditionals, and function calls that can alter the normal sequential execution of code.

Non-linear control flow gives rise to highly-complex problems in programming language design. Here are two such problems:
- In functions that return a value, how do we ensure that all paths return a value?
- How do we track ownership of values through conditional structures to ensure owners are properly invalidated and dropped?

The Abstract Syntax Tree (AST), while useful for representing the syntactic structure of code, is not well-suited for analyzing and transforming non-linear control flow.

To address these challenges, we introduce the Mid-Level Intermediate Representation (MIR) and Control Flow Graph (CFG) in the Nico compiler. The compiler will need to change significantly to support these new representations, but they will prove beneficial in the long run.

## Comparison to LLVM IR

Until now, we have avoided the need for a mid-level IR for one reason:
- It would be redundant to introduce a second IR in a language that already builds LLVM IR.

As we will see, the new MIR is indeed very similar to LLVM IR. However, there are several reasons why we need our own MIR in addition to LLVM IR:
1. **Higher-level abstractions**: MIR can represent higher-level constructs that are closer to the source code, making it easier to perform certain analyses and transformations.
2. **Easier to analyze and transform**: MIR is designed to be more amenable to analysis and transformation than LLVM IR.
3. **Separation of concerns**: By having a separate MIR, we can isolate the complexities of control flow and ownership tracking from the lower-level details of LLVM IR generation.

Some programming languages, such as Rust, also implement their own mid-level IRs for similar reasons. 
In fact, Rust has both a High-Level Intermediate Representation (HIR) and a Mid-Level Intermediate Representation (MIR) before generating LLVM IR.

## Modifications to the Compiler Pipeline

Currently, the Nico compiler pipeline consists of the following stages:
1. **Lexer**: Converts the source code string into tokens
2. **Parser**: Converts tokens into an Abstract Syntax Tree
3. **Type checkers**: Builds a symbol tree and annotates the AST with type information)
4. **Code generator**: Converts the AST into LLVM IR

The introduction of MIR and CFG will modify the pipeline as follows:
1. **Lexer**: Converts the source code string into tokens
2. **Parser**: Converts tokens into an Abstract Syntax Tree
3. **Type checkers**: Builds a symbol tree and annotates the AST with type information)
4. **MIR builder**: Converts the annotated AST into MIR and constructs the CFG
5. **Control flow analyzer**: Analyzes the CFG to ensure proper control flow and ownership tracking
6. **Code generator**: Converts the CFG into LLVM IR

Two new modules are added to the pipeline: the MIR builder and the control flow analyzer.
These will operate off the annotated AST produced by the type checkers.

Additionally, the code generator will need to be significantly modified to generate LLVM IR from the CFG instead of directly from the AST.
Because this change is so significant, the old code generator will be kept intact for now, and a new code generator will be implemented to work with the CFG.
Over time, the old code generator may be deprecated once the new one is fully functional.

## Mid-Level Intermediate Representation (MIR)

The Nico Mid-Level Intermediate Representation (MIR) is a simplified, lower-level representation of the program that captures its control flow and data flow in a way that is easier to analyze and transform than the original Abstract Syntax Tree (AST).
Although we design the MIR ourselves, it will share many similarities with LLVM IR.

The MIR is composed of a set of **functions**. In each function are **instructions**, which are organized into **basic blocks**.

A **basic block** is a sequence of instructions that are executed sequentially. A function may have multiple basic blocks. Control is only transferred between basic blocks at the end of a block through terminator instructions. **Terminator instructions** are special instructions that indicate the end of a basic block and specify where control should go next. Examples of terminator instructions include jumps, conditional branches, and function returns.
A basic block may only have one terminator instruction, which must be the last instruction in the block.
```
func example():
bb1:
    @t1 = add 1, 2
    @t2 = mul @t1, 3
    br bb2
bb2:
    ret @t2
```

A basic block should not be confused with a block expression, which is a syntactic construct that defines a local scope and groups multiple statements together. Basic blocks are groupings of instructions and cannot contain other basic blocks within them.

Unlike expressions and statements, which can be nested within each other in the AST, instructions in MIR are *atomic*, meaning they perform a single operation and cannot contain other instructions within them.
