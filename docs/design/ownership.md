# Ownership Transfer and Deletion

This document discusses how ownership can be transferred and how owners are invalidated in the Nico programming language.
The rules of ownership can be tricky to verify at compile time, but they provide strong guarantees about memory safety and resource management.
They also open the door for borrowing, a powerful feature that allows for safe references to owned values without taking ownership.

> [!WARNING]
> This document is a work in progress and is subject to change.

## Introduction

This document does not discuss borrowing. Borrowing is covered in a separate document.

The concept of ownership exists in all programming languages where you can assign values to variables on the stack.
When you assign a **value** to a **variable**, that variable becomes the **owner** of that value.
When an owner goes out of scope, the value it owns (on the stack) is automatically deleted.

Values can typically only have one owner at a time. 
This ensures that memory is always freed exactly once, preventing memory leaks and double frees.

Some languages provide mechanisms for **shared ownership**, where multiple variables can own the same value.
This is typically implemented using reference counting or garbage collection and is used for managing heap-allocated memory.
This will not be discussed in this document.

Ownership becomes more complicated when you allow ownership to be **transferred** or **moved** from one variable to another.

Not all languages support ownership transfer. In C++, you use `std::move` to indicate that ownership is being transferred. In Rust, ownership transfer is the default assignment behavior for non-Copy types. Rust is also special in that it enforces ownership rules at compile time, preventing you from using an owner after it has been moved.

In this document, we will discuss what it means to transfer ownership, why it is complicated, and how we will implement it in Nico.

## Approaches to Ownership Transfer

Ownership transfer, also known as "moving" is often seen as a distinct operation from copying.
When you copy a value, both the original and the copy own their own separate instances of that value.
When you move a value, the original owner relinquishes ownership, and the new owner takes over.

Internally, a move actually *does* copy data to some extent. Specifically, it copies the stack data that represents the value. Pointers to heap data are copied, but the heap data itself is not duplicated. In other words, a move is a shallow copy that transfers ownership.
This makes moves more efficient than deep copies, especially for large data structures.

Because values can only have one owner, something must change about the original owner to have it "lose" ownership.
Here, we identify 3 possible approaches to ownership transfer:

1. **The original owner is invalidated at runtime**: When a move occurs, the original owner is marked as invalid, and any subsequent attempts to use it result in a runtime error. Additionally, the original owner does not free the memory when it goes out of scope, since ownership has been transferred.
   - This approach suggests that ownership transfer is a runtime concept, and that the compiler does not enforce ownership rules.
   - It also suggests that all potential owners have some metadata associated with them to track their validity.
  
2. **The original owner is changed to an "empty" state at runtime**: When a move occurs, the original owner is set to a special "empty" state, indicating that it no longer owns any value. The owner is still valid and can be used, but attempts to use it may lead to unexpected behavior. The memory for the original owner is still implicitly freed, but this is okay since the data it now owns is empty.
   - This is the approach taken by C++ with `std::move`.
   - This approach also suggests that ownership transfer is a runtime concept, and that the compiler does not enforce ownership rules.
   - It does not require additional metadata to track validity, but it also means that the original owner can still be used, which may lead to bugs if the programmer is not careful.

3. **The original owner is invalidated at compile time**: When a move occurs, the compiler marks the original owner as invalid, and any subsequent attempts to use it result in a compile-time error. The original owner does not free the memory when it goes out of scope, since ownership has been transferred.
   - This is the approach taken by Rust.
   - Potential owners do not need additional metadata to track validity, since the compiler enforces ownership rules.
   - This approach provides the strongest guarantees about memory safety, but it also requires more complex compiler logic to track ownership.

For the Nico programming language, we will implement the third approach: invalidation at compile time.
This approach provides the strongest guarantees about memory safety and aligns with Nico's goals of safety and performance.
It is also complex to implement. We will explain why in the next section.

## Challenges of Compile-Time Ownership Transfer

At first glance, compile-time ownership transfer seems straightforward:
- All variables have a corresponding field entry in the symbol tree.
- The symbol tree can track whether a variable is valid or invalid.
- When a move occurs, the compiler marks the original owner as invalid in the symbol tree.
- Any subsequent attempts to use the original owner result in a compile-time error.

```cpp
class Node::FieldEntry : public virtual Node::ILocatable {
public:
    // Whether this field entry is declared in a global scope or not.
    const bool is_global;
    // The field object that this entry represents.
    Field field;
    // If this field is a local variable, the pointer to the LLVM
    // allocation.
    llvm::AllocaInst* llvm_ptr = nullptr;
    // Whether this field entry is currently valid (i.e., owns a value).
    bool is_valid = true;

    /* ... */
};
```

However, this alone leaves us with three problems:

1. **Ownership transfer requires a new owner**: Ownership transfer only makes sense during an assignment where there is a new owner to receive the value.
   - If the user just writes `mv x`, we can invalidate `x`, but there is no new owner to receive the value. So any data that `x` owned cannot be properly freed later.
   - Ownership must also be able to be transferred out of a block using a yield statement, which does not make it clear if there is a new owner.
2. **Ownership transfer can occur in conditional branches**: In a conditional branch, only one branch will eventually execute. So if a variable is invalidated in one branch, that same variable may still be valid in another branch.
   - For example, if, during a conditional branch with at least 2 branches, we invalidate `x` in the first branch, `x` must be valid at the beginning of the second branch, and `x` must be invalid after all branches complete.
     ```
     if (condition) {
         mv x; // Invalidate x
     } else {
         // x must still be valid here
     }
     // x must be invalid here
     ```
   - Additionally, if `x` is only conditionally invalidated, we must ensure it is dropped properly if it remains valid after the conditional.
3. **Ownership transfer can occur in loops**: In a loop, the same block of code may execute multiple times. If a variable is created outside the loop, we cannot allow it to be invalidated inside the loop since it may be invalidated multiple times.
   - For example, if we invalidate `x` inside a loop, it will be invalidated the first time the loop runs, but on subsequent iterations, `x` is already invalid, leading to errors.
     ```
     let x = ...
     while (condition) {
         mv x; // Invalidates x on first iteration, but x is already invalid on subsequent iterations
     }
     ```

We will need to address these challenges in order to implement compile-time ownership transfer in Nico.

## Observation: Deletion as a Separate Operation

When we move a value from one variable to another, we are performing two distinct operations:
1. Transferring ownership of the value to the new owner.
2. Invalidating the original owner to indicate that it no longer owns the value.

There is another operation that is related to invalidation: **dropping**.
Dropping is the process of freeing the memory associated with a variable when it goes out of scope. Dropping is also two distinct operations:
1. Freeing the memory associated with the value.
2. Invalidating the owner to indicate that it no longer owns the value.

Dropping can be done implicitly when a variable goes out of scope, or explicitly using a `del` statement.

The commonality between moving and dropping is that both operations involve invalidating an owner. Henceforth, we will use the term "invalidating" to refer to either operation.

## Observation: Blocks Aren't the Issue

We often think of conditional branches as blocks of code. However, in Nico, blocks are just expressions, and conditional branches can be any expression.

For example:
```
if condition:
  yield 1
else:
  yield 2
```
...is equivalent to:
```
if condition then 1 else 2
```

So the user can do this:
```
if condition then f1(mv x) else f2(mv x)
```

This expression does not use blocks, but it still has the same ownership transfer challenges as before.
So whatever ownership mechanisms we implement must work for all expressions, not just blocks.
