# Control Flow

The following is an explanation of how control flow is managed within the code generator in the Nico programming language.
Understanding control flow is crucial for generating the correct IR for LLVM.

> [!CAUTION]
> This information in this document is planned to be heavily revised with a new control flow graph system.

## Introduction

Many modern programming languages allow for nested blocks, control structures, and, in some cases, functions, which can result in complex and versatile control flow. For many, the nested structure provides an intuitive way to reason about code execution and scope.
The Nico language will also support this design.

The internal complexity of control flow is not to be underestimated, especially with the uniqueness of Nico's block system and how LLVM IR is structured.
The terms "block expression", "local scope", and "LLVM basic block" are highly related but distinct concepts that must be carefully managed separately. 

According to our development guiding principles: "Sometimes, the simplest solution can make things much harder to extend later."
Part of the reason this document exists is because previous attempts have been made to unify symbol resolution with control flow management, which has led to significant complications in the code generator.

In this document, we will review how Nico blocks work, how LLVM blocks work, and the importance of "control flow resolution" as a separate concern from symbol resolution.

## Plain Block Expressions

Much like other modern programming languages, Nico supports a general grouping of statements into "blocks".
```
block {
    let x = 5
    printout x
}
```

Above is a **plain block expression**. The `block` keyword makes it a *plain* block.
For this document, we will only use braced blocks, but understand that indented blocks are functionally equivalent.

An important difference between Nico blocks and blocks in other languages is that Nico blocks are expressions, meaning they yield values.
The value yielded by a block is called the **yield value**. The memory location where the yield value is stored is called the **yield value allocation** or yield allocation for short.
The yield value is an inaccessible variable within the block that can be assigned to using a `yield` statement.
```
block {
    let x = 5
    yield x * 2
}
```

Yield statements are non-declaring statements that assign a value to the block's yield allocation.
Ironically, yield statements themselves do not yield values and cannot be used as expressions.
```
let x = yield 5  // Error
```

When a block has no yield statements, it implicitly yields the unit value `()`.
When a block has more than one yield statement, the type of the yield statements' expressions must all be the same, and the block yields that type.
The value ultimately yielded by the block is the value assigned by the last executed yield statement.
```
let x = block {
    yield 5      // First yield
    yield 10     // Second yield
    yield 15     // Third yield
}
printout x  // Outputs 15
```

## Block-Based Structures

Control structures and functions use a similar block-based syntax and, in fact, may use blocks as part of their structure.

## Conditional Expressions

Conditional expressions, such as `if` expressions, use blocks to group statements for each branch.
```
let x = if cond {
    yield 5
} 
else {
    yield 10
}
```

Although these blocks do not use the `block` keyword, these are still plain block expressions. The reason has to do with the linear nature of conditional expressions. 
This will be explained later in this document.

## Loop Expressions

Loop expressions also use blocks to group statements.
```
let x = loop {
    // Loop body
    if some_condition {
        break 42
    }
    // More loop body
}
```
Loops that use the `loop` keyword are called **non-conditional loops**. These loops loop indefinitely until a `break` statement is encountered.

Blocks used in loop expressions are **loop block expressions**.
Loop blocks differ from plain blocks in that they support `break` and `continue` statements for controlling loop execution.
The `continue` statement behaves similarly to other languages, immediately jumping to the next iteration of the loop.

The `break` statement is like a `yield` statement in that it assigns a value to the loop's yield allocation, but it also immediately exits the loop.
If the user intends to exit the loop without yielding a value, they can use `break ()` to yield the unit value.

Loops can also use the `while` keyword to create **conditional loops**.
```
while cond {
    // Loop body
    if some_condition {
        break ()
    }
    // More loop body
}
```

Conditional loops differ from non-conditional loops in that they evaluate a condition before each iteration of the loop.
If the condition happens to be the literal value `true`, the loop is converted to a non-conditional loop.

Conditional loops *do not allow* yield value types other than `()`. This is because conditional loops may not execute any iterations, and thus would not be able to predictably yield a value.

Note: Because of the rules for loop yield values, the `yield` statement is effectively useless within loop blocks.
- For conditional loops, the yield value is always `()`, so an explicit `yield` statement is redundant.
- For non-conditional loops, the only way to exit the loop is with a `break` statement, meaning the yield value of a loop block is always determined by `break` statements.

## Functions

Functions also use blocks to group statements.
```
func myfunc() -> i32 {
    yield 21
    return 42
}
```

Blocks used in functions are **function block expressions**.
Function blocks differ from plain blocks in that they support `return` statements for exiting the function and setting the function's return value.
The `return` statement is similar to a `yield` statement in that it assigns a value to the function's return value allocation, but it also immediately exits the function.

If the user intends to exit the function without yielding a value, they can use `return ()` to yield the unit value.
If the user does not include any `return` statements, then the yield value is determined by the last executed `yield` statement, or `()` if no `yield` statements were executed.

## LLVM Basic Block Rules

LLVM blocks follow several rules, which must be observed carefully:
- Instructions must be written inside of a block.
- Within a block, instructions are executed sequentially, from top to bottom.
- If a block already has instructions in it, any new instructions will be appended to the end of the block.
- All blocks must have exactly one parent function, the function in which the block is contained.
- Terminator instructions, such as branch or return, are used to jump out of blocks.
- All blocks must have exactly one terminator instruction, and that instruction must be at the end of the block.
- Blocks that are unreachable are not considered invalid.
- The first block created within a function is the function's entry block. When the function is called, execution begins in the entry block.

## Block Code Generation

A block expression does not necessarily map to an LLVM basic block.
LLVM basic blocks are a lower-level construct that represent a sequence of instructions with a single entry point and a single exit point. They are more relevant to control flow management.

When we generate code for a plain block expression, we allocate space for the yield value at the start of the block and then load from that allocation at the end of the block.
Let's use an example:
```
block {
    yield 5
}
```

We can draw a high-level representation of the generated LLVM IR as follows:
```
(allocate $yieldval)
store 5 to $yieldval
(load from $yieldval)
```

The actual generated LLVM IR would be this:
```llvm
%"$yieldval" = alloca i32, align 4
store i32 5, ptr %"$yieldval", align 4
%0 = load i32, ptr %"$yieldval", align 4
```

When we use the code generator to visit a plain block expression, we allocate the yield value allocation at the start of the block and then load from that allocation at the end of the block.

# Issues with Loop Block Generation

It should now be clear why loop blocks and function blocks cannot be treated the same as plain blocks.
Consider what happens when we introduce a `break` statement in a loop block:
```
loop {
    break 10
}
```

If we try to generate code for this loop block in the same way as a plain block, we would end up with something like this:
```
    (allocate $yieldval)
    store 10 to $yieldval
    (jump to loop exit)
unreachable:
    (load from $yieldval)
```

A `break` statement results in a jump instruction (branch or `br` in LLVM IR) that exits the current block.
Any instruction that appears after a jump instruction in the same block is unreachable.
We create an unreachable block for any code that may appear to keep the IR valid.
Because the load instruction appears after the jump instruction, it is unreachable.

At first glance, there are 3 potential solutions to this problem:
1. Access the block's yield allocation from outside the block expression.
2. Use the block's yield value directly.
3. Use a different yield allocation for loop blocks and have `break` statements write to that allocation instead of the block's yield allocation.
   
The first solution is problematic for a few reasons:
- The yield allocation is created as a local variable in the `Expr::Block` visit function. To access it outside the function, it needs to be added to some kind of context object that can be accessed globally.
- That context object would have to be stack-based to support nested blocks and manage multiple yield allocations.
- If the block visit function pops the yield allocation off the stack, then the yield allocation will no longer be accessible when generating the rest of the loop expression.
- If the block visit function does not pop the yield allocation off the stack, then all other expressions will have to manage the stack correctly, which adds too much complexity.

The second solution is also problematic. If we use the block's yield value directly, we end up with something like this:
```
    (allocate $yieldval)
    store 10 to $yieldval
    (jump to loop exit)
unreachable:
    (load from $yieldval into %0)
    (jump to loop_start)
loop_exit:
    (use %0)
```

This solution is clearly invalid because the value `%0` is defined in an unreachable block, meaning it cannot be used in the `loop_exit` block.
LLVM notices this error because it tracks the predecessors of each block and will notice that `%0` is not accessible in all routes to `loop_exit`.
This results in the IR being rejected.

The third solution appears to be the most complicated, but it is the most viable of the three.

## Control Flow Resolution

Control flow is more than just knowing which block to jump to. 
It is also about managing the yield allocations for blocks, loops, and functions.
We introduce the term **control flow resolution** to describe the process of managing control flow in the code generator. It is a process separate from **symbol resolution**, which manages variable scopes and symbol tables.

For symbol resolution, we use a `SymbolTree`. For control flow resolution, we use a `ControlStack`.

It can be tempting to manage both symbol resolution and control flow resolution in the symbol tree, given that:
- The symbol tree is very complicated already.
- The symbol tree may be considered a "single source of truth" for scopes and symbols.
- The symbol tree already handles some LLVM allocation management for variables and fields.
- The local scope chains in the symbol tree can be used as stacks for control flow resolution.
- Creating control stack blocks that mirror symbol tree scopes feels like creating "multiple sources of truth".

However, mixing these two concerns leads to significant complications in the code generator.

Instead of focusing on how these concerns are similar, we should focus on how they are different:
- Scope vs. Control Flow
  - Symbol resolution is concerned with scopes.
  - Control flow resolution is concerned LLVM basic blocks and jumps, which do not always align with scopes.
- Compiler Stages
  - The type checker is concerned with symbol resolution and uses the symbol tree.
    - The type checker doesn't need to know about control flow and LLVM blocks.
  - The code generator is concerned with control flow resolution and uses the control stack.
    - The code generation doesn't need to worry about symbol resolution because symbols are already resolved by the type checker. It technically uses the symbol tree, but only through the AST nodes to access the llvm allocations for variables and fields.

This separation of concerns also makes things easier to extend later. 

At the time of this writing, functions are not fully supported yet, so it is not clear how functions with parameters will interact with symbol resolution and control flow resolution. If we use the symbol tree for control flow resolution, we may find that adding function support in the type checker will break how control flow resolution works in the code generator.

In the end, it is best to accept that symbol resolution and control flow resolution are separate concerns that should be managed separately.
