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
  - The term "possible rvalue" is not useful here since all expressions can be used as rvalues.
- Some rvalues are evaluated based on the memory locations of their sub-expressions.
  - Therefore, some rvalues can contain expressions that are lvalues.
  - Therefore, lvalues are not necessarily on the left side of an assignment.


**Lvalue errors** occur when an expression that is not a possible lvalue is visited as an lvalue.

Going back to our earlier example, `5 = 10`,
while it is true that `5` is not an lvalue, the root of the problem is that `5` is not a *possible lvalue*.

Earlier, we noted that it is impossible for the parser to catch all lvalue errors since some expressions can only be determined to be possible lvalues after type checking. Therefore, it is best to catch lvalue errors during type checking.

The strategy for catching lvalue errors is as follows:
- First, attempt to rule out the expression as a possible lvalue.
  - Most expressions can be ruled out based on their expression kind alone.
    - For example, a binary expression is never a possible lvalue.
- Once the expression is ruled out as a possible lvalue, check if it is being visited as an lvalue.
  - If it is, report an lvalue error.
- If the expression cannot be ruled out as a possible lvalue, then we must assume it is a possible lvalue.
  - In this case, we don't have to worry about whether it is being visited as an lvalue or not. The code generator should be able to handle either case.

## Mutability: Design Considerations

In C++, a struct object can be declared as `const`. This generally means that the object is completely immutable and only const member functions can be called on it.

```cpp
struct Point {
    int x, y;
    void move(int dx, int dy) { x += dx; y += dy; }
    void print() const { std::cout << "(" << x << ", " << y << ")\n"; }
};

const Point p{1, 2};
p.move(3, 4); // Error: cannot call non-const member function on const object
p.x = 10;   // Error: cannot assign to member of const object
p.print();    // OK: can call const member function on const object
```

This behavior can be desirable. It prevents accidental modification of data that is intended to be read-only.
However, structs can become large and complex. Having all members of a struct be immutable can be overly restrictive.

For example, suppose we want to add another function to our `Point` struct that calculates the distance from the origin.

```cpp
double distance() {
    return std::sqrt(x * x + y * y);
}
```

Next, to improve the efficiency of our program, we decide to cache the result of the distance calculation. We add a member variable to store the cached distance and modify the `distance` function to update this cache when it is called.

```cpp
struct Point {
    int x, y;
    double cached_distance = -1;

    double distance() {
        if (cached_distance == -1) {
            cached_distance = std::sqrt(x * x + y * y);
        }
        return cached_distance;
    }
};
```

The problem is that if we declare a `Point` object as `const`, we can no longer call the `distance` function because it modifies the `cached_distance` member variable.

We can try and achieve more fine-grained control over mutability by declaring individual member variables as `const`...

```cpp
struct Point {
    const int x, y; // x and y are immutable
    ...
};
```

But now we can no longer move the point, even if the `Point` object is not `const`.

To solve this problem, we must ask ourselves: What does it mean for a struct object to be `const`?

One interpretation is that the members that compose the object's "state" are immutable. 

This interpretation implies that not all members of a struct are part of its state. Some members, like our `cached_distance`, are implementation details that do not affect the logical state of the object.

When these implementation details need to be modified, it should be allowed even if the object is `const`. C++ provides a way to do this using the `mutable` keyword.

```cpp
struct Point {
    int x, y;
    mutable double cached_distance = -1; // cached_distance can be modified even if Point is const
    ...
};
```

This approach allows us to have `const` struct objects while still being able to modify certain members that are not part of the object's logical state.

## Mutability: Problems with Reusing LValue Visitation

Listed in our project's code style guiding principles is this:
- Keep it simple, but leave things open for future extension. Sometimes, the simplest solution can make things much harder to extend later.

Perhaps the aforementioned feature of allowing internal mutable members in immutable objects is not a feature we need right now. 
However, we want our language to be extensible and potentially support this feature in the future.

We want to handle this carefully. Taking the easy approach can lead to complications later on. 
This is especially true with mutability since it has some overlap with lvalues. 
It can be easy to reuse the lvalue-visiting mechanism to check for mutability, but this strategy can make it difficult to implement our desired rules for mutability later on.

When an expression is visited as an lvalue, we take this to mean that the expression is, or is part of, the target of an assignment. And if something is the target of an assignment, we take this to mean that it is being modified.

```
let var a = 5
a = 10 // 'a' is being modified here
```

Now, suppose we try to do the same assignment, but with an immutable variable.
```
let b = 5
b = 10 // Error: 'b' is immutable and cannot be modified
```

It can be tempting to use the lvalue-visiting mechanism to enforce immutability.
And for most simple cases, this approach works fine. But this is more of a shortcut than a proper solution.

Recall our definition of an lvalue:
- An expression is an **lvalue** if and only if it is a **possible lvalue** AND is visited/used as an lvalue.

In our example, `b` is a possible lvalue and it is being visited as an lvalue, so, under our definition, `b` is an lvalue.

The problem is that `b` is not assignable because it was declared as immutable.
Assignability is a property that is *separate* from being an lvalue.

By reusing the lvalue-visiting mechanism, we are also reusing our lvalue-error-catching mechanism. 
Recall our strategy for catching lvalue errors:
- First, attempt to rule out the expression as a possible lvalue.
- Once the expression is ruled out as a possible lvalue, check if it is being visited as an lvalue.
- If it is, report an lvalue error.

> [!WARNING]
> This design is still a work in progress and is subject to change.

Now, suppose we want to have an immutable struct, `s`, with an internal mutable member, `m`.

```
struct S:
  prop mut m: i32 = 0

let s = new S { m: 5 }
s.m = 10
```

Here, s is immutable, but we allow internal mutable members, like `m` to be modified. So we allow this assignment.

If we are reusing our lvalue-visiting mechanism to check for mutability, we will run into problems. 
- In the example, we visit `s.m` as an lvalue.
- In this visit, we visit `s` as an lvalue.
  - This makes sense because the address of `m` is based on the address of `s`, so we need the address of `s`.
- `s` is a possible lvalue and it is being visited as an lvalue, so `s` is an lvalue.
- If we rule out `s` as a possible lvalue, we will report an lvalue error because `s` is immutable and cannot be modified.

Ruling out an expression as a possible lvalue is an irreversible action since we report the error right away.

What we need is a way to defer an expression's assignability check until we have all the information we need to determine whether it is assignable or not.

## Mutability: Design Goals

Let us define more clearly what internal mutability means in our language.

**Internal mutability** is the concept of allowing certain members of an immutable object to be modified.

There are two ways to achieve internal mutability:
- Indirect internal mutability
- Direct internal mutability

**Indirect internal mutability** is when an immutable object contains a member that is a reference or pointer to a mutable object.
It allows us to bypass immutability by modifying an object outside of the immutable object.

This is actually possible in C++:
```cpp
int global = 0;

struct Object {
    int * const can_change = &global;  
};

int main() {
    const Object cant_change = Object();
    *cant_change.can_change = 1;
    std::cout << global << std::endl; // Outputs: 1
    return 0;
}
```

In our language, we achieve this by using the `var*T` type, which is a pointer to a mutable `T`.
```
struct Object:
  prop can_change: var*i32
```

**Direct internal mutability** is when an immutable object contains a member that is explicitly marked as mutable, bypassing the immutability of the containing object.

This is similar to the `mutable` keyword in C++.
```cpp
struct Object {
    mutable int can_change = 0;
};

int main() {
    const Object cant_change = Object();
    cant_change.can_change = 1;
    std::cout << cant_change.can_change << std::endl;
    return 0;
}
```

In our language, we achieve this by using `mut` in place of `var` for member declarations.
```
struct Object:
  prop mut can_change: i32 = 0
```
