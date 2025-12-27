# Borrowed Pointers

This document discusses how borrowed pointers are represented and managed in the Nico programming language.
Borrowed pointers are a powerful and highly complex feature that allows for safe and efficient memory management without adding the overhead of reference counting or garbage collection.

> [!WARNING]
> This document is a work in progress and is subject to change.

## Introduction

Currently, the Nico language supports raw pointers, which works almost exactly like C/C++ pointers. However, raw pointers do not provide any safety guarantees. Common issues with raw pointers include:
- Using a pointer after the memory it points to has been freed (dangling pointer).
- Having multiple threads access the same memory without synchronization (data races).
- Losing all references to a memory location (memory leak).
- Parts of the program making invalid assumptions about pointer validity (undefined behavior).

Many modern programming languages introduce mechanisms to mitigate these issues. 
For example, in Java and C#, all class objects are allocated on the heap and are managed by a garbage collector, which automatically reclaims memory that is no longer in use.
C++ introduced smart pointers, such as `std::shared_ptr` and `std::unique_ptr`, which provide automatic memory management through reference counting and ownership semantics.
Rust (famously) uses a system of ownership and borrowing to ensure memory safety at compile time without a garbage collector.

In this document, we will explore how borrowed pointers can be implemented in Nico to provide similar safety guarantees while maintaining performance and flexibility.

## Motivation for Borrowed Pointers

It should be noted that if memory safety was the only goal, then a reference counting system would be sufficient. 
And indeed, Nico is planned to have reference counting smart pointers in the future. 
We can choose not to implement borrowed pointers and the language would remain just as expressive.
So why bother with borrowed pointers at all?

First, understand that the Nico syntax is designed to allow for two kinds of built-in pointers.
The first kind would use the `@` symbol (e.g., `@T` for a pointer to type `T`) and the second kind would use the `&` symbol (e.g., `&T` for a pointer to type `T`).
The first kind would be used for raw pointers while the second kind would be used for a "safer" pointer type.

Now, we ask: if reference counting is such an effective solution, why not have `&T` be a reference counted pointer type?
Reference counting is simpler to understand and implement compared to borrowed pointers.

Reference counting requires two additional constructs:
- A control block to track the number of references to a memory location.
- A weak pointer type to break reference cycles.

The first construct requires additional overhead for every pointer, and the second would require additional syntax and semantics in the language (and the language has little room for more syntax).
It would be easier to implement reference counting as a core library feature rather than a built-in language feature. We can have types like `Sptr` and `Wptr` to represent strong and weak pointers, respectively.
And indeed, this is the plan for Nico.

However, you cannot do the same for borrowed pointers.
Borrowed pointers require language-level support to enforce the borrowing rules at compile time.
This is because borrowed pointers rely on the concept of ownership and lifetimes, which cannot be expressed through library constructs alone.
Therefore, if we want to have borrowed pointers in Nico, we must implement them as a built-in feature of the language.

One of the philosophical goals of Nico is to provide flexibility and control to the programmer while still ensuring safety.
Borrowed pointers are a valuable addition to the language because they allow for fine-grained control over memory management without sacrificing safety.
Although they are strict, giving users this option, in a way, grants them more freedom in how they manage memory.

The Nico language only has room for one more built-in pointer type.
If we decide to use that for something else, we lose the opportunity to have borrowed pointers in the language.

## Basics of Borrowed Pointers

Borrowed pointers in Nico are represented using the `&` symbol.
A borrowed pointer `&T` points to a value of type `T`.
Internally, they are the same as raw pointers, but the compiler enforces additional rules to ensure safety.
```
let x: i32 = 42;
let p: &i32 = &x; // p is a borrowed pointer to x
```

### Ownership and Lifetimes

In Nico, every value has an owner, which is responsible for managing the memory of that value.
An owner can be a variable, a function parameter, or a member of a tuple, struct, or class.
When the owner goes out of scope, the memory is automatically freed.
```
block {
    let x = 37    // x owns the value 37
    printout x    // do stuff with x
}                 // x goes out of scope, memory is freed
```

A value can only have one owner at a time.
By guaranteeing single ownership, we can ensure that memory is always freed exactly once, preventing memory leaks and double frees.

Unlike in Rust, Nico does not have implicit move semantics.
To avoid violating the single ownership rule, the default behavior when assigning or passing values is to create a copy.
```
let a = 5    // a owns the value 5
let b = a    // b is a copy of a, a still owns 5
printout b   // b also owns 5, but a different 5 from a
```

A move is an operation that transfers ownership from one variable to another.
In Nico, moves must be explicit using the `mv` keyword.
```
let a = 5       // a owns the value 5
let b = mv a    // ownership of 5 is moved from a to b, a is now invalid
printout b      // b owns 5
// printout a   // ERROR: a is invalid after the move
```

A move is typically more efficient than a copy. The memory for the value is not duplicated; the ownership is simply transferred.

We say that an owner "lives" from when it is created up to the last point it is used, is moved, or is dropped.
An owner is implicitly dropped when it goes out of scope.
This duration is called the owner's lifetime.
```
block {
    let a = 10      // a's lifetime starts here
    printout a      // a is used here
    block {
        let b = 20  // b's lifetime starts here
        printout b  // b is last used here; b's lifetime ends here
    }
    printout a      // a is last used here; a's lifetime ends here
}
```

### Borrowing

Borrowing is the process of creating a borrowed pointer to a value without taking ownership of that value.
When a value is borrowed, the owner retains ownership, and the borrowed pointer can be used to access the value temporarily.
```
let x = 42          // x owns the value 42
let p: &i32 = &x    // p is a borrowed pointer to x
printout ^p         // dereference p to access the value 42

// let y = mv ^p    // ERROR: cannot move value through borrowed pointer
```

A borrowed pointer does not take ownership of the value it points to.
This means you cannot move or drop the value through a borrowed pointer.

Borrowed pointers also have a lifetime, which is the duration for which the borrowed pointer is valid.
The lifetime of a borrowed pointer must not exceed the lifetime of the owner.
```
let p: &i32
block {
    let x = 100        // x owns the value 100
    p = &x             // ERROR: x does not live long enough
}
printout ^p            // dereference p to access the value
```

In the example above, the borrowed pointer `p` cannot outlive the owner `x`.

A borrowed pointer can be created using the `&` operator.
The value is immutable through the borrowed pointer by default.
To create a mutable borrowed pointer, use `var&`.
```
let var x = 50          // x owns the value 50
let p: var&i32 = var&x  // p is a mutable borrowed pointer to x
^p = 60                 // modify the value through the mutable borrowed pointer
```

### Borrowing Rules

In addition to the ownership and lifetime rules, Nico enforces the following borrowing rules to ensure safety:

When a value is borrowed, the original owner is temporarily inaccessible for the duration of the borrow.
```
let var x = 30         // x owns the value 30
let p: &i32 = &x       // p is a borrowed pointer to x
// x = 40              // ERROR: cannot access x while it is borrowed
printout ^p            // dereference p to access the value 30
```

A value can have either one mutable borrowed pointer or multiple immutable borrowed pointers at any given time, but not both.
```
let var x = 70             // x owns the value 70
let p1: &i32 = &x          // p1 is an immutable borrowed pointer to x
let p2: &i32 = &x          // p2 is another immutable borrowed pointer to x
let p3: var&i32 = var&x    // ERROR: cannot borrow x as mutable while it is immutably borrowed

printout ^p1, ^p2          // both p1 and p2 can be used

let p4: var&i32 = var&x     // OK: p1 and p2 are no longer used
// let p5: &i32 = &x        // ERROR: x already borrowed as mutable
// let p6: var&i32 = var&x  // ERROR: x already borrowed as mutable

printout ^p4               // p4 can be used
```

These additional rules may seem overly restrictive, but they are essential for ensuring memory safety and preventing data races.
Without these rules, it would be possible to create situations where multiple pointers access the same memory location simultaneously, leading to undefined behavior.
Unfortunately, we cannot compromise on these rules if we want to maintain safety guarantees.
