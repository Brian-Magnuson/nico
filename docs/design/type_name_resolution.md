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

## Why Type Name Resolution Is Challenging

The main challenges of type name resolution stem from a particular feature of our language: declaration space.

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
typedef Bar = Foo
typedef Foo = i32
```

This case is different from the function example above because both declarations are in declaration space, and there is no execution space to process after the declarations.
Because type checkers must always visit statements one by one, a declaration-space type checker must examine the first declaration and try to resolve `Bar` before it has been declared.

We'll explore how to solve this problem in later sections.


## How Names Work

First, we need to understand that a named type has a *name*.
In the Nico programming language, a *name* is not merely an identifier.

A **name** is a string consisting of one or more parts separated by `::` and used to refer to a node in the symbol tree.

When we *declare* a named type, we use an **identifier**, which is a single-part string to introduce the binding.
But when we *reference* a named type elsewhere, we use its *name* to look it up in the symbol tree.

Since named types use names, they must also be scoped.
So if you declare a named type in a particular scope, you can only reference it from within that scope or from scopes that can see it (e.g., through namespace qualification).
```
namespace ns:
    typedef MyType: i32

let x: ns::MyType = 42
```

Additionally, since named types use names, they must have a node in the symbol tree that represents them.
A type annotation may use different names depending on the surrounding scope, but the name resolution system ensures that these names resolve to the same node in the symbol tree.

We actually already do this with our primitive types.

There is no type annotation subclass for `i32`.
When the user writes `i32` for a type annotation, they are actually writing a named type annotation that references the primitive type node `i32` in the symbol tree.
So, in a sense, `i32` is a named type.

We can draw our relationships like this:
```
i32 => Type::Int -> Node::PrimitiveType(i32)
MyType => Type::Named(MyType) -> Node::TypeDef(MyType) -> Type::Int -> Node::PrimitiveType(i32)
```

On the left, we have the named type annotations that the user writes in their code.
The double arrows show how the annotation checker resolves these type annotations to actual type objects.


## Chaining Named Types

Named types can reference other named types, creating chains of type dependencies.
For example, if we have:
```
typedef Point: { x: i32, y: i32 }
typedef Rectangle: { top_left: Point, bottom_right: Point }
```

Here, `Rectangle` is a named type that references the named type `Point`.
When the type checker resolves `Rectangle`, it will follow the chain to resolve `Point` as well.

Let's look at a simpler example to illustrate the concept:
```
typedef A: i32
typedef B: A
```

We draw the relationships like this:
```
A => Type::Named(A) -> Node::TypeDef(A) -> Type::Int -> Node::PrimitiveType(i32)
B => Type::Named(B) -> Node::TypeDef(B) -> Type::Named(A) -> Node::TypeDef(A) -> Type::Int -> Node::PrimitiveType(i32)
```

Notice this alternating pattern of Node and Type objects in the chain: each named type references a Node in the symbol tree, which resolves to a Type, and this Type may in turn reference another Node, creating a chain of Node -> Type -> Node -> Type transitions.

## When A Name Is Not Yet Known

In some cases, a name may not yet be known when the type checker encounters it. 

Because type name definitions are processed in declaration space, the type name resolution system must account for the possibility that a name may not yet be available when it is first referenced.

Let's consider a simple example to illustrate this:
```
typedef A: B
typedef B: i32
typedef C: A
```

In this example, when the type checker encounters the definition of `A`, it tries to resolve `B` immediately. However, at that point, `B` has not yet been defined, so the resolution fails. 
The type name resolution system must be able to handle this situation by deferring the resolution of `A` until `B` is available, allowing the type checker to complete its processing once all necessary types have been defined.

One approach to handling this is to pretend like we never visited the first statement, storing it in a special list
to revisit later once the missing name becomes available.
At first, this may seem like a good idea since these statements are so simple.
However, it won't work for circular references, where each type in the cycle depends on the other, creating an infinite loop of deferrals.

### Placeholder Node Approach

It may seem as though creating a placeholder node would be impossible since nodes require a place in the symbol tree, meaning they must have a scope.
And we cannot tell from a name alone what scope the node would belong to.

But this isn't a problem as long as we don't *use* the node as a real node.

It would have no name, no symbol, no parent, and no children.
Attempting to call any of the node's normal functions would result in an error.

The placeholder node would also include a back-reference to the named type that references it, allowing us to update the named type's reference once the actual node is available.

```
A => Type::Named(A) -> Node::TypeDef(A) -> Type::Named(B) -> Node::UnresolvedType(B)
```

Here, we can create the type object `Type::Named(B)` even though we don't actually know if `B` exists yet.
We just assume that it will be defined somewhere in declaration space and create a placeholder node `Node::UnresolvedType(B)` to represent it until we can resolve it.

It is possible that, before encounter `B`, we encounter another named type that also references `B`:
```
C => Type::Named(C) -> Node::TypeDef(C) -> Type::Named(B) -> Node::UnresolvedType(B)
```

In this case, we create another `Type::Named(B)`. This type object will be different from the one for `A` and will also reference a different placeholder node.

If you think about it, this makes sense: we can't rule out that the `B` referenced by `C` is different from the `B` referenced by `A`, since they could be in different scopes.

So for now, we create two separate unresolved nodes for `B`, one for `A` and one for `C`.

Then, once we finally encounter `B`, both of these nodes can be resolved to the same actual node for `B`, and both `A` and `C` will correctly resolve to the same type.

```
B => Type::Named(B) -> Node::TypeDef(B) -> Type::Int -> Node::PrimitiveType(i32)

A => Type::Named(A) -> Node::TypeDef(A) -> Type::Named(B) -> Node::TypeDef(B) -> Type::Int -> Node::PrimitiveType(i32)

C => Type::Named(C) -> Node::TypeDef(C) -> Type::Named(B) -> Node::TypeDef(B) -> Type::Int -> Node::PrimitiveType(i32)
```

In this scenario, the type object `Type::Named(B)` for both `A` and `C` *are different*, but they will both resolve to the same actual node for `B`, which is `Node::TypeDef(B)`.

To enable this solution, we need to avoid accessing the node field until the end of declaration space.
In face, we should avoid any case where we attempt to access a type's properties.

This means:
- No checking the size of types
- No checking the compatibility of types
- No checking the fields of types

This also means we cannot check *expressions* in declaration space since expression checking involves checking the types of expressions, which may involve checking the properties of types.
But declaration space is not meant for expression checking anyway, so this is not a problem.


## Type Equivalence Problem

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
Type::is_a<Type::Tuple>(expr->type);
auto tuple_type = Type::as_a<Type::Tuple>(expr->type);
expr->type->is_directly_compatible_with(tuple_type);
```

These functions will take the place of `std::dynamic_pointer_cast` and `Type::operator==` and will check for direct compatibility instead of type equivalence.

## Self-Referential Problem

Question: Can a named type be defined in terms of itself?

There are a few ways to do this, and, as we'll see, only some of them are valid.

Let's start with this first case:
```
typedef Foo: Foo
```
This is obviously invalid, though not for the reason that it is circular.

When the type checker first tries to resolve `Foo`, it will see that it is defined in terms of `Foo`, which has not been fully resolved yet.
The next step would be to wait until `Foo` is fully resolved, but this will never happen because `Foo` cannot be resolved until `Foo` is resolved.

Let's observe the next case, 
```
typedef Foo: (Foo)
```

This is also invalid for similar reasons as the first case.
The type checker must first resolve `(Foo)`, which requires resolving `Foo`, which is not yet resolved.

Notice how we have to resolve the right side first.
We have to first make sure the type on the right side is fully resolved before we can establish the binding between `Foo` and the right side.
If the right side is not fully resolved, then we cannot establish the binding, and we cannot resolve `Foo`.

Let's observe the next case:
```
typedef Foo: (i32, Foo)
```

This case seems very similar to the previous case, and it is indeed invalid.

But there's a good reason we want to look at this case: consider the size of `Foo` if it were valid.
```
sizeof(Foo) == sizeof(i32) + sizeof(Foo)
```

It is a recursive definition of `Foo` that leads to an infinite size, which is not valid.
This reveals an important aspect of valid named types: they must have a finite size.
A lack of a finite size is perhaps the biggest indicator of an invalid named type.

Let us examine one more case:
```
typedef Foo: (@Foo)
```

Although this may seem invalid, let us consider the size of `Foo`.
`Foo` is a tuple containing a pointer to `Foo`.
The size of a pointer is always finite.
Therefore the size of the tuple is finite.
Therefore, the size of `Foo` is finite, and there is no infinite recursion in the definition of `Foo`.

Indeed, this is a valid named type.
The presense of the pointer breaks the infinite recursion and allows `Foo` to have a finite size.

## Infinite Pointer Problem

If a named type can reference itself like this:
```
typedef Foo: (@Foo)
```

Then we see no reason to disallow this:
```
typedef Foo: @Foo
```

Indeed, we would consider this a valid named type as well.
The size of `Foo` is the size of a pointer, which is finite.

But thinking about it more deeply, this type is quite strange.
The type is a pointer to itself, which is a pointer to itself, which is a pointer to itself, and so on.
Without named types, we literally cannot write a type like this; it would be infinitely long: `...@@@@@@@@type`.

This type also seemingly has no practical use.

It would not be hard for the user to create values of this type:
```
typedef Foo: @Foo
let var p: Foo
p = @p
```

We don't even need a transmute cast to make this possible.
We create `p`, which is a pointer, and we assign `p` its own address.

Explicit casts would also work as normal. When we dereference `p`, we would get a real address, which we can dereference again, giving us the same address, and so on.
```
^p -> 0x123abc
^^p -> 0x123abc
^^^p -> 0x123abc
```

The real issue arises when we *implicitly* dereference `p`.
We do implicit dereferencing when using dot access or subscript access on a pointer type.
```
let x = [0, 1, 2]
let xp = @x
let xpp = @xp
printout x[0], xp[0], xpp[0]
```
When we use a dot operator or subscript operator on a pointer type, we implicitly dereference the pointer *completely* to access the underlying value.

But we cannot do this with pointers that contain their own address as this would cause an infinite loop of dereferencing.
```
printout p[0]  // Too many implicit dereferences
```

It doesn't matter that `p` is not even an array or a struct; as long as it is a pointer, we have to dereference it until we can confirm that it is or is not an array or struct.

This is a very unique and bizarre error that can only occur with named types: "too many implicit dereferences" or "too much indirection".

Luckily, this is a very easy error to detect and report.
When we try to implicitly dereference a pointer type, we can keep track of how many times we have dereferenced it.
If we dereference a pointer type more than a certain threshold (e.g., 256), we can report an error about too much indirection and stop the program from entering an infinite loop of dereferencing.

This doesn't stop the user from creating these pointers nor dereferencing them explicitly.
In any case where implicit dereferencing would occur, the user is still free to use explicit dereferencing to access the underlying value, which would work as normal and would not cause an infinite loop.

## Type Resolution Strategy

Type definitions are declaration-space-only statements, meaning they can only be defined in declaration space.
The parser enforces this.
Therefore, when we reach the end of declaration space, we can be sure that all type definitions have been processed.
Therefore, if there were any type definitions that were not fully resolved during declaration space, we can attempt to resolve them at the end of declaration space when we know that all types have been defined.

If a type definition cannot be resolved at the end of declaration space, then we can be confident that it is invalid and report an error to the user.

Our strategy is simple:
1. During our first pass through declaration space, we attempt to resolve type definitions as we encounter them.
   1. If we encounter a named type annotation that cannot be fully resolved, we create a placeholder node to represent the unresolved type.
   2. We store the placeholder node in a list of unresolved type definitions to revisit later.
   3. We continue processing declaration space as if the unresolved type were fully resolved, allowing us to handle cases where the named type is defined in terms of itself or in terms of another named type that is defined later in declaration space.
2. Once we reach the end of declaration space, we revisit all unresolved type definitions and attempt to resolve them now that we know all type definitions have been processed.
   1. If any unresolved type definition cannot be resolved at this point, we report an error to the user indicating that the type definition is invalid.
   2. If an unresolved type definition can be resolved, we update the named type nodes that reference the placeholder node to reference the actual node for the type definition, ensuring that all named types are correctly resolved to their definitions.
   3. We then check if the resolved type definitions are valid (e.g., they have a finite size) and report any errors if they are not valid.
3. After all type definitions have been resolved and validated, we can proceed to check the rest of the code in execution space, knowing that all named types have been correctly resolved to their definitions.
