# LValues and Assignability

This document explains how lvalues and assignability are handled in the Nico programming language compiler. 
Understanding lvalues is crucial for correctly implementing variable assignments, references, and other related features in the language.

> [!WARNING]
> This document is a work in progress and is subject to change.

## Introduction

Assignment is one of the most basic operations in programming languages.
To assign a value to something is to change the value at a certain location in memory.
If an expression is not associated with a memory location, it cannot be the target of an assignment. 
For example, `5 = 10` is not a valid assignment because the literal `5` does not refer to a memory location that can be changed.

Many programming languages use the term **lvalue** to refer to expressions associated with a memory location and **rvalue** to refer to temporaries, literals, and other expressions that do not have a memory location.

The term **lvalue** can mean "left value", as in the left side of an assignment. 
It can also be interpreted as "locator value", meaning that the expression refers to a location in memory.

In addition to checking whether an expression is an lvalue, we also need to check whether the location to which it refers is mutable. 
Often, programmers will want to prevent data from being changed in unexpected ways. 
If a variable is immutable, it cannot be assigned to after its initial declaration.

For an assignment to be valid, the target must be both an lvalue and mutable. Handling lvalues and checking for mutability can be a complex task, especially in a language with features like references, pointers, and complex data structures.
Here, we aim to clarify how these concepts will be implemented in our compiler.

## LValues

An expression is an **lvalue** if and only if it is a **possible lvalue** AND is visited/used as an lvalue.

An expression visited as an lvalue by the code generator, produces a pointer to a memory location rather than the value itself.

```cpp
a = 5 // 'a' is used as an lvalue.
// The code generator uses the memory address of 'a' to store the value 5.

b = a // 'a' is used as an rvalue here.
// The code generator uses the value stored at the memory address of 'a' to assign to 'b'.
```

A **possible lvalue** is an expression that refers to a location in memory.

The following expressions are considered possible lvalues:
- A name reference expression.
- A member access expression.
- A subscript (index) access expression.
- A reference expression.
- A pointer dereference expression.
- A call expression that returns a reference.

We say an expression is a possible lvalue if it is one of the above, regardless of whether it is actually used as an lvalue.

Only possible lvalues can be visited/used as lvalues.

Note:
- A pointer is not a possible lvalue; it is a value that holds a memory address. 
  - Dereferencing a pointer can yield a possible lvalue if the pointer points to a valid memory location.
  - Therefore, some possible lvalues can contain expressions that are not lvalues.
- A call expression is not a possible lvalue unless it returns a reference.
  - This cannot be determined until the call expression is type-checked.
  - Therefore, the parser cannot catch all lvalue errors; some must be caught during type checking.
- All possible lvalues are expressions, and can therefore be used as rvalues.

**Lvalue errors** occur when an expression that is not a possible lvalue is visited as an lvalue.

Going back to our earlier example, `5 = 10`,
while it is true that `5` is not an lvalue, the root of the problem is that `5` is not a *possible lvalue*.

Earlier, we noted that it is impossible for the parser to catch all lvalue errors since some expressions can only be determined to be possible lvalues after type checking. Therefore, it is best to catch lvalue errors during type checking.

The strategy for catching lvalue errors is as follows:
- First, attempt to rule out the expression as a possible lvalue.
  - Most expressions can be ruled out based on their expression kind alone.
    - For example, a literal expression is never a possible lvalue.
- Once the expression is ruled out as a possible lvalue, check if it is being visited as an lvalue.
  - If it is, report an lvalue error.
- If the expression cannot be ruled out as a possible lvalue, then we must assume it is a possible lvalue.
  - In this case, we don't have to worry about whether it is being visited as an lvalue or not. The code generator should be able to handle either case.
