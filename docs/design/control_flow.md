# Control Flow

The following is an explanation of how control flow is managed within the code generator in the Nico programming language.
Understanding control flow is crucial for generating the correct IR for LLVM.

> [!WARNING]
> this document is a work in progress and is subject to change.

## Introduction

Many modern programming languages allow for nested blocks, control structures, and, in some cases, functions, which can result in complex and versatile control flow. For many, the nested structure provides an intuitive way to reason about code execution and scope.
The Nico language will also support this design.

The difficulty of control flow arises how LLVM IR is structured.
LLVM IR has "blocks", but these blocks are not nested in the same way that high-level language constructs are.
Instead, LLVM IR relies on explicit control flow instructions (with labels and jump instructions) to manage the flow of execution between these blocks.

Achieving the high-level control flow semantics of the Nico language will require careful mapping of these constructs to the underlying LLVM IR and tracking which blocks to return to.

## LLVM Blocks

LLVM blocks follow several rules, which must be observed carefully:
- Instructions must be written inside of a block.
- Within a block, instructions are executed sequentially, from top to bottom.
- If a block already has instructions in it, any new instructions will be appended to the end of the block.
- All blocks must have exactly one parent function, the function in which the block is contained.
- Terminator instructions, such as branch or return, are used to jump out of blocks.
- All blocks must have exactly one terminator instruction, and that instruction must be at the end of the block.
- Blocks that are unreachable are not considered invalid.
- The first block created within a function is the function's entry block. When the function is called, execution begins in the entry block.

From these rules, we can also infer the following:
- Blocks may not be empty.
- Aside from the entry block, the order of all other blocks within a function does not matter.

## Functions

Consider the following function:
```
func myfunc() -> i32:
    return 0
```

This function can be represented in LLVM IR as follows:
```llvm
define i32 @myfunc() {
entry:
    %__retval__ = alloca i32
    ; ... code that stores to %__retval__ ...
    store i32 0, ptr %__retval__
    br label %exit

exit:
    %result = load i32, ptr %__retval__
    ret i32 %result

unreachable:
    br label %exit
}
```

Although the generated IR may seem overly complicated given the simplicity of the original function, its structure can be used consistently in many different control flow situations.

1. First, within our entry block, we allocate memory for our return value. We use the function's return type to determine the size and alignment of the allocated memory.
2. We then create an exit block where we load the value from the return value allocation and return it.
3. We continue by inserting new instructions at the end of the entry block.
4. If we encounter a return statement, we store the value to the return value allocation, then immediately branch to the exit block. We also create an unreachable block for cases where statements appear after the return statement.

This design has the following benefits:
- It ensures that there is always one return statement in the function.
- It allows statements to appear after the return statement without invalidating the IR, even if they are unreachable.
- It allows us to associate the return value with a variable-like construct, which is useful for type checking.
- It allows us to potentially set the return value without immediately exiting the function, which is useful for certain control flow patterns.

If the optimizer is used, it may be able to simplify the generated IR by eliminating unnecessary blocks or instructions, while still preserving the original semantics of the function.
