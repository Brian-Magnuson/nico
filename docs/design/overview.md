# Overview

The following is an overview of the Nico programming language design. This document is intended to provide a high-level view of the language and its features.

> [!WARNING]
> This document is a work in progress and is subject to change.

# Data types

## Primitive types

### Integer types

The integer types are signed and unsigned integers of various sizes. The following integer types are supported:
- Signed integers: `i8`, `i16`, `i32`, `i64`
- Unsigned integers: `u8`, `u16`, `u32`, `u64`

Integer literals are assumed to be written in base 10 unless otherwise specified by a base prefix: `0b` for binary, `0o` for octal, and `0x` for hexadecimal.
```
42          // Decimal
042         // Decimal (not octal)
0b101010    // Binary
0o52        // Octal
0x2A        // Hexadecimal
```

Integer literals may include underscores (`_`) to improve readability. These underscores are ignored by the compiler.
```
1_000_000  // One million
```

All integers are assumed to be of type `i32` unless otherwise specified by a type suffix. The type suffix is written as the type name after the literal.
```
42i8       // 8-bit signed integer
42_i8      // Same as above with an underscore for readability
42_u64     // 64-bit unsigned integer
```

For convenience, the following special suffixes are also supported:
- `l` or `L`: Equivalent to `i64`
- `u` or `U`: Equivalent to `u32`
- `ul` or `UL`: Equivalent to `u64` (other combinations of `u`/`U` and `l`/`L` are not supported)

It is recommended to use the full type names for clarity.

If the integer cannot be parsed as the specified type, a lexer error will occur.

Note that the type of an integer will never be inferred from the surrounding context. Only type suffixes are used to determine the type of an integer literal.
```
let x: i16 = 42      // Error: 42 is of type i32, not i16
```

One might expect base-2, base-8, and base-16 literals to be interpreted as raw bit patterns for the desired type. However, this is not the case.

Although they are written like numbers, bit patterns are not numbers; they are representations of numbers. And integer literals are parsed as numbers, not bit patterns.
This means that signed number representations such as two's complement are completely ignored. 
For example, the literal `0b11111111_i8` can be interpreted in two ways:
- The number 255 written in base 2, stored in an 8-bit signed integer
- The 8-bit signed integer bit pattern for -1 in two's complement

Our parser always follows the first interpretation. So `0b11111111_i8` is a parser error, because 255 cannot be represented as an 8-bit signed integer.
The only way to write a negative number is to either use a negative sign or cast an unsigned integer to a signed integer:
```
-0b1_i8              // OK: -1
0b1111_1111_u8 as i8 // OK: -1
0b1111_1111_i8       // Error: 255 cannot be represented as i8
```

### Floating-point types

The floating-point types are `f32` and `f64`, representing 32-bit and 64-bit IEEE 754 floating-point numbers respectively.

Floating-point literals may be written in decimal or scientific notation:
```
3.14        // Decimal notation
2.5e3       // Scientific notation (2.5 * 10^3)
2.5E+3      // Same as above
```

Base prefixes are not allowed for floating-point literals. 
You cannot write FP-numbers using binary, octal, or hexadecimal notation.
This is because we always interpret FP literals as numbers, not bit patterns (even though the bit patterns for FP numbers are well established),
and there is no standard way to represent non-natural numbers in these bases.

FP literals with no fractional part require either a `.0` or a type suffix like `f32` or `f64` (see below) to distinguish them from integer literals. 
FP literals with an absolute value between 0 (inclusive) and 1 (exclusive) require a leading `0` before the decimal point to be recognized as numbers.
```
42.0
0.5
```

Floating-point literals may include underscores (`_`) to improve readability. These underscores are ignored by the compiler.
```
1_000.000_1  // One thousand and one ten-thousandth
```

All floating-point literals are assumed to be of type `f64` unless otherwise specified by a type suffix. The type suffix is written as the type name after the literal.
```
3.14f32       // 32-bit floating-point number
3.14_f32      // Same as above with an underscore for readability
```

For convenience, the following special suffix is also supported:
- `f` or `F`: Equivalent to `f32`

It is recommended to use the full type names for clarity.

If the floating-point number cannot be accurately represented as the specified type, the lexer will try to round it to the nearest representable value.

Note that the type of a floating-point number will never be inferred from the surrounding context. Only type suffixes are used to determine the type of a floating-point literal.
```
let x: f32 = 3.14      // Error: 3.14 is of type f64, not f32
```

There are two special values for floating-point numbers: `inf` and `nan`. These represent positive infinity and "not a number" respectively.
```
let x: f64 = inf       // Positive infinity
```

By default, these values are of type `f64`.

You cannot use type suffixes or underscores with these values. However, you can choose the floating-point type by adding `32` or `64` immediately after the value:
```
let x: f32 = inf32
let y: f64 = nan64
```

Attempting to use a type suffix will not result in an error, but the token will be interpreted as an identifier.
```
let x: f32 = inf_f32 // 'inf_f32' is an identifier, not a floating-point literal
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

An array is a collection of elements of the same type. Arrays come in two varieties: sized arrays and unsized arrays.

For sized arrays, the type is written as `[T; N]`, where `T` is the type of the elements and `N` is the number of elements. For example, an array of 3 integers would be written as `[i32; 3]`.

Arrays are zero-indexed and may be written as literals using square brackets:
```
[1, 2, 3]
```

For array literals, the size is inferred from the number of elements provided.

Array literals that have no elements can be written as `[]`. These arrays have a special type, empty array, also written as `[]`.
Empty arrays have no base type associated with them, but may be assigned to any array with a size of 0.
```
let empty_1 = []            // Implicitly has type []
let empty_2: [i32; 0] = []  // Type becomes [i32; 0]
```

For unsized arrays, the type is written as `[T; ?]`, where `T` is the type of the elements.

The unsized array type can never be used directly. It can only be used as the base type of a raw pointer, written as `@[T; ?]`.

The type can be obtained by casting a sized array pointer to an unsized array pointer:
```
let arr: [i32; 3] = [1, 2, 3]
let p: @[i32; ?] = @arr as @[i32; ?]
```

Implicit bounds checking is not performed on unsized arrays. Accessing the elements of an unsized array is considered unsafe and must be done within an `unsafe` block.

The unsized array type should not be confused with the data structure commonly known as a "dynamic array". Dynamic arrays are implemented using classes and provide additional functionality, such as resizing and automatic memory management.

It is worth noting that arrays are not pointers, do not decay to pointers, and cannot be casted to pointers. Dereferencing can only be done on pointers and array access can only be done on arrays.

Using `=` to assign one array to another will shallow copy the elements of the array. For other copy behaviors, the copy must be done manually.

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

The type of the above literal is written as `(i32)`. The trailing comma is not necessary in the type annotation.

A tuple may also have no elements. It is written as an empty pair of parentheses:
```
()
```

A tuple with no elements is called the unit type. It is used to represent the absence of a value.

Using `=` to assign one tuple to another will shallow copy the elements of the tuple. For other copy behaviors, the copy must be done manually.

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

Using `=` to assign one struct literal value to another will shallow copy the properties of the struct. For other copy behaviors, the copy must be done manually.

## Pointer and reference types

### Raw pointer types

A raw pointer contains the memory address and has no special ownership or lifetime semantics.
There are currently three kinds of raw pointers: raw typed pointers, null pointers, and any pointers.

### Raw typed pointers

A raw typed pointer points to a value of a specific type.
It is a raw pointer because it does not have any ownership or lifetime semantics.
We may simply refer to them as "raw pointers" for brevity.
It is the most basic form of pointer in Nico.

It is written as `@T`, where `T` is the type of the value being pointed to. 
Raw typed pointers can point to null or invalid memory locations.

To create a raw typed pointer, use the `@` operator:
```
let x = 42
let p: @i32 = @x
```

The `@` operator always creates a raw typed pointer.

To dereference a raw typed pointer, use the `^` operator.
Because raw typed pointers can potentially point to invalid memory locations, dereferencing a raw typed pointer is considered unsafe and must be done within an `unsafe` block:
```
unsafe:
    let y = ^p
```

Raw pointers are implicitly dereferenced in access and subscript expressions. For all other contexts, you must use the `^` operator to dereference the pointer.
```
let t = (1, 2, 3)
let tptr = @t
unsafe:
    let first_element = tptr.0    // Implicit dereference
    let second_element = (^tptr).1 // Explicit dereference
```

The keyword `var` may be added to the `@` operator and pointer type to allow mutating the pointed-to value.
This is separate from the mutability of the pointer itself, and you can use any combination of mutable and immutable pointers and pointed-to values:
```
let var a = 42
let p1: @i32 = @a            // Immutable pointer with immutable value
let p2: var@i32 = var@a      // Immutable pointer with mutable value
let var p3: @i32 = @a        // Mutable pointer with immutable value
let var p4: var@i32 = var@a  // Mutable pointer with mutable value
```

You cannot create a mutable pointer to a value that was not declared as mutable.
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

A null pointer, written as `nullptr`, is a special raw pointer that does not have a type and has only one value: `nullptr`.
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

This does not mean you are safe from dereferencing null pointers. 
A raw pointer of any other type may still point to null. 
We add this check because the only value of the `nullptr` type is `nullptr`, so dereferencing it is never valid.

You can assign `nullptr` to any other raw pointer type without requiring an explicit cast (though an explicit cast is allowed).
```
let p1: @i32 = nullptr
```

Here, we specified the type of `@i32`. If we had not, the type would have been inferred as `nullptr`.

For type compatibility purposes, all null pointers are considered mutable, even though there is no pointed-at value to mutate.
Thus, you can safely assign a `nullptr` to both mutable and immutable raw pointer types.

You can use equality operators to compare null pointers with other raw pointers.
```
let p1: @i32 = nullptr
let p2: @f64 = nullptr

let is_nullptr = p1 == nullptr // true
let is_equal = p1 == p2 // true
```

This is useful for checking if a pointer is null, regardless of its base type.

Although it is possible to compare pointers of different types, this feature is usually not useful unless the base types are related in some way (e.g., through inheritance).

### Any pointer type

The any pointer type is a special kind of raw pointer that can be assigned a value of any mutable raw pointer type without requiring an explicit cast or unsafe context.
```
let p1: anyptr = var@x
```

Any pointers cannot be assigned immutable raw pointer types.
This is because any pointers can potentially be used in "any" way, meaning it cannot make any guarantees about whether the pointed-to value will be mutated or not.
```
let p1: anyptr = @x // Error: cannot assign immutable pointer to anyptr
```

Values of type `anyptr` cannot be dereferenced in any way, not even within an `unsafe` block.
Attempting to do so will result in a compile-time error.
```
let p: anyptr = var@x
unsafe:
    ^p // Error: cannot dereference anyptr
    p.member // Error: cannot dereference anyptr
    p[0] // Error: cannot dereference anyptr
```

Although any pointers cannot be dereferenced, they are considered mutable pointers for type compatibility purposes.

The only way to use an any pointer is to perform a reinterpret cast to a specific raw pointer type.
```
let p1: anyptr = var@x
unsafe:
    let p2 = transmute p1 as var@i32
    let y = ^p2
```

The any pointer type is useful for working with low-level or external code that uses raw pointers that would be otherwise incompatible with Nico's raw pointer types.

For example, C functions like `malloc` and `free` use `void*` pointers. But `void` does not exist in Nico. So we use `anyptr` instead:
```
func malloc(size: usize) -> anyptr
func free(ptr: anyptr)
```

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
References are implicitly dereferenced in access and subscript expressions. For all other contexts, you must use the `^` operator to dereference the reference. 
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

### Function pointer types

For every function, its name can be used as a pointer to that function. Function pointers are written as `func(p1: T1, p2: T2, ...) -> TR`. 
```
func my_function(a: i32, b: i32) -> i32:
    yield a + b

let f: func(a: i32, b: i32) -> i32 = my_function
```

If a function is overloaded, attempting to use its name will result in a special function pointer written as `overloadedfn`. This type is tied to the set of function overloads with that name.

When calling an `overloadedfn`, the compiler will attempt to resolve the correct overload based on the argument types provided.
```
func my_function(a: i32) -> i32 { ... }
func my_function(a: f64) -> f64 { ... }

let f = my_function // has type overloadedfn
f(42)      // calls the i32 overload
f(3.14)    // calls the f64 overload
```

The type `overloadedfn` cannot be written as a type annotation; it must be inferred by the compiler.
Additionally, `overloadedfn` is considered incompatible with all other function pointer types, including other `overloadedfn` types.

## Type-of annotation

The type-of annotation is used to refer to the type of an expression. It is written as `typeof(expression)`.
```
let x = 42
let y: typeof(x) = 64
```

The parentheses around the expression are part of the type-of annotation syntax and are always required.
If a 1-tuple is desired, another set of parentheses must be used:
```
let x: typeof((42,));
```

The type-of annotation is an annotation, not an expression. It cannot be used in places where an expression is expected, such as the right side of an assignment or in function arguments.

Because the type-of annotation requires an expression, it can only be used in places where the expression's type is known. For example, you cannot use `typeof` to refer to the type of a variable that has not yet been declared.
```
let y: typeof(x) = 64 // Error: x is not declared yet
let x = 42
```

Note that, although `typeof` requires an expression, the expression is NOT evaluated. It is only used to determine the type.
```
let x: typeof(my_function()) = 37 // OK; my_function is not called
let zero = 0
let y: typeof(1 / zero) = 42 // OK; type is i32, no division by zero occurs
```

Type-of annotations are designed to work with almost any expression, including block expressions.
This means it is possible to write entire statements in an annotation.
These statements can potentially modify the symbol tree, but will never be executed.
```
let x: typeof(block {
    let y = 0
    yield y
}) = 0
```

This code will compile without warnings, but it can be potentially confusing.

## Unsized data types

Data types may be sized or unsized. Sized data types have a known size at compile time, while unsized data types do not.

A data type is unsized if it meets any of the following criteria:
- It is one of the basic unsized types
- It is an aggregate type that contains an unsized type without any levels of indirection (the unsized type is not behind a pointer or reference)
- It is an infinitely recursive type without any levels of indirection

A data type is also considered unsized if it is excessively deeply nested, even if it technically has a finite size. The maximum allowed nesting depth is 256 levels.

There is currently only one basic unsized type:
- The unsized array type: `[T; ?]`
  - Useful when the size of the array is determined at runtime (e.g., when the size needs to change dynamically).

If a type is unsized, memory cannot be allocated for it directly. This means that:
- Variables, function parameters, and function return types cannot be of unsized types
- Data for an unsized type cannot be used as an rvalue expression

An unsized type can be made into a sized type by placing it behind a level of indirection, such as a raw pointer or reference.
```
let a: [i32; ?]   // Error: unsized array type
let p: @[i32; ?]  // OK: pointer to an unsized array
```

Unsized types are usually incompatible with other types, which makes their use limited.
There are some exceptions:
- A pointer to a sized array type (e.g. `@[T; N]`) may be implicitly cast to a pointer to an unsized array type (e.g. `@[T; ?]`). The data remains the same, but the size information is discarded.

As mentioned previously, an aggregate type becomes unsized if it contains an unsized type without any levels of indirection. There are additional rules regarding aggregate types with unsized members:
- Its members cannot be accessed via `.` or `[]` expressions.
  - This is because the offsets of the members cannot be determined if any member is unsized.
- It cannot be instantiated directly.
  - This is because aggregate types are instantiated by providing values for each member, and all rvalue expressions must be of sized types.

These rules effectively prevent the normal use of aggregate types with unsized members, regardless of whether the aggregate type itself is sized or unsized.

Examples of aggregate types containing unsized members include:
- `@[[i32; ?]; 3]` - A sized array of 3 unsized arrays
- `@([i32; ?])` - A tuple containing an unsized array
- `@{ arr: [i32; ?] }` - A struct literal type containing an unsized array

The only way to "use" an aggregate type with unsized members is to perform a reinterpret cast to a pointer to a usable type.
```
let arr: @([i32; 3])
unsafe:
    let p_unsized: @([i32; ?]) = transmute arr as @([i32; ?])
    let p_sized: @([i32; 3]) = transmute p_unsized as @([i32; 3])
    printout p_sized.0[0]
```

Reinterpret casts are a highly unsafe operation and should used with extreme caution.

## Type compatibility

There are three kinds of type compatibility:
- Equivalent: Two types are equivalent if they are the same type.
- Assignment-compatible: Two types are assignment-compatible if a value of one type can be assigned to a variable of the other type without requiring an explicit cast.
- Cast-compatible: Two types are cast-compatible if a value of one type can be cast to the other type using an explicit cast.

For more information on cast compatibility, refer to the section on cast expressions.

Assignment compatibility, as the name implies, is generally based on assignment operations.
It is used for assignment statements, variable initializations, function arguments, and yield/break/return statements.

It is assymetric, meaning `a = b` being valid does not imply that `b = a` is also valid.
To avoid confusion, the "assignment compatibility" of two types should only be described in the context of a specific assignment direction.

Here are some general rules for assignment compatibility:
- All types are assignment-compatible with themselves in both directions.
- A value of type `nullptr` is assignable to any raw pointer type.
- Any mutable raw pointer type is assignable to `anyptr`.
- Any mutable raw pointer type is assignable to an immutable raw pointer type if the base types are assignment-compatible.
- A pointer to a sized array type (e.g. `@[T; N]`) is assignable to a pointer to an unsized array type (e.g. `@[T; ?]`).
- A pointer to a derived class type is assignable to a pointer to a base class type.

Additionally:
- If the type conversion would require the data of the value to change, the types are not assignment-compatible.

Because of this, integer types of different widths are not assignment-compatible, and floating-point types of different widths are not assignment-compatible.

Generally, if a type may be assigned to another type, it is also cast-compatible.
However, not all cast-compatible types are assignment-compatible.

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

If the the type from the type annotation and the type of the initialization expression do not match, the types may still be assignment-compatible.
In such cases, the type from the annotation takes precedence, and the newly declared variable will have that type.
```
let a = nullptr       // The type of `a` is `nullptr`
let b: @i32 = nullptr // The type of `b` is `@i32`
```

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

If you omit the block opening token or `=>`, the function becomes a function header, having no implementation (the declaration ends there).
Function headers are not allowed outside of external declaration blocks. See the corresponding section for more information.

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
    pass // OK

func f2():
    yield 42 // Bad

func f3() -> i32:
    pass // Bad
```

Functions can be called before they are declared. This is because the compiler checks all function declarations before checking other statements.
```
let x = my_function(1, 2)
func my_function(a: i32, b: i32) -> i32:
    yield a + b
```

This has the side effect of disallowing top-level declarations if their identifiers are used to declare functions *later* in the code.
```
block { let x = 64 } // OK - x is scoped to the block.
let x = 42 // Error: x is the name of a function.
func x() -> i32:
    yield 0
```

You can declare a function in the same scope with the same name as another function. This is called function overloading.
Function overloading has rules that must be followed:
- When a new overload is declared, and
- When an overloaded function is called.

When a new overload is declared, the new overload must be "sufficiently different" from all existing overloads to avoid potential ambiguity.

Two function overloads conflict if they have the same set of parameters, or if one set of parameters is a superset of the other, differing only by optional parameters.

The formal rules for this are defined below:
1. Let $f_i$ be one of multiple overloads of a function $f$.
2. A **parameter string** is the concatenation of a parameter's name and type, excluding any default value.
   - E.g., for `a: i32 = 5`, the parameter string is `"a: i32"`.
3. Let $M(f_i)$ be the set of parameter strings for *all* parameters of $f_i$.
4. Let $D(f_i)$ be the subset of $M(f_i)$ consisting of parameter strings for parameters that have default values.
5. Consider two overloads $f_1$ and $f_2$ of $f$.
6. If $M(f_1) = M(f_2)$, then $f_1$ and $f_2$ conflict.
7. If $M(f_1) \supset M(f_2)$ and $M(f_1) - M(f_2) \subseteq D(f_1)$, then $f_1$ and $f_2$ conflict.
8. If $M(f_2) \supset M(f_1)$ and $M(f_2) - M(f_1) \subseteq D(f_2)$, then $f_1$ and $f_2$ conflict.
9. Otherwise, $f_1$ and $f_2$ do not conflict.

```
func my_function(a: i32, b: i32): // OK - first overload
    statement1

func my_function(c: i32, d: i32): // OK - different parameter strings
    statement1

func my_function(a: i32, b: i32 = 3): // Error - conflicts with first overload
    statement1
```

Based on these rules, two functions also cannot differ by only their return types.

When an overloaded function is called, the compiler will attempt to match the provided arguments to one of the overloads. If no overload matches, or if multiple overloads match, a compile-time error will occur.
```
func my_function(a: i32): // First overload
    statement1
func my_function(b: i32): // Second overload
    statement1

my_function(a: 37) // OK - calls first overload
my_function(c: 42) // Error - no matching overload
my_function(64)    // Error - multiple matching overloads
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
let s = new MyStruct { x: 0, y: 3.14 }
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

### External declaration blocks

External declaration blocks are used to declare functions or variables that are defined outside of the current module. This is useful for interfacing with code written in other programming languages or for linking to external libraries.

To create an external declaration block, use the `extern` keyword followed by either a colon or an opening curly brace. The block may be written in indented form or braced form:
```
extern:
    func external_function(a: i32) -> i32
    let external_variable: f64

extern {
    func external_function(a: i32) -> i32
    let external_variable: f64
}
```

Despite using a similar syntax to block expressions, extern blocks are not block expressions. They are a kind of declaration, i.e., a statement.

Inside an external declaration block, you may only declare the following:
- Variables using `let` with a type annotation and no initializer.
- Functions using `func` without a body (function headers).

External declaration blocks allow you to declare variables and functions that are defined in external code, such as C libraries.
- With variables, the identifier and type should match the external definition.
- With functions, the identifier, parameter types (but not parameter identifiers), and return type should match the external definition.

Currently there is no way to specify a different linkage name for an external variable or function. You can, however, create a wrapper function or variable with the desired name.

You can add a string literal after the `extern` keyword to specify the ABI to use.
Currently, the only supported ABI is `"C"`.
```
extern "C":
    func c_function(a: i32) -> i32
    let c_variable: f64
```
Using an unsupported ABI will result in a compile-time error.
If you do not specify an ABI, the default ABI for the target platform will be used (currently "C").

Extern blocks may only be placed at the top level. 
They cannot be nested inside other blocks, functions, or complex types.

Externally-declared variables/functions may be referenced/called in safe contexts without requiring an `unsafe` block.
It is the user's responsibility to ensure that the external code is safe to use.

## Expressions

An expression is a piece of code that produces a value. Expressions may be simple literals or more complex combinations of literals, operators, and other expressions.

### Operator precedence and associativity

Operator precedence determines the order in which operators are evaluated in an expression. Operators with higher precedence are evaluated before operators with lower precedence.

In the following list, lower numbers (listed first) indicate higher precedence:
1. Primary expressions: literals, name references, blocks, conditionals, loops, grouping expressions, `sizeof`, `alloc`
2. Postfix expressions: `()` (function call), `[]`, `.`
3. Unary expressions: `-`, `not`, `!`, `^` (indirection)
4. Cast expression: `as`
5. Multiplicative expressions (L to R): `*`, `/`, `%`
6. Additive expressions (L to R): `+`, `-`
7. Bit shift expressions (L to R): `<<`, `>>`
8. Bitwise AND expressions (L to R): `bitand`
9. Bitwise XOR expressions (L to R): `bitxor`
10. Bitwise OR expressions (L to R): `bitor`
11. Comparison expressions (L to R): `<`, `<=`, `>`, `>=`
12. Equality expressions (L to R): `==`, `!=`
13. Logical AND expressions (L to R): `and`
14. Logical OR expressions (L to R): `or`
15. Assignment expressions (R to L): `=`, `+=`, `-=`, `*=`, `/=`, `%=`

Note that bitwise expressions have higher precedence than comparison expressions.
This is different from C's operator precedence, which places bitwise expressions below comparison expressions.

Parentheses may be used to group expressions and override the default precedence:
```
let x = 3 + 4 * 5       // Multiplication, then addition; x = 23
let y = (3 + 4) * 5     // Addition, then multiplication; y = 35
```

There are no errors or warnings for not using parentheses to clarify precedence or using parentheses where they are not needed.
Users are encouraged to use parentheses where they improve readability.

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
bitnot a
a bitand b
a bitor b
a bitxor b
a << b
a >> b
```

The bitwise not, and, or, and xor operators use the keywords `bitnot`, `bitand`, `bitor`, and `bitxor` respectively.
The bit shift operators use the symbols `<<` and `>>`.

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

If the left side of the dot operator is a pointer type, it is automatically and fully dereferenced.
```
let var p1: &MyClass = &my_object
let x = (^p1).field // Explicit dereference
let y = p1.field   // Automatic dereference

let var p2: &&MyClass = &p1
let z = p2.field   // Automatic full dereference
```

If the pointer is a raw pointer, dereferencing is still implicit, but must be done within an `unsafe` block:
```
let var p: @MyClass = @my_object
unsafe:
    let x = (^p).field // Explicit dereference
    let y = p.field    // Automatic dereference
```

### Subscript expressions

Subscript expressions are used to access elements of arrays. These use square brackets:
```
array[0]
```

Similar to access expressions, if the left side of the square brackets is a pointer type, it is automatically and fully dereferenced.
```
let var p1: &[i32; 3] = &my_array
let x = (^p1)[0] // Explicit dereference
let y = p1[0]   // Automatic dereference
```

If the pointer is a raw pointer, dereferencing is still implicit, but must be done within an `unsafe` block:
```
let var p: @[i32; 3] = @my_array
unsafe:
    let x = (^p)[0] // Explicit dereference
    let y = p[0]    // Automatic dereference
```

### Size-of expression

The size-of expression is used to get the size, in bytes, of a type. This uses the `sizeof` keyword:
```
let size = sizeof i32
```

The `sizeof` operator requires an annotation, not an expression. However, you can use it in conjunction with the type-of annotation to get the size of an expression's type:
```
let x = 42
let size = sizeof typeof(x)
```

Note that the expression under `typeof` is not evaluated; it is only used to determine the type.
```
let size = sizeof typeof(1 / 0) // OK; type is i32, no division by zero occurs
```

### Type-test expression

A type-test expression is used to check if two pointers to related class types point to the same type. The expression yields a boolean value indicated the result of the type test.
This uses the `is` keyword:
```
if p1 is @DerivedClass:
    // p1 points to an object of type DerivedClass
```

The `is` operator may only be used with pointers to related class types. Any other usage will result in a compile-time error.

Type-test expressions do not modify the pointer. However, they are generally used in conjunction with downcasting to ensure the downcast is valid.

### Cast expressions

A cast expression is used to convert a value from one type to another. This uses the `as` keyword:
```
let x = 42 as f64
```

Only certain casts are allowed and the exact effects of the cast may differ based on the types involved.

You are allowed to cast a value to its own type. This will have no effect and may result in a warning.

**Numeric casts**

All numeric types may be cast to any other numeric type or boolean. 
Such casts must always be explicit; there are no implicit numeric casts.

The following rules apply:
- Any integer to signed integer:
  - If the expression type is wider than the target type, the value is truncated toward zero to fit the target type.
  - If the expression type is narrower than the target type, the value is sign-extended to fit the target type.
- Any integer to unsigned integer:
  - If the expression type is wider than the target type, the value is truncated toward zero to fit the target type.
  - If the expression type is narrower than the target type, the value is zero-extended to fit the target type.
- Any integer to floating-point:
  - The integer value is converted to the nearest representable floating-point value.
- Floating-point to any integer:
  - The FP value is clamped (runtime check) to the range of the target integer type and any fractional part is truncated toward zero.
    - If the FP value is `NaN`, the result is `0`.
- Floating-point to floating-point:
  - The FP value is rounded to the nearest representable value in the target type.
- Any number to boolean:
  - The value becomes true if the original value is non-zero, and false if the original value is zero.
- Boolean to any number:
  - The value becomes `1` for true and `0` for false.

The `char` data type is not a numeric type. However, it may be cast to any numeric type and vice versa. In such cases, the `char` type follows the same rules as if it were a `u32` type.

These rules are designed such that the result is always well-defined.
That is, the program will not panic or produce undefined behavior in the event of overflow, underflow, truncation, clamping, or loss of precision.
It is the programmer's responsibility to ensure the cast behaves as intended.

**Nullptr casts**

You are allowed to cast `nullptr` to any pointer type:
```
let p: @i32 = nullptr as @i32
```

This is usually not necessary, as `nullptr` may be assigned to any pointer type without requiring an explicit cast.

**Array pointer casts**

When casting between array pointer types, the types must either be equivalent, or the target type must be a pointer to an unsized array with the same element types.
```
let p1: @[i32; 3] = @my_array
let p2: @[i32; ?] = p1 as @[i32; ?] // OK
```

**Class casts**

You can cast between class pointer types if there is an inheritance relationship between the classes. This is called an upcast or downcast.
Upcasts (casting to a base class) are always safe and may be done implicitly. Downcasts (casting to a derived class) must be done explicitly.
```
let p1: @DerivedClass = @my_derived_object
let p2: @BaseClass = p1 // Upcast (implicitly allowed)
let p3: @DerivedClass = p2 as @DerivedClass // Downcast (explicitly required)
```

If the pointer base types are unrelated, the cast will result in a compile-time error.

Downcasting is allowed in safe contexts. However, if the object being pointed to is not actually of the target derived class type, the expression will panic at runtime.

Because of this, it is recommended to use type-test expressions to verify the type before downcasting.
```
if p2 is @DerivedClass:
    let p3: @DerivedClass = p2 as @DerivedClass
    // Safe to use p3 here
else:
    // Handle the case where p2 is not a DerivedClass
```

**Reinterpret casts**

Reinterpret casts allow you to reinterpret the bits of any value as another type as long as the source and target types have the same size. The bits are left unchanged during the cast.

The most raw form of a reinterpret cast is the transmute cast, which checks that the source and target types have the same size and nothing else.
It is an unsafe operation and must be done within an `unsafe` block:
```
let f = 3.14
unsafe:
    let i: i64 = transmute f as i64 // Reinterpret cast
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

### Allocation expressions

> [!CAUTION]
> The information in this section is currently inconsistent with the current implementation.
> The current implementation is planned to be updated to match this documentation.

An allocation expression is used to manually allocate heap memory for a type. This uses the `alloc` keyword:
```
alloc type
```

You can optionally provide an expression to initialize the allocated memory:
```
alloc type with expression
```

If you provide an expression, you can omit the type, and it will be inferred from the expression:
```
alloc with expression
```

The amount of memory allocated is determined by the size of the type.
The allocation expression always yields a mutable raw pointer of the allocated type.
```
let p: var@i32 = alloc i32
```
The pointer is always mutable. If the user wishes to have an immutable pointer, they must either cast the value or assign it to a variable whose type is an immutable pointer.
```
let p1 = alloc i32         // Mutable pointer
let p2: @i32 = alloc i32   // Immutable pointer via type annotation
let p3 = alloc i32 as @i32 // Immutable pointer via cast
```

You can use the same syntax to allocate memory for sized arrays:
```
let p: var@[i32; 3] = alloc [i32; 3] with [1, 2, 3]
```

Note that this syntax only works for sized arrays, meaning the size cannot be changed. 
You cannot use an expression in the array type because the array type annotation requires a constant size.
```
alloc [i32; n] // ERROR: Array type requires a literal size

alloc [i32; ?] // ERROR: Cannot allocate memory for unsized type
```

Occasionally, you may want to allocate a dynamic amount of memory using an expression.
You can do this using the `alloc for` syntax:
```
alloc for amount_expr of base_type
```

The amount of memory allocated is determined by multiplying the size of `base_type` by the value of `amount_expr`.
```
let n = 100
let p: var@[i32; ?] = alloc for n of i32
```
In the above example, `n` is 100, so 400 bytes of contiguous memory (100 * 4 bytes) will be allocated.

A runtime check is performed to ensure that `amount_expr` is non-negative. If it is negative, the program will panic at runtime.

The yielded type will be a mutable unsized array pointer with `base_type` being the base type of the array (`var@[base_type; ?]`).
With unsized arrays, you can access elements in unsafe contexts without runtime checks, but you cannot change the type without a reinterpret cast.

You cannot initialize memory when using the `alloc for` syntax, and `with` is not allowed.

The `alloc` keyword may be used in safe contexts.

If allocation fails (e.g., due to insufficient memory), the program will panic at runtime.

The allocated memory will remain valid until it is explicitly deallocated using the `dealloc` statement.
```
unsafe:
    dealloc p
```

Deallocation statements are a kind of non-declaring statement.
For more information, refer to the section on deallocation statements.

Deallocation is an unsafe operation and must be done within an `unsafe` block.

If the allocated memory is not explicitly deallocated, the memory will remain allocated until the program terminates. 
Continued failure to deallocate memory will result in less available memory for the program, potentially impacting performance or causing the program to run out of memory.
It is the programmer's responsibility to ensure allocated memory is properly deallocated when no longer needed.

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

The type of a block expression is based on the first yield statement from the top that targets the block.
All other yield statements must yield a type that is assignment-compatible with this type.
```
block:
    yield 42       // First yield; block type is i32
    yield 100      // OK; yields i32
    yield 3.14     // Error; yields f64, incompatible with i32
```

Any block can be given a label. The label must be written first, inside the block:
```
block:
  label "my_block"

  statement1

block {
  label "my_block"

  statement1
}
```

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
mv var value
```

### Unsafe blocks and expressions

An unsafe block is like a regular block, but allows the use of unsafe operations. 

An unsafe operation is typically an operation that can potentially result in 
undefined behavior and has no implicit runtime checks to prevent errors.

Unsafe operations include:
- Dereferencing raw pointers
- Accessing elements of an unsized array
- Performing pointer arithmetic
- Perform a reinterpret cast
- Calling unsafe functions
- Deallocating memory using `dealloc`

Unsafe blocks are used to encapsulate unsafe operations and prevent them from leaking into safe code. They are written as follows:
```
unsafe:
    statement1
    statement2
```

Unsafe blocks behave like plain blocks and can be used to yield values:
```
let a = 1
let p = @a
let b = unsafe { yield ^p }
```

Unsafe blocks may still be written within safe blocks. Memory errors may occur, but they can at least be traced back to these unsafe blocks.

Unsafe blocks do not propagate into nested blocks. If the nested block contains unsafe operations, it must also be marked as unsafe.
```
unsafe:
    statement_1 // In unsafe context
    block:
        statement_2 // In safe context; error if unsafe operation
    statement_3 // In unsafe context
```

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

### Deallocation statements

A deallocation statement is used to deallocate memory that was previously allocated using the `alloc` keyword. It is written as follows:
```
dealloc pointer
```

Deallocation statements can only be used with raw pointers whose type is not `nullptr`.

If the pointer is to memory that was not allocated with `alloc` or has already been deallocated, the behavior is undefined.

If the pointer is `nullptr`, nothing happens.

Deallocation statements must be used within an `unsafe` block, as deallocating memory incorrectly can lead to memory errors.

When a pointer is deallocated, the memory it points to is freed and may be reused for other allocations. It is the programmer's responsibility to ensure that the pointer being deallocated was previously allocated using `alloc` and has not already been deallocated.
```
let p: @i32 = alloc 42
unsafe:
    dealloc p
```

# Core Library

The Nico core library is a special set of built-in functions, types, and constants that are essential and always available to Nico programs.

It should not be confused with the standard library, which is a larger, optional set of functions, types, and constants that provide additional functionality.

The core library:
- Is designed to be minimal.
- Is always available without needing to import anything.
- Is implemented as part of the compiler.
- Is tightly integrated with the language itself.

The standard library:
- Is designed to be comprehensive.
- Must be explicitly imported to be used.
- Is implemented as separate modules.
- Is built on top of the language and core library.

The core library contains constructs that are too fundamental to be implemented in the standard library and too complex to be implemented as part of the language itself.

The following is a non-exhaustive, non-comprehensive list of items in the core library:
- `Iterable`: An interface for types that can be iterated over.
- `Range`: A lazily-evaluated sequence of values that can be iterated over.
- `Option`: A type that represents an optional value.
- `Result`: A type that represents either a success or an error.
- `Promise`: A type that represents a value that may be available in the future.
- `Sptr`: A smart pointer that manages shared ownership of a value using reference counting.
- `Wptr`: A weak pointer that holds a non-owning reference to a value managed by a `Sptr`.
