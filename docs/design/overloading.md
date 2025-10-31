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
2. A parameter's *parameter string* is a string that includes both the name of the parameter and its type, but not its default value. 
   - E.g., for a parameter `a: i32 = 5`, the parameter string is `"a: i32"`.
3. Let $M(f_i)$ be the set of parameter strings for *all* parameters in function $f_i$.
4. Let $D(f_i)$ be the set of parameter strings for *all* parameters in function $f_i$ that have default values.
5. WLOG, let $f_1$ and $f_2$ be two overloads of function $f$.
6. If $M(f_1) = M(f_2)$, then $f_1$ and $f_2$ are in conflict, and we are done.
7. If $M(f_1) \supset M(f_2)$ and $M(f_1) - M(f_2) \subseteq D(f_1)$, then $f_1$ and $f_2$ are in conflict, and we are done.
   - That is, if the only difference between the two functions are optional parameters, then they are in conflict.
8. If $M(f_2) \supset M(f_1)$ and $M(f_2) - M(f_1) \subseteq D(f_2)$, then $f_1$ and $f_2$ are in conflict, and we are done.
9. Otherwise, $f_1$ and $f_2$ are not in conflict.

We only check for conflicts in pairs of overloads. When a new overload is defined, we check it against all existing overloads using the above rules.

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
