# Type Name Resolution

The following document discusses type name resolution in the Nico programming language, including the requirements, challenges, and proposed solutions.
Type name resolution refers to the process of resolving named types to their actual definitions.

> [!WARNING]
> This document is a work in progress and is subject to change.

## Introduction

The Nico programming language supports various types in its type system.
These include but are not limited to **basic types** (e.g., `i32`, `f64`, `bool`), **pointer types** (e.g., `@i32`, `var@i32`), and **aggregate types** (e.g., `[i32, 4]`, `{ x: i32, y: f64 }`).
The Nico programming language is planned to support another very important kind of type: named types.

A **named type** is not a type itself, but a special name reference to a type definition.
For example, the word `Point` does not mean anything in the base Nico language.
But we can give it a meaning by defining a struct called `Point`:
```
struct Point:
    field x: i32
    field y: i32
```

This declaration introduces a named type called `Point`, which resolves to a struct definition with two fields: `x` and `y`, both of type `i32`.
After this declaration, we can use `Point` as a type in our code:
```
let p: Point = new Point { x: 1, y: 2 }
```

Struct definitions are not the only way to introduce named types.
We can also use type aliases to create named types that refer to existing types:
```
typedef MyInt: i32
```

Type aliases are considered named types because they introduce a new name that, when referenced, must resolve to an actual type definition.
This declaration creates a named type `MyInt` that resolves to the basic type `i32`.
We can then use `MyInt` as a type in our code:
```
let x: MyInt = 42
```

We can use type aliases to give names to object types. These have a similar effect to struct definitions since they are both based on structs internally.
```
typedef Point: { x: i32, y: i32 }
```

Named types are a powerful feature that allows us to create more meaningful and reusable code.
However, they also introduce challenges in type name resolution, including self-referential types, circular references, recursive types, etc.
We require a robust type name resolution mechanism to handle these challenges and ensure that named types are correctly resolved to their definitions.


## Challenges in Type Name Resolution

Here, we will discuss some of the challenges that arise from creating a type name resolution mechanism for the Nico programming language.
Many of these challenges stem from an important feature of the Nico programming language:
declaration space.

The Nico programming language describes two kinds of spaces: **declaration space** and **execution space**.
Declaration and execution space affect the order in which statements are processed.
Whether something is in declaration space or execution space is determined by the type of statement and its scope, not by its position in the source code.

**Declaration space** is where declarations are processed.
This includes static variables, function headers, structs, classes, namespaces, external declaration namespaces, and exports.

**Execution space** is where statements are executed in the order they are written.
Statements that are not in declaration space are in execution space.
This includes but is not limited to assignments, let variable declarations, function calls, loops, conditionals, block expressions, print statements, and heap allocations.

```
                       // Execution space

namespace ns:          // Declaration space
    func foo() {       // Declaration space
        statement1     // Execution space
    }      
    static var x: i32  // Declaration space

statement1             // Execution space

func bar():            // Declaration space
    statement2         // Execution space
```

Declaration space has an important property: the order of declarations does not matter as long as all dependencies are satisfied by the end of the declaration space.
A prime example of this is when we have two functions that call each other:
```
func foo():
    bar()

func bar():
    foo()
```

In this example, both `foo` and `bar` are in declaration space. 
The function `foo` is allowed to call `bar` even though `bar` is declared after `foo`.
This is easy to validate because function calls are execution space only, meaning they are always checked after all declarations are processed.
In this example, by the time the call expression `bar()` is checked, `bar` has already been declared, so the call is valid.
This is a simple example of how declaration space allows for more flexible ordering of declarations, which can lead to more readable and maintainable code.

However, the situation is different with named types.
Consider the following example:
```
typedef Foo: (@Bar)
typedef Bar: (@Foo)
```

This code is valid despite the circular reference between `Foo` and `Bar` due to the use of pointer types, which ensure that the types are not infinitely large.

This case is different from the function example above because both declarations are in declaration space, and there is no execution space to process after the declarations.
Because type checkers must always visit statements one by one, a declaration-space type checker must examine the first declaration and try to resolve `Bar` before it has been declared.

In addition to ensuring the above code is valid, we also must watch out for invalid cases such as these:
```
typedef Foo: Foo

typedef Bar: Baz
typedef Baz: Bar
```

It may be possible to solve our problems by introducing an additional type-checker that runs before the declaration-space type checker.
However, if we unpack the problems further, we can potentially design an algorithm that can be integrated into the declaration-space type checker itself, which would be more efficient and easier to maintain.

### Type Equivalence Problem

Question: If `i32` is given the type alias `MyInt`, is `MyInt` considered equivalent to `i32`?

Currently the Nico programming language describes three forms of type compatibility:
- **Type equivalence**: When two types are considered the same type.
- **Assignment compatibility**: When a value of one type can be assigned to a variable of another type without a transformation.
- **Cast compatibility**: When there exists a valid transformation (cast) that can convert a value of one type to another type.

In one sense, we can say that `MyInt` is not equivalent to `i32`.
They have different names and the former is defined in terms of the latter.
The relationship is not symmetric.

They are definitely assignment compatible since the value of a `MyInt` can be assigned to a variable of type `i32` without any transformation and vice versa.

Consider the following code:
```
let x: MyInt = 1
let y: i32 = 2
let z = x + y
```

When we create a type alias, we want values with the alias type to behave in the same way as values with the original type.
In the above code, we want `x` and `y` to be able to interact with each other as if they were the same type.
We want to allow this addition operation.

However, assignment compatibility alone would not be enough to determine if `x` and `y` can be added together.
Assignment compatibility, as the name implies, is used to check the validity of an assignment, not an addition operation.
And it would be tedious to register new operator overloads for every new type alias we create.

The answer must then be that the compatibility between `MyInt` and `i32` is weaker than type equivalence but stronger than assignment compatibility.
That is, they are in a fourth category of compatibility between type equivalence and assignment compatibility.

We can call this category **direct compatibility** and define it as such:
Two types `A` and `B` are directly compatible if a value of type `A` can be used in any context where a value of type `B` is expected without any transformation, and vice versa.
An alternative definition is that two types `A` and `B` are directly compatible if the underlying LLVM IR types of `A` and `B` are the same.

Some notes about direct type compatibility: 
- It is symmetric, meaning if `A` is directly compatible with `B`, then `B` is directly compatible with `A`.
- If `A` and `B` are directly compatible, then they are also assignment compatible, but not necessarily type equivalent.
- If `A` and `B` are type equivalent, then they are also directly compatible.

This matters a lot in our compiler because of how we currently check for type equivalence.
For example, when we try to type-check access expressions (e.g., `p.x`), we want to make sure that `p` is of an object type or a tuple.
We currently achieve this using code like this:
```cpp
if (auto tuple_type = std::dynamic_pointer_cast<Type::Tuple>(expr->type)) {
    // Handle tuple access
} else if (auto object_type = std::dynamic_pointer_cast<Type::Object>(expr->type)) {
    // Handle object access
}
```

But if the type of `p` is a named type, like `Point`, then the expression's type will be `Type::Named` which is neither a tuple nor an object.

What we want is not to check for type equivalence, but direct compatibility.
We must define a new function that checks for direct compatibility between two types, and use it in our type checker instead of checking for type equivalence.
It may look something like this:
```cpp
expr->type->is<Type::Tuple>();
auto tuple_type = expr->type->as<Type::Tuple>();
expr->type->is_directly_compatible_with(tuple_type);
```

These functions will take the place of `std::dynamic_pointer_cast` and `Type::operator==` and will check for direct compatibility instead of type equivalence.

For most types, the behavior of these functions will be like calling `std::dynamic_pointer_cast`.
But when named types get involved, we can take additional steps to check for direct compatibility.

### Transitive Type Compatibility Problem

This problem is similar to the above problem, but with a small twist.
Consider the following code:
```
typedef CoolInt: i32
typedef EpicInt: CoolInt
```

Given the above code, is `EpicInt` directly compatible with `i32`?

### Self-Referential Problem

### Infinite Pointer Problem

If a named type can reference itself like this:
```
typedef Foo: (@Foo)
```

Then we see no reason to disallow this:
```
typedef Foo: @Foo
```

...

### Circular Reference Problem


