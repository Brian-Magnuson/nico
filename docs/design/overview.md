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

### Object type

An object type is a custom data type representing a mapping of identifiers to values. They are similar to complex types, but with several differences:
- They are not defined using a class or struct; they are usually used when needed.
- They are not identified by a name (except when using a type alias).
- They are use move semantics, regardless of the internal types, and are not directly copyable.
- They can only contain properties; no shared variables or functions are allowed.

Additionally, you cannot add more properties to an object after it is created.

Object types offer a flexible way to define custom data types without the overhead of defining a full class or struct.

An object type is written as `{ prop1: T1, prop2: T2, ... }`, where `prop1`, `prop2`, etc., are the names of the properties and `T1`, `T2`, etc., are the types of the properties. For example, an object with two properties, `x` and `y`, would be written as `{ x: i32, y: f64 }`.

Object type properties are immutable unless explicitly marked as mutable. To declare a mutable property, use the `var` keyword:
```
{ var x: i32, y: f64 }
```

Object type properties may also have default values. This is done by using the `=` operator:
```
{ x: i32 = 42, y: f64 = 3.14 }
```

Type annotations for each property are still required, even if a default value is provided. The default value must match the type.
The default value must also be a reference to a value with a static lifetime.

Objects may be written as literals using curly braces:
```
{ x: 42, y: 3.14 }
```

Values are assigned to their corresponding properties. By the end of the object literal, all properties must be assigned a value.

Objects always use braces; there is no indented form, and they are not considered blocks. Blocks always have some keyword signifying the opening of a block, such as `if`, `while`, or `block`.

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
  - Examples: variables, functions, structs
- Expressions: These yield values.
  - Examples: arithmetic expressions, function calls, blocks, control flow expressions
- Non-declaring statements: Statements that neither yield values nor introduce new names.
  - Examples: pass, return, break, continue, yield

## Whitespace

Whitespace is only significant in certain contexts. It is used to separate tokens and check for indentation levels.

Indentation is based on "left spacing", which is the number of spaces or tabs at the beginning of a line up to the next non-whitespace character. If a line contains only whitespace, left spacing is ignored for that line.

The lexer tracks left spacing and inserts `INDENT` and `DEDENT` before the next token to indicate changes in indentation. Tabs and spaces may be used within the same file, but the left spacing of a single line may not contain a mix of both tabs and spaces.

It is recommended to always use spaces for indentation and to choose a consistent number of spaces for each level of indentation.

An `INDENT` token is inserted when there is a colon, a newline, and an increase in left spacing from the previous line. All indent-based blocks (including those used by control structures) require the colon for the `INDENT` token.

A `DEDENT` token is inserted when there is a newline followed by a decrease in spacing to a previous indentation level. If the end of the file is reached, `DEDENT` tokens are inserted to close all open blocks.

Left spacing is not updated after a newline within a grouping token, such as parentheses or braces. As such, no `INDENT` or `DEDENT` tokens are inserted within these tokens.

In all other instances, whitespace is ignored. There is no dedicated token for whitespace other than the `INDENT` and `DEDENT` tokens.

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

Since left spacing requires the line to contain a non-whitespace character, it follows that every block opened by an `INDENT` must contain at least one statement. If an empty block is desired, `pass` may be used:
```
block:
    pass
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

Functions are used to encapsulate code and allow it to be reused. The `func` keyword declares a function:
```
func my_function(): // Indented form
    statement1

func my_function() { // Braced form
    statement1
}
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

Functions may also return values. The return type is specified after the parameter list:
```
func my_function(a: i32, b: i32) -> i32:
    yield a + b
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

### Complex types

> [!WARNING]
> This section is subject to change.

Complex types are user-defined types that are composed of other types. There are two kinds of complex types:
- **Structs**: Basic complex types that use copy semantics (but can be moved if needed). All properties must also be struct-types or primitive types.
- **Classes**: Complex types that are not copyable (but can be referenced or moved).

Struct-types are meant for complex types where copying is relatively quick and cheap.

The syntax for declaring structs and classes is similar:
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

class MyClass: // Class
    let x: i32 = 0
    prop y: i32
    func my_func():
        statement1
    method my_method(self):
        statement1
```

Complex type definitions may only consist of declarations. Expressions and non-declaring statements are not allowed.

> [!WARNING]
> This part is subject to change.

Currently, inner complex types are not allowed. This may change in the future.

In complex types, member variables declared with `let` and member functions declared with `func` are said to be *shared*. This is similar to static members in C++ and Java. Outside the complex type, they are accessed using the class/struct name:
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

Complex types may have properties (also known as instance member variables), which are declared with `prop`. Unlike shared variables, they are stored for each instance of the complex type. Properties are accessed using the instance name:
```
struct MyStruct:
    prop x: i32

func global_func():
    let s = MyStruct()
    let x = s.x // Accessing property
```

Properties may have default values. This is done by using the `=` operator:
```
struct MyStruct:
    prop x: i32 = 0
    prop y: i32 = 3.14
```

Type annotations for each property are still required, even if a default value is provided. The default value must match the type. If the type is a reference, any default value must be a reference to a value with a static lifetime.

Complex types may also have methods (also known as instance member functions), which are declared with `method`. Methods are similar to functions, but they are called on an instance of the complex type. They also have access to the instance's properties and other methods through the instance parameter (named `self` in this example):
```
struct MyStruct:
    prop x: i32
    method my_method(self):
        let y = self.x // Accessing property
        self.my_other_method() // Calling method
    method my_other_method(self):
        pass

func global_func():
    let s = new MyStruct { x = 0 }
    s.my_method() // Calling method
```

Method definitions require at least one parameter at the beginning of the parameter list, which will receive the instance of the complex type. Unlike other parameters, this parameter does not require a type annotation. If one is not provided, it is assumed to be `&T`, where `T` is the type of the complex type.

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

The instance parameter type may be changed to its non-pointer form. In such cases, the full object is passed to the method. Whether copy or move semantics are used depends on the kind of complex type. The dot notation is still used to call the method:
```
method my_method(self: MyStruct) // Immutable object

s.my_method() // OK
```

To construct an instance of a complex type, use the `new` keyword followed by the type name and an object literal:
```
let s = new MyStruct { x: 0 }
```

The braced part is not a true object literal, but the mapping syntax is the same.

By the end of the object literal, all properties must be assigned a value.

There is no special way to define a constructor, but you can define shared functions that act as constructors:
```
struct MyStruct:
    prop x: i32
    prop y: i32

    func init(x: i32, y: i32) -> MyStruct:
        return new MyStruct { x: x, y: y }
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

### Return statements

A return statement is used to exit a function and yield a value to the caller:
```
return value
```

Return statements can only be used within functions.

Return statements must always provide a value to be returned, even if the expected type is `()`. This is to prevent ambiguity in case any statements come after the return statement.
```
return ()
```

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

Similar to return statements, yield statements must always provide a value to be yielded, even if the expected type is `()`. This is to prevent ambiguity in case any statements come after the yield statement.
```
yield ()
```

Ironically, `yield` statements themselves do not yield values (they are a non-declaring statement); they only affect the yield values of the surrounding blocks. This is not valid:
```
let x = yield 42
```
