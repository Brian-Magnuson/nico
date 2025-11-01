# Function Overloading

This document explains how function overloading will be implemented in the Nico programming language.
Function overloading is a common feature in many programming languages that enables flexibility and code reuse by allowing multiple functions to share the same name.

> [!WARNING]
> This document is a work in progress and may be subject to change.

## Introduction

Some modern programming languages like C++, C#, Kotlin, and Swift support function overloading. 
This feature allows developers to define multiple functions with the same name but different parameter lists (different types or number of parameters). 
The compiler determines which function to call based on the arguments provided during the function call.
It is a kind of compile-time polymorphism.

Function overloading might be simple to implement if the language only supports positional arguments and not named or default arguments. 
Such an implementation would mean there is only one way to call a function, and the compiler can easily match the function signature with the provided arguments.
However, we want Nico to support named and default arguments, which allows some arguments to be specified out of order or even omitted.
In fact, the number of ways to call a function increases *exponentially* with the number of default arguments.
Thus, a complex, analytic approach is required to implement function overloading in Nico.

It can be difficult to implement functions with overloading all at once.
However, our development guiding principles state that we must "leave things open for future extension".
Thus, a major focus of this document is to outline a design that is fairly simple to implement and allows for function overloading to be added later.

## Overload Resolution Problem

Overload resolution is relevant in two contexts:
1. When a function is **defined**. The compiler must check if the new function definition conflicts with any existing definitions.
2. When a function is **called**. The compiler must determine which function definition to invoke based on the provided arguments.

The second context is easy to implement. If we have a collection of overloads, we can *attempt* to match the provided arguments against each overload.
We'll say a match is found when:
- After applying default arguments, positional arguements, and named arguments (in that order), all parameters of the function are satisfied.

If zero or more than one match is found, an error is raised.

It is the first context that is more difficult to implement.
This is because it is not easy to define when an overload conflicts with an existing definition.
For example, consider the following two function definitions:
```
func f(a: i32, b: f64 = 0) { ... }
func f(b: i32, a: f64 = 0) { ... }
func f(c: i32) { ... }
```

While it may seem obvious that these definitions conflict, it is not immediately clear *why* they conflict.
These functions use different parameter names, different numbers of parameters, and different default arguments.
While it is true that they can all be called with a single `i32` argument, the ambiguity *could* be resolved if the caller uses a named argument.
For example, the call `f(a: 5)` would unambiguously refer to the first function definition. 

Theoretically, we could *ignore* the first context altogether and only perform overload resolution during function calls.
However, this would lead to poor developer experience, as errors would only be caught at call sites.
Additionally, a user could write this:
```
func f(a: i32) { ... }
func f(a: i32) { ... }
```

These definitions have identical signatures, and thus are clearly conflicting. But if we ignore the first context, this error would only be caught when `f` is called.

The important question is: Where do we draw the line on what constitutes a conflict? Striking the right balance of strictness and permissiveness is crucial for a good developer experience.

## Overload Conflict Resolution Design

For one possible solution to this problem, we accept the following:
1. We *cannot* prevent the possibility of ambiguous calls when checking for conflicts during function definitions.
2. Because we support named arguments, we leave it to the user to be as *specific* as possible when calling overloaded functions.

Under this design, we *allow* these function definitions to coexist without conflict:
```
func f(a: i32, b: f64 = 0.0) { ... }
func f(b: i32, a: f64 = 0.0) { ... }
func f(c: i32) { ... }
```

While it is true that a call like `f(5)` would be ambiguous, we only require the user to be *more specific* when calling the function. For example:
- `f(a: 5)` would unambiguously refer to the first function.
- `f(b: 5)` would unambiguously refer to the second function.
- `f(c: 5)` would unambiguously refer to the third function.

This design does not cover this case:
```
func f(a: i32) { ... }
func f(a: i32, b: f64 = 0.0) { ... }
```

Suppose a user wants to call the first overload. The most specific call they can make to refer to the first overload is `f(a: 5)`. However, this call is also valid for the second overload, so it is ambiguous.

Our decision is to *disallow* such definitions from coexisting. This decision is based on the following:
- Throughout the language, we allow users to write code with varying levels of specificity.
  - E.g., when declaring a variable, the user can choose to provide an explicit type annotation or let the compiler infer the type
  - E.g., when referencing a name, the user can choose to write the full symbol or write a shorter version and let the compiler resolve it.
- As long as the user is specific enough in what they want, they can avoid ambiguity.
- When specificity increases, ambiguity should *never* increase.
- The above case *violates* this principle, because the user cannot be specific enough to avoid ambiguity.

This introduces the idea that every function definition has a possible call that is *most specific* to that definition. And in such a call, there are no positional arguments. We can conclude that:
- The order of parameters in a function definition *does not matter* when checking for conflicts.
- For two function definitions, ambiguity is only possible if the set of parameter names in one definition is a *subset* of the set of parameter names in the other definition (this is not sufficient to guarantee ambiguity, but it is necessary).

We now present our formal rules for overload conflict resolution:
1. Let $f_i$ be one of multiple overloads of a function $f$.
2. A **parameter string** is the concatenation of a parameter's name and type, excluding any default value.
   - E.g., for `a: i32 = 5`, the parameter string is `"a: i32"`.
3. Let $M(f_i)$ be the set of parameter strings for *all* parameters of $f_i$.
4. Let $D(f_i)$ be the subset of $M(f_i)$ consisting of parameter strings for parameters that have default values.
5. Consider two overloads $f_1$ and $f_2$ of $f$.
6. If $M(f_1) = M(f_2)$, then $f_1$ and $f_2$ conflict.
7. If $M(f_1) \supset M(f_2)$ and $M(f_1) - M(f_2) \subseteq D(f_1)$, then $f_1$ and $f_2$ conflict.
   - Intuitively, this occurs when the only difference between $f_1$ and $f_2$ is that $f_1$ declares additional optional parameters.
8. If $M(f_2) \supset M(f_1)$ and $M(f_2) - M(f_1) \subseteq D(f_2)$, then $f_1$ and $f_2$ conflict.
9. Otherwise, $f_1$ and $f_2$ do not conflict.

We only check for conflicts in pairs of overloads. When a new overload is defined, we check it against all existing overloads using the above rules.

When reporting errors, we can briefly describe these rules like this:
> Two function overloads conflict if they have the same set of parameters, or if one set of parameters is a superset of the other, differing only by optional parameters.

### Examples

Let us use the above rules to analyze some examples.

**Example 1**
```
func f(a: i32) { ... }
func f(a: i32) { ... }
```

- Here, $M(f_1) = M(f_2) = \{"a: i32"\}$.
- By rule 6, these definitions are in conflict.

**Example 2**
```
func f(a: i32) { ... }
func f(a: i32, b: i32 = 0) { ... }
```

- $M(f_1) = \{"a: i32"\}$
- $M(f_2) = \{"a: i32", "b: i32"\}$
- $D(f_2) = \{"b: i32"\}$
- We have $M(f_2) \supset M(f_1)$ and $M(f_2) - M(f_1) = \{"b: i32"\} \subseteq D(f_2)$.
- By rule 8, these definitions are in conflict.

**Example 3**
```
func f(a: i32, b: i32 = 0) { ... }
func f(b: i32, a: i32 = 0) { ... }
```
- $M(f_1) = M(f_2) = \{"a: i32", "b: i32"\}$.
- By rule 6, these definitions are in conflict.

**Example 4**
```
func f(a: i32) { ... }
func f(a: i32, b: i32) { ... }
```
- $M(f_1) = \{"a: i32"\}$
- $M(f_2) = \{"a: i32", "b: i32"\}$
- $D(f_2) = \{\}$
- Here, $M(f_2) \supset M(f_1)$, but $M(f_2) - M(f_1) = \{"b: i32"\} \not\subseteq D(f_2)$
- None of the other rules apply, so these definitions are *not* in conflict.

**Example 5**
```
func f(a: i32) { ... }
func f(b: i32) { ... }
```
- $M(f_1) = \{"a: i32"\}$
- $M(f_2) = \{"b: i32"\}$
- None of the rules apply, so these definitions are *not* in conflict.

## Function Name Reference and Call Resolution

This section addresses two problems:
1. When multiple function overloads are defined, where do we store them such that they can be retrieved later?
2. If all we have is a name reference expression (e.g., just `f`), how do we resolve its type?

We try to solve these problems by introducing two new nodes:
- A symbol node called `OverloadGroup`, which represents a collection of function overloads.
- A type node called `OverloadedFn`, which represents a function type that could refer to multiple overloads.

An `OverloadGroup` is a special kind of field entry in a symbol table. Normally, when we look up a name in a symbol table, we get back a single field entry. If we look up the name of an overloaded function, we get back an `OverloadGroup` entry instead. This entry contains a list of all overloads for that function name.

The type object for an `OverloadGroup` is always an instance of `OverloadedFn`.
An `OverloadedFn` is a special type that indicates the expression is callable, but does not specify *how* it is callable. It contains a pointer back to the overload group it represents. We call it "dynamic" because the exact function type is resolved later when the expression is called with arguments. This still happens at compile-time.

The reason we have this separate type is to allow cases like this:
```
func greet() { printout "Hello!" }
func greet(name: String) { printout "Hello, ", name, "!" }

let greeter = greet
```

In other programming languages, this assignment would be ambiguous and the compiler would raise an error. 
This is where Nico differs from other languages. 
In Nico, we allow this assignment to succeed by giving `greeter` a `OverloadedFn` type. 
This allows `greeter` to encompass all overloads of `greet`.
```
greeter()           // Calls greet()
greeter(name="Bob") // Calls greet(name: String)
```

The `OverloadedFn` type has two very important rules:
- The user is *not allowed* to write this type explicitly; it can only be inferred by the compiler.
- When compared to any other type, including other `OverloadedFn` types, it is always considered *not equal*.

The `OverloadedFn` type can be considered one of the few times we *defer* type checking for later. The reason we allow this is because there are only a few ways to use a function pointer, especially since function pointers are immutable.
Because of this, there are only a few situations where we need to resolve the exact function type.

Let's assume that the only time we need to resolve the exact function type is when the function pointer is called.
When type checking the call expression, we combine the information from the `OverloadedFn` type and the provided arguments to perform overload resolution, resulting in an exact function type.
We then reassign function type and field entry of the callee expression, allowing the IR to be generated as normal.

Going back to our earlier example:
```
func greet() { printout "Hello!" }
func greet(name: String) { printout "Hello, ", name, "!" }

let greeter = greet

greeter() 
```

The AST of this last statement is this:
```
(expr (call (nameref greeter)))
```

The type checker will follow these steps:
1. Visit the `(expr ...)` node:
   1. Visit the `(call ...)` node:
       1. Visit the `(nameref greeter)` node:
          1. Look up `greeter` in the symbol table, getting back an overload group with type `OverloadedFn` pointing to the overload group for `greet`.
          2. Assign the type of the `(nameref greeter)` node to be this `OverloadedFn` type.
          3. Return.
       2. See that the callee has type `OverloadedFn`.
       3. Perform overload resolution using the overload group and the provided arguments (none in this case).
       4. Find a match with the `greet()` overload.
       5. Assign the field entry of `(nameref greeter)` to be the field entry for `greet()`.
       6. Reassign the type of `(nameref greeter)` to be the exact function type `func() -> ()`.
       7. Assign the type of the `(call ...)` node to be `()`, the return type of `greet()`.
       8. Return.
   2. Return (the `(expr ...)` node is an expression statement and has no type).
2. Done.

## Non-Overloaded Functions

In this last section, we address how non-overloaded functions are represented in this design.
This is important because our system for overload resolution interferes with how we would "normally" look up function names in the symbol table.

There are two possible approaches:

**Approach 1: Use Overload Groups for Non-Overloaded Functions**

An overload group is a collection of overloads for a function name.
When a function is *not overloaded*, we can think of the sole definition as a single overload in an overload group.
Thus, we can represent all functions, whether overloaded or not, as overload groups.

When looking up a function name in the symbol table, if the function exists, we will always find an overload group.
The search function will check to see if the overload group contains only one overload.
If so, it will return the field entry for that single overload.
Otherwise, it will return the overload group itself.

When a new overload is defined, we will check if an overload group already exists for that name.
If so, we will add the new overload to the existing group.

**Approach 2: Do Not Use Overload Groups for Non-Overloaded Functions**

All function definitions, whether part of an overload group or not, will have their own field entry in the symbol table.
If a function is not overloaded, then there is no reason to create an overload group for it.

When looking up a function name in the symbol table, if the function exists and is not overloaded, we will find its field entry directly.

When a new overload is defined, we will check if a function with that name already exists in the symbol table.
If so, we create a new overload group, add the existing function's field entry to it, and then add the new overload's field entry to it.

---

Our decision is to use **Approach 1**.

The drawbacks of this approach are:
- Slightly more memory usage, as every function now has an overload group, even if it only contains one overload.
- Slightly more complexity in the symbol table lookup logic, as we always have to check if the overload group contains only one overload.

The benefits of this approach are:
- Simpler implementation, as we do not need to handle two different cases when looking up function names.
- Consistent representation of functions, as all functions are treated uniformly as overload groups.
- Easier future extension, as we can easily add overloads to any function without needing to change its representation in the symbol table.

It is worth noting that this approach can work with possible *non-overloadable* functions in the future. 
An example of this is closures. These can exist as a single field entry outside of an overload group.
The presence of an overload group may even be used to indicate whether a function is overloadable or not.
