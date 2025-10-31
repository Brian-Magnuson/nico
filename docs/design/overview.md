# Overview

The following is an overview of the Nico programming language design. This document is intended to provide a high-level view of the language and its features.

> [!WARNING]
> This document is a work in progress and is subject to change.

# Data types

## Primitive types

### Numeric types

> [!WARNING]
> This part is subject to change.

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

### Struct literal type

A struct literal type is a custom data type representing a collection of named properties. They are the literal counterpart to named struct types, which are defined using struct declarations.

Struct literal types offer a flexible way to define custom data types without the overhead of defining a full class.

A struct literal type is written as `{ prop1: T1, prop2: T2, ... }`, where `prop1`, `prop2`, etc., are the names of the properties and `T1`, `T2`, etc., are the types of the properties. For example, a struct literal type with two properties, `x` and `y`, would be written as `{ x: i32, y: f64 }`.

Only properties are allowed in struct literal types; no shared variables or functions are permitted.

Struct literal type properties are immutable unless explicitly marked as mutable. To declare a mutable property, use the `var` keyword:
```
{ var x: i32, y: f64 }
```

Struct literal type properties may also have default values. This is done by using the `=` operator:
```
{ x: i32 = 42, y: f64 = 3.14 }
```

Type annotations for each property are still required, even if a default value is provided. The default value must match the type.
The default value must also be a reference to a value with a static lifetime.

Struct literal values may be written using curly braces:
```
{ x: 42, y: 3.14 }
```

Values are assigned to their corresponding properties. By the end of the object literal, all properties must be assigned a value.

You cannot add more properties to a struct literal after it is created.

Struct literal values always use braces; there is no indented form, and they are not considered blocks. Blocks always have some keyword signifying the opening of a block, such as `if`, `while`, or `block`.

Struct literal values always use copy semantics. Use in combination with wrapper classes if different behavior is desired.

## Pointer and reference types

### Raw pointer types

A raw pointer contains the memory address of a value. It is written as `@T`, where `T` is the type of the value being pointed to. Raw pointers can point to null or invalid memory locations. They do not have any lifetime restrictions; they simply store a memory address.

To create a raw pointer, use the `@` operator:
```
let x = 42
let p: @i32 = @x
```

To dereference a raw pointer, use the `^` operator.
Because raw pointers can potentially point to invalid memory locations, dereferencing a raw pointer is considered unsafe and must be done within an `unsafe` block:
```
unsafe:
    let y = ^p
```

Raw pointers are never implicitly dereferenced. You must always use the `^` operator to access the value.

The keyword `var` may be added to the `@` operator and pointer type to allow mutating the pointed-to value.
This is separate from the mutability of the pointer itself, and you can use any combination of mutable and immutable pointers and pointed-to values:
```
let var a = 42
let p1: @i32 = @a            // Immutable pointer with immutable value
let p2: var@i32 = var@a      // Immutable pointer with mutable value
let var p3: @i32 = @a        // Mutable pointer with immutable value
let var p4: var@i32 = var@a  // Mutable pointer with mutable value
```

You cannot have a mutable pointer to a value that was not declared as mutable.
```
let a = 42
let var p: @i32 = @a // Error: a is not mutable
```

You cannot assign an immutable pointer to a mutable pointer.
```
let var a = 42
let p1: @i32 = @a
let p2: var@i32 = var@a // OK
p1 = p2                 // OK; p2 is mutable, but p1 is not
let p3: var@i32 = p1    // Error: p1 is not mutable
```

### Null pointer type

There is a special pointer type called the null pointer type, written as `nullptr`. It has only one value, which is also written as `nullptr`.
```
let p1: nullptr = nullptr
let p2 = nullptr // Type inferred as nullptr
```

The null pointer stores the address `0`. It is often used to indicate that a pointer does not point to any valid memory location. It can be said that `nullptr` is a "pointer to nothing" or "null".

You are not allowed to dereference a pointer whose type is `nullptr`. Attempting to do so will result in a compile-time error.
```
let p = nullptr
unsafe:
    ^p // Error: cannot dereference nullptr
```

This does not mean you are safe from dereferencing null pointers. A raw pointer of any other type may still point to null. We add this check because the only value of the `nullptr` type is `nullptr`, so dereferencing it is never valid.

The `nullptr` type is special in that pointers of this type may be assigned to any other pointer type without requiring an explicit cast.
```
let p1: @i32 = nullptr
```

Here, we specified the type of `@i32`. If we had not, the type would have been inferred as `nullptr`.

### Reference types

A reference is a safe pointer to a value. It is written as `&T`, where `T` is the type of the value. A reference can never be null and the referenced value must live at least as long as the reference.
To create a reference, use the `&` operator:
```
let x = 42
let r: &i32 = &x
```

References can be mutable. Such references are written as `var&T`. They follow the same rules as mutable variables.
```
let var x = 42
let r1 = var&x
let var y = 64
let r2: var&i32 = var&y
```

Like raw pointers, references can be dereferenced using the `^` operator.
References are not automatically dereferenced in any context. You must always use the `^` operator to access the value:
```
let var x = 42
let r: var&i32 = var&x
let y = ^r + 1
^r = 64
```

You can create references to references. The rule of the referent living at least as long as the reference still applies:
```
let var x = 42
let r: var&i32 = var&x            // Must live at least as long as x
let rr: &var&i32 = &r             // Must live at least as long as r
let rrr: &&var&i32 = &rr          // Must live at least as long as rr
^^^rrr = 64
```

Notice how `rr` and `rrr` use `&` and not `var&`. The only value that we are mutating is `x`, so only its reference needs to a `var&` type.

You can also create references to raw pointers and raw pointers to references. The rule of requiring unsafe blocks for dereferencing raw pointers still applies:
```
let x = 42
let p: @i32 = @x
let rp: &@i32 = &p
^rp // Safe; this is just dereferencing a reference
unsafe:
    ^^rp // Unsafe; this is dereferencing a raw pointer
```

# Statements

All programs consist of statements. Statements fall into three categories:
- Declarations: These introduce new names and definitions into the program.
  - Examples: variables, functions, structs
- Expressions: These yield values.
  - Examples: arithmetic expressions, function calls, blocks, control flow expressions
- Non-declaring statements: Statements that neither yield values nor introduce new names.
  - Examples: pass, return, break, continue, yield

## Whitespace

Whitespace is only significant in certain contexts. It is used to separate tokens and check for indentation levels.

Indentation is based on "left spacing", which is the number of spaces or tabs at the beginning of a line up to the next non-whitespace character. 
If a line contains only whitespace, left spacing is ignored for that line.

The lexer tracks left spacing and inserts `INDENT` and `DEDENT` before the next token to indicate changes in indentation. 
Tabs and spaces may be used within the same file, but the left spacing of a single line may not contain a mix of both tabs and spaces.

It is recommended to always use spaces for indentation and to choose a consistent number of spaces for each level of indentation.

An `INDENT` token is inserted when there is a colon, a newline, and an increase in left spacing from the previous line. 
All indent-based blocks (including those used by control structures) require the colon for the `INDENT` token. 
If there is a colon, a newline, but no increase in left spacing, the indent is considered malformed.

A `DEDENT` token is inserted when there is a newline followed by a decrease in spacing to a previous indentation level. If the end of the file is reached, `DEDENT` tokens are inserted to close all open blocks.

Left spacing is not updated after a newline within a grouping token, such as parentheses or braces. As such, no `INDENT` or `DEDENT` tokens are inserted within these tokens.
- This has the side effect of making it impossible for `INDENT` and missing `DEDENT` tokens to generate an "unclosed grouping" error. The grouping stack must always be empty when creating an `INDENT`, and `DEDENTS` cannot be created until all grouping tokens since the last `INDENT` are closed.

In all other instances, whitespace is ignored. There is no dedicated token for whitespace other than the `INDENT` and `DEDENT` tokens. Not even for newlines.

Here is an example of statements with different left spacing:
```
    block:             // (4)
      let x = 42       // (6)
          let y = 64   // (10)
        let z = x + y  // (8)
    let u = 16         // (4)
  let v = 128          // (2)
```

The `block` keyword is followed by a colon, a newline, and an increase in left spacing, so an `INDENT` is inserted. Since the block was opened on a line with a left spacing of 4, all lines with a left spacing greater than 4 are considered to be part of the block.

Here, `x`, `y`, and `z` are all part of the same block, despite having different left spacing. With the `u` declaration, left spacing decreases to 4, where the previous `INDENT` was made. Thus, a `DEDENT` is inserted.

Since left spacing requires the line to contain a non-whitespace character, it follows that every block opened by an `INDENT` must contain at least one non-whitespace character. If an empty block is desired, you can use `pass` or a comment:
```
block:
    pass

block:
    // yes, this works too
```

These are not valid:
```
block:
    if condition:
    pass

block:
    if condition: pass
```

The first if statement has a colon and a newline, but the left spacing does not increase. The indent is not properly formed, resulting in a lexer error.

The second if statement has a colon, but no newline. Thus, the lexer will insert a `COLON` token instead of an `INDENT` token, which will later result in a parser error.

It is recommended to use consistent spacing to improve readability.

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

### Functions

Functions are used to encapsulate code and allow it to be reused. The `func` keyword declares a function. It may be written in indented form, braced form, or short form:
```
func my_function(): // Indented form
    statement1

func my_function() { // Braced form
    statement1
}

func my_function() => expression // Short form
```

Functions may accept arguments by listing parameters in parentheses. Types are always required when listing parameters (except for the instance parameter in methods):
```
func my_function(a: i32, b: i32):
    statement1
```

Function parameters are immutable by default. To declare a mutable parameter, use the `var` keyword:
```
func my_function(var a: i32, b: var&i32, var c: var&i32):
    statement1
```

Function parameters may provide a default value. This is done by using the `=` operator:
```
func my_function(a: i32, b: i32 = 3.14):
    statement1

func my_function(a: i32, b: i32 = 3.14, c: i32):
    statement1
```

Callers may omit arguments for parameters with default values.
The parameter type is still required, even if a default value is provided, and the default value must match the type.

Default values may even be used in the middle of a parameter list. This is considered perfectly acceptable, though it may make the function more difficult to call.

Reference-type parameters are only allowed default values if the default value is a reference to a value with a static lifetime. Strings have a static lifetime, so they are allowed to be used as default values:
```
func my_function(a: i32, b: &str = "Hello, world!"):
    statement1
```

Functions may also return values. The return type is specified using a single arrow `->` after the parameter list and before the block/double-arrow:
```
func my_function(a: i32, b: i32) -> i32:
    yield a + b

func my_function(a: i32, b: i32) -> i32 => a + b // Short form
```

Similar to blocks, the `yield` statement may be used to set the return value of the function. The function will return the value from the last yield statement executed within the function.

You can also use a return statement to exit the function early:
```
func my_function(a: i32, b: i32) -> i32:
    if a == 0:
        return 0
    yield a + b
```

With return types, there are several rules to follow:
1. Functions that do not specify a return type have an implicit return type of `()`.
2. Functions that return `()` have an implicit `yield ()`, meaning no additional `yield` or `return` statements are required.
3. For all functions, the block must yield the expected return type, and all return statements must return the expected type.

```
func f1():
    pass // Okay

func f2():
    yield 42 // Bad

func f3() -> i32:
    pass // Bad
```

### Structs

Named structs are user-defined types that consist of a collection of named properties.

Structs are similar to classes, but they do not support all object-oriented programming features. Specifically:
- They may only inherit from interfaces, not other structs or classes.
- You cannot downcast to a struct pointer type from a base interface pointer type.
- They do not support abstract methods.

The lack of OOP features makes structs simpler and more lightweight than classes. Classes may have more overhead due to features like virtual method tables and inheritance.

They use the following syntax:
```
struct MyStruct: // Indented form
    let x: i32 = 0
    prop y: i32
    func my_func():
        statement1
    method my_method(self):
        statement1

struct MyStruct { // Braced form
    let x: i32 = 0
    prop y: i32
    func my_func():
        statement1
    method my_method(self):
        statement1
}
```

Despite using a similar syntax to blocks, struct definitions are not considered blocks. They are a separate kind of declaration.

Properties, sometimes called instance member variables, are declared with the `prop` keyword. They are similar to variables, but they are stored for each instance of the struct.

Properties may have default values. This is done by using the `=` operator:
```
struct MyStruct:
    prop x: i32 = 0
    prop y: f64 = 3.14
```

Methods, sometimes called instance member functions, are declared with the `method` keyword. They are similar to functions, but they are called on an instance of the struct and have access to the instance's properties and other methods through the instance parameter (named `self` in this example).
```
struct MyStruct:
    prop x: i32

    method my_method(self):
        let y = self.x + 1
```

Method definitions require at least one parameter at the beginning of the parameter list, which will receive the instance of the complex type. Unlike other parameters, this parameter does not require a type annotation. If one is not provided, it is assumed to be `&T`, where `T` is the name of the struct type.

Arguments to the instance parameter cannot be passed in parentheses; the dot notation must be used instead:
```
MyStruct.my_method(s) // Bad; my_method is not a shared function
s.my_method() // OK
```

Because the instance parameter is treated like a parameter inside the function, it follows the same mutability rules as other parameters. If the instance parameter is mutable, the method may modify the instance's properties:
```
method my_method_1(self: &MyStruct) // Immutable reference, immutable object
method my_method_2(self) // Same as above, but without type annotation

method my_method_3(var self: &MyStruct) // Mutable reference, immutable object
method my_method_4(var self) // Same as above, but without type annotation

method my_method_5(self: var&MyStruct) // Immutable reference, mutable object

method my_method_6(var self: var&MyStruct) // Mutable reference, mutable object
```

The instance parameter type may be changed to its non-pointer form. In such cases, the full object is passed to the method. The dot notation is still used to call the method:
```
method my_method(self: MyStruct) // Immutable object

s.my_method() // OK
```

In structs, member variables declared with `let` and member functions declared with `func` are said to be *shared*. This is similar to static members in C++ and Java.
Because they don't have access to `self`, they cannot access instance properties or methods.
Outside the complex type, they are accessed using the struct name:
```
struct MyStruct:
    let x: i32 = 0
    func my_func():
        statement1

func global_func():
    let x = MyStruct.x // Accessing shared member
    MyStruct.my_func() // Calling shared member
```

We reuse the `let` and `func` keywords, because they are stored in a similar manner as other scoped variables and functions.

Shared variables, like local variables, must always be initialized to some value when declared. If no value is provided, a type annotation is required. The variable will be initialized to the default value for the specified type.

To construct an instance of a struct, use the `new` keyword followed by the struct name and a struct literal value:
```
let s = new MyStruct { x = 0, y = 3.14 }
```

There is no special syntax for defining constructors. Instead, use shared functions to create instances of the struct.
```
struct MyStruct:
    prop x: i32
    prop y: f64

    func new() -> MyStruct:
        return new MyStruct { x = 0, y = 3.14 }
```

Structs use copy semantics. If other behavior is desired, use wrapper classes.

### Classes

Classes are user-defined types that consist of a collection of named properties. 

Classes are similar to named structs. They use the same syntax, except they are declared with the `class` keyword instead of `struct`.
```
class MyStruct: // Indented form
    let x: i32 = 0
    prop y: i32
    func my_func():
        statement1
    method my_method(self):
        statement1

class MyStruct { // Braced form
    let x: i32 = 0
    prop y: i32
    func my_func():
        statement1
    method my_method(self):
        statement1
}
```

Classes differ from structs in the following ways:
- They may optionally inherit from a single concrete class or struct, in addition to any number of interfaces.
- They support downcasting to class pointer types from base class or interface pointer types.
- They support abstract methods.

Classes are designed to support object-oriented programming features, making them more suitable for complex data structures and behaviors.
The drawback is that instances of classes require additional memory overhead due to features like virtual method tables and inheritance.

## Expressions

An expression is a piece of code that produces a value. Expressions may be simple literals or more complex combinations of literals, operators, and other expressions.

### Arithmetic expressions

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
We define integer division as:
$$ a / b = trunc(a \div b) $$

Where `a` is the dividend, `b` is the divisor, and $\div$ represents true division.

The percent operator, `%`, is the remainder operator. 
It is not to be confused with modulo, which behaves differently for negative numbers.
Remainder is defined as:
$$ r = a - (a / b) * b $$

Where `a` is the dividend, `b` is the divisor, and `/` uses integer division as defined above.


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

There are two ways to pass arguments to a function:
- **Positional arguments**: These are passed in the order they are defined in the function. For example, `my_function(1, 2, 3)` will pass `1` to the first parameter, `2` to the second parameter, and `3` to the third parameter. Positional must always come first.
- **Named arguments**: These are passed by name and may be in any order. For example, `my_function(c: 3, b: 2, a: 1)` will pass `1` to the `a` parameter, `2` to the `b` parameter, and `3` to the `c` parameter.

When a function is called, every parameter must be matched with an argument, or the call is invalid. You can use a mix of positional, named, and default arguments, as long as the positional arguments come first and every parameter is matched with an argument.

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

Any block can be given a label. The label must be written first, inside the block:
block:
  label "my_block"

  statement1

block {
  label "my_block"

  statement1
}

Blocks may not have more than one label. The label does not need to be globally unique.

Labels are not statements; they simply change the properties of the block they are contained in.
Labels use string literals, but the string literal is not allocated in memory. It is only used for identification purposes.

Labels allow yield statements to specify a target scope. For more information, refer to the section on yield statements.

Note: Structs do not use blocks, and thus, do not support labels.

### If expressions

An if expression is used to conditionally execute code. It can be written using braced-block form, indented-block form, or short-form:
```
if condition {
    statement1
}

if condition:
    statement1

if condition then expression1
```

For short-form if expressions, the `then` keyword is required to separate the condition from the statement.

An if expression may also include an else branch:
```
if condition:
    statement1
else:
    statement2

if condition then expression1 else expression2
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

If expressions yield the values of their branches. If the branches are blocks, they yield the value from the last yield statement executed within the block. If the branches are single expressions, they yield the value of that expression.
```
let x = if condition:
    yield 42
else:
    yield 0
```

All blocks in an if expression must yield the same type.

### Loop expressions

A loop expression is used to repeatedly execute code. When using the `loop` keyword or any other loop with a `true` condition, we call it a non-conditional loop or infinite loop.

It can be written in braced-block form, indented-block form, or short-form:
```
loop { statement1 }

loop:
    statement1

loop expression
```

A loop expression runs indefinitely. To break out of a loop, use the `break` statement.

### While expressions

A while expression is used to conditionally execute code. Except when the condition is literally `true`, a while expression is considered a conditional loop.

It can be written in braced-block form, indented-block form, or short-form:
```
while condition { statement1 }

while condition:
    statement1

while condition do expression
```

Conditional while expressions are only allowed to yield `()`. This is because the while loop may not execute, and thus, cannot guarantee a yield value.

A while-loop may yield values other than `()` if and only if the condition is literally `true`. In such cases, the while-loop is treated like a non-conditional loop expression.

For conditional short-form while expressions, the `do` keyword is required to separate the condition from the statement.
Additionally, unlike with if-statements, the expression in the conditional short-form while expression will not affect the value yielded by the while loop. The while loop will always yield `()`.
This allows users to write the following without worrying about the yield value:
```
while condition do x = x + 1
```

Although the expression yields the value of `x`, the while loop will continue to yield `()`.

### Do-while expressions

A do-while expression is similar to a while expression, but the condition is checked after the block is executed. This means the block will always
execute at least once. 

It may be written in braced-block form, indented-block form, or short-form:
```
do { statement1 } while condition

do:
    statement1
while condition

do expression while condition
```

Like while expressions, conditional do-while expressions are only allowed to yield `()`. This is because the do-while loop may not execute, and thus, cannot guarantee a yield value.

A do-while loop may yield values other than `()` if and only if the condition is literally `true`. In such cases, the do-while loop is treated like a non-conditional loop expression.

The expression in the short-form do-while expression will not affect the value yielded by the do-while loop. The do-while loop will always yield `()`.
```
do x = x + 1 while condition
```

Although the expression yields the value of `x`, the do-while loop will continue to yield `()`.

### Move expressions

A move expression is used to move a value from one location to another. It is written as follows:
```
mv value
```

When a value is moved, the original location is invalidated along with all references to it. This is useful for transferring ownership of a value to a function or another variable.

A mutable value may be moved as mutable:
```
var mv value
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

### Pass statements

A pass statement is a non-declaring statement that does nothing. It may be used whenever a statement is required, but no action is needed.
```
pass
```

Due to the rules of left spacing, blocks opened by an `INDENT` token must contain at least one statement, else the indent is considered malformed. If an empty block is desired, a pass statement may be used:
```
block:
    pass
```

### Continue statements

A continue statement is a non-declaring statement used inside loops to skip the rest of the current iteration and continue to the next iteration.
```
continue
```

### Yield statements

A yield statement is used to set the yield value of a block and possibly exit the block. 
The yield value is the value that the block will yield when executed. 
When a block ends, it will evaluate to the last yield value set:
```
yield value
```

There are three types of yield statements:
- `yield`: Sets the yield value of the current local scope and continues execution. They can only be used within local scopes.
- `break`: Sets the yield value of the nearest enclosing loop and exits the loop. They can only be used within loops.
- `return`: Sets the return value of the nearest enclosing function and exits the function. They can only be used within functions.

Yield statements must always provide a value to be yielded, even if the expected type is `()`. This is to prevent ambiguity in case any statements come after the yield statement.
```
yield ()
break ()
return ()
```

Regular yield statements only yield values within the block they are contained in. To propagate a value out of a block, use multiple yield statements:
```
let x = block:
    yield block:
        yield 42
```

Ironically, `yield` statements themselves do not yield values (they are a non-declaring statement); they only affect the yield values of the surrounding blocks. These are not valid:
```
let x = yield 42
if condition then yield 42 else yield 0
```

When a yield statement is associated with a block, we say it "targets" that block.

Yield statements (except `return`) may use labels to specify which block to target. This is useful for yielding from nested blocks:
```
loop:
    label "outer"

    loop:
        if condition:
            from "outer" break 42
```

Labels do not need to be globally unique. A yield statement will always target the nearest enclosing block with the specified label. If a labeled block contains a block with the same label, a yield statement using the label will always target the inner block.

Labels may be applied to blocks, even if they are not targeted by any yield statement. However, all yield statements must target a valid block. If a yield statement targets a label that does not exist in any enclosing block, it is considered invalid. 
Additionally, `break` statements may only target loop blocks.

Labels are allowed on functions and you can use `yield` to set the yield value of a function. However, we do not allow `return` statements to use labels, as they always target the nearest enclosing function.
