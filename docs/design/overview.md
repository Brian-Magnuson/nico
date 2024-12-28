# Overview

The following is an overview of the Nico programming language design. This document is intended to provide a high-level view of the language and its features.

> [!WARNING]
> This document is a work in progress and is subject to change.

# Data types

## Primitive types

### Numeric types

Currently, only two numeric types are supported: `i32` and `f64`. The `i32` type is a 32-bit signed integer, and the `f64` type is a 64-bit IEEE-754 floating-point number. They may be written as literals in the following ways:
```
42
3.14
```

### Boolean type

The boolean type, `bool`, is a simple type with two possible values: `true` and `false`.

### Character type

The character type, `char`, is a single Unicode code point. It is 32 bits wide and may be written as a literal using single quotes:
```
'a'
'😀'
```

### String type

A string, `str`, is an immutable UTF-8 encoded string. It may be written as a literal using double quotes:
```
"Hello, world!"
```

Unlike the previous types, strings vary in size and cannot be stored directly on the stack. All string literals are stored in static memory and are accessed by reference.

## Aggregate types

### Array type

An array is a fixed-size collection of elements of the same type. The type is written as `[T; N]`, where `T` is the type of the elements and `N` is the number of elements. For example, an array of 3 integers would be written as `[i32; 3]`.

Arrays are zero-indexed and may be written as literals using square brackets:
```
[1, 2, 3]
```

### Tuple type and unit type

A tuple is a fixed-size collection of elements of different types. The type is written as `(T1, T2, ..., Tn)`, where `T1`, `T2`, etc., are the types of the elements. For example, a tuple of an integer and a boolean would be written as `(i32, bool)`.

Tuples may be written as literals using parentheses:
```
(42, true)
```

If a tuple has only one element, it must be written with a trailing comma to distinguish it from a parenthesized expression:
```
(42,)
```

A tuple may also have no elements. It is written as an empty pair of parentheses:
```
()
```

A tuple with no elements is called the unit type. It is used to represent the absence of a value.

## Reference and pointer types

### Reference type

A reference is a safe pointer to a value. It is written as `&T`, where `T` is the type of the value. A reference can never be null and the referenced value must live at least as long as the reference.

A reference must be assigned some location in memory; you cannot use the address of a literal or a temporary value.
```
let x = 42
let r = &x
```

References can be mutable. Such references are written as `var&T`:
```
let var x = 42
let r1 = var&x
let var y = 64
let r2: var&i32 = var&y
```

You cannot have a mutable reference to an immutable value.

You can also have a mutable value containing a mutable reference:
```
let var x = 42
let var y = 64
let var r = var&x
r = var&y
```

You can dereference a reference using the `*` operator:
```
let x = 42
let r = &x
let y = *r
```

However, a reference may be automatically dereferenced under these conditions:
- When a reference is assigned/passed to a variable/argument where the referenced type is expected.
- When accessing a field or method of the referenced type.

If you wish to mutate the referenced value, you must use the `*` operator:
```
let var x = 42
let r = var&x
*r = 64
```

### Pointer type

A pointer is a raw pointer to a value. It is written as `*T`, where `T` is the type of the value. A pointer can be null and does not have any lifetime restrictions.

To create a raw pointer, use the address-of operator in conjunction with an explicit cast:
```
let x = 42
let p = &x as *i32
```

This alone is safe to do. However, dereferencing a pointer is unsafe and must be done within an `unsafe` block:
```
unsafe:
    let y = *p
```

Raw pointers are never implicitly dereferenced. You must always use the `*` operator to access the value.

You can also have a mutable pointer:
```
let var x = 42
let p = var&x as var*i32
unsafe:
    *p = 64
```

# Statements

All programs consist of statements. Statements fall into three categories:
- Declarations: These introduce new names and definitions into the program.
- Expressions: These yield values.
- Non-declaring statements: Statements that can only be used within certain contexts and do not yield values.

## Whitespace

Whitespace is only significant in certain contexts. It is used to separate tokens and check for indentation levels.

An `INDENT` token is inserted when there is a colon, a newline, and an increase in indentation. 

A `DEDENT` token is inserted when there is a newline followed by a decrease in indentation.

`INDENT` and `DEDENT` tokens are not inserted when there is currently an open grouping symbol, such as parentheses or braces.

In all other instances, whitespace is ignored. There is no dedicated token for whitespace other than the `INDENT` and `DEDENT` tokens.

As all statements have a unique start token, whitespace is not needed to separate them. This is perfectly valid:
```
let x = 42 let y = 3.14
let z 
    = x + y
```

However, it is generally recommended to use whitespace to improve readability.

In the event that two statements need to be separated, but must stay on the same line, a semicolon may be used:
```
x; -y
```

## Declarations

### Variables

Variables are used to store data. The `let` keyword declares a local variable:
```
let x = 42
```

Optionally, a data type may be specified:
```
let x: i32 = 42
```

A variable must always be initialized when declared. If no value is provided, a type annotation is required. The variable will be initialized to the default value for the specified type. If this default value is never used, it may be optimized out.

Variables are immutable by default. To declare a mutable variable, use the `var` keyword:
```
let var x = 42
```

## Expressions

An expression is a piece of code that produces a value. Expressions may be simple literals or more complex combinations of literals, operators, and other expressions.

### Artithmetic expressions

The following arithmetic operators are supported:
```
-a
a + b
a - b
a * b
a / b
a % b
```

The forward-slash operator, `/`, will perform integer division if both operands are integers.

### Comparison expressions

The following comparison operators are supported:
```
a == b
a != b
a < b
a <= b
a > b
a >= b
```

Note: Comparisons involving `NaN` will always return `false` in accordance with IEEE-754.

### Logical expressions

The following logical operators are supported:
```
not a
!a
a and b
a or b
```

Note: `not a` and `!a` are equivalent.

Logical `and` and `or` operators are short-circuiting, meaning the right side of the expression will only be evaluated if necessary.

### Bitwise expressions

The following bitwise operators are supported:
```
~a
a & b
a | b
a ^ b
a << b
a >> b
```

These perform the same operations as in C.

### Access expressions

Access expressions are used to access object or tuple properties, including functions. These use the dot operator:
```
object.field
object.method()
tuple.0
```

Methods can also be called using the class/struct name:
```
MyClass.method(object)
```

### Index expressions

Index expressions are used to access elements of arrays. These use square brackets:
```
array[0]
```

### Function call expressions

Function call expressions are used to call functions. These use parentheses:
```
my_function()
my_func_with_args(1, 2, 3)
```

### Block expressions

A block expression is used to group statements together. It may be written in idented form or braced form:
```
block:
    statement1
    statement2

block { statement1 statement2 }
```

Note: Despite structs having a similar syntax, they do not use blocks, and thus, do not yield a value.

The braced form is useful when multiple statements need to be kept on a single line.

Blocks yield the value from the last yield statement executed within the block. If no yield statement is executed, the block yields `()`.
```
let x = block:
    yield 42
```

### If expressions

An if expression is used to conditionally execute code. It uses blocks, and thus, may be written in idented form or braced form:
```
if condition:
    statement1

if condition { statement1 }
```

An if expression may also include an else block:
```
if condition:
    statement1
else:
    statement2
```

If expressions may be chained together:
```
if condition1:
    statement1
elif condition2:
    statement2
else:
    statement3
```

Like block expressions, if expressions yield the value from the last yield statement executed within the block. This is useful for initializing variables based on conditions:
```
let x = if condition:
    yield 42
else:
    yield 0
```

All blocks in an if expression must yield the same type.

### Loop expressions

A loop expression is used to repeatedly execute code. It uses blocks, and thus, may be written in idented form or braced form:
```
loop:
    statement1

loop { statement1 }
```

A loop expression runs indefinitely. To break out of a loop, use the `break` statement.

### While expressions

A while expression is used to conditionally execute code. It uses blocks, and thus, may be written in idented form or braced form:
```
while condition:
    statement1

while condition { statement1 }
```

### Move expressions

A move expression is used to move a value from one location to another. It is written as follows:
```
^value
```

When a value is moved, the original location is invalidated along with all references to it. This is useful for transferring ownership of a value to a function or another variable.

A mutable value may be moved as mutable:
```
var^value
```

### Unsafe blocks and expressions

An unsafe block is like a regular block, but allows the use of unsafe operations. Unsafe operations include dereferencing raw pointers, performing pointer arithmetic, calling unsafe functions, and using `alloc` and `dealloc`.

Unsafe blocks are used to encapsulate unsafe operations and prevent them from leaking into safe code. They are written as follows:
```
unsafe:
    statement1
    statement2
```

Unsafe blocks may still be written within safe blocks. Memory errors may occur, but they can at least be traced back to these unsafe blocks.

## Non-declaring statements

### Return statements

A return statement is used to exit a function and yield a value to the caller:
```
return value
```

Return statements can only be used within functions.

### Break and continue statements

A break statement is used to exit a loop:
```
break
```

A continue statement is used to skip the rest of the current iteration and continue to the next:
```
continue
```

Break and continue statements can only be used within loops.

### Yield statements

A yield statement is used to set the yield value of a block. The yield value is the value that the block will yield when executed. When a block ends, it will evaluate to the last yield value set:
```
yield value
```

Yield statements can only be used within blocks. Yield statements only yield values within the block they are contained in. To propagate a value out of a block, use multiple yield statements:
```
let x = block:
    yield block:
        yield 42
```
