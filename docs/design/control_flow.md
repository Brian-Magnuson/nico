# Control Flow

The following is an explanation of how control flow is managed within the code generator in the Nico programming language.
Understanding control flow is crucial for generating the correct IR for LLVM.

> [!WARNING]
> This document is a work in progress and is subject to change.

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
    %"$retval" = alloca i32
    ; ... code that stores to %"$retval" ...
    store i32 0, ptr %"$retval"
    br label %exit

exit:
    %result = load i32, ptr %"$retval"
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
- It gives every block an exit point, even if there was no explicit return statement.

If the optimizer is used, it may be able to simplify the generated IR by eliminating unnecessary blocks or instructions, while still preserving the original semantics of the function.

## Non-declaring control statements

All of Nico's non-declaring statements, except `pass`, have some effect on control flow. They are:
- `return`
  - Sets the yield value for the function (the return value), then exits the function.
- `break`
  - Exits the enclosing loop expression.
- `continue`
  - Skips the rest of the current iteration of the enclosing loop expression and continues with the next iteration.
- `yield`
  - Sets the yield value for the surrounding block (includes regular Nico blocks, conditional-expressions, loop-expressions, and functions)

From these rules, we can infer a few design requirements for our language:
- Because every block-based expression is an expression, it must be capable of yielding a value, meaning it must allocate space for such a value.
- Yield statements don't actually affect control flow; as long as the code generator can access the allocated space for the yield value, no extra mechanisms are needed to manage control flow with yield statements.
- A function's yield value is special because it can be set within nested blocks (with `return`). There are a few design choices to implement this:
  - Give the function's yield value a special name and have some mechanism to check if the current surrounding block is a function or another block.
    - For example, if the function's yield value is named `$retval` and other yield values are named `$yieldval`, a yield statement in a function must be able to determine which name use when setting the yield value.
  - Keep track of the function's yield value separately
    - This is probably the easier option to implement.
- A loop's merge and continue blocks must be tracked together; else, we cannot use `break` or `continue`.
- `break` and `continue`, much like `return`, must also create unreachable blocks.
- A conditional's then, else, and merge blocks do not need to be tracked together because none of the non-declaring control statements result in a jump to one of these blocks.

## Script and main function

In many languages, executable code contains a special function called `main` where execution starts and ends. For LLVM, one can create a `main` function by defining a function with the name "main" and setting its linkage to external.

Libraries do not have a main function; instead, they provide functions that can be called by the main function or other functions.

We want users to be able to execute Nico code like a script, without needing to wrap it in a function.
We also want to give users the option to write code for libraries.

For this, we use the following design:
- Code at the top level is generated as part of a function named "script"
  - "script" will be an internal-use symbol; it won't have the "::" prefix used by other symbols.
    - We specifically avoid using the name "main" for such a function since external programs would mistake it for the entry point of the program.
  - Variables declared within this script are made global.
- All Nico code is generated in this way, even code for libraries.
  - Ideally, libraries would not have any code in these script functions; we can possibly warn the user to not do this.
- If the code is to be executed, then we will add in a `main` function.
  - "main" is also considered an internal-use symbol.
  - It will simply call the "script" function.
    - Because the structure of `main` is predictable, it won't need the same flexible structure as shown above.

## Recommendations

The previous compiler design used a block stack, which was a `std::vector<llvm::BasicBlock*>`. However, this design is not very robust.
It isn't clear what blocks go into the block stack and what their relationships are.

Since there are different kinds of block-based expressions with different design requirements, a better approach may be to create a new base class that allows different blocks to be chained together in a linked list.

Here is a possible implementation:
```cpp
class Block {
public:
    class Function;
    class Script;

    class Control;
    class Plain;
    class Loop;
    class Conditional;

    std::shared_ptr<Block> prev;
    llvm::Value* yield_val;
};

class Block::Function : public Block {
public:
    llvm::BasicBlock* exit_block;
};

class Block::Script : public Block::Function {
    // Adds no additional members
};

class Block::Control : public Block {
public:
    llvm::BasicBlock* merge_block;
};

class Block::Plain : public Block::Control {
public:
    // Adds no additional members
};

class Block::Loop : public Block::Control {
public:
    llvm::BasicBlock* continue_block;
};

class Block::Conditional : public Block::Control {
public:
    // Adds no additional members
};
```
