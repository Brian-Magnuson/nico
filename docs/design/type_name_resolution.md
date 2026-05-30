# Type Name Resolution

The following document discusses type name resolution in the Nico programming language, including the requirements, challenges, and proposed solutions.
Type name resolution refers to the process of resolving named types to their actual definitions.

> [!WARNING]
> This document is a work in progress and is subject to change.

> [!CAUTION]
> This document has multiple inconsistencies with the current implementation of the compiler and should be updated.

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
i32 => Node::PrimitiveType(i32) -> Type::Int
MyType => Node::TypeDef(MyType) -> Type::Int
```

On the left, we have the type annotations that the user writes in their code.
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
A => Node::TypeDef(A) -> Type::Int
B => Node::TypeDef(B) -> Type::Named(A) -> Node::TypeDef(A) -> Type::Int
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

### Placeholder Type Approach

In this approach, we create a "temporary type" that represents an unresolved type in the type system. 
This temporary type would act as a placeholder until the actual type can be resolved, at which point it would be replaced with the correct type.
It would have to store the following information:
- The type annotation that was attempted to be resolved
- The scope in which the resolution was attempted

Additionally, we would need to track every place where a temporary type is used so that, when the actual type is resolved, all references to the temporary type can be updated to point to the correct type.

We can try drawing the relationships with the above example using temporary types:
```
A => Node::TypeDef(A) -> Type::Unresolved(B, scope)
B => Node::TypeDef(B) -> Type::Int
C => Node::TypeDef(C) -> Type::Named(A) -> Node::TypeDef(A) -> Type::Unresolved(B, scope)
```

Notice how we can use `Type::Named(A)` even though the actual type for `A` has not yet been resolved.
The temporary unresolved type allows us to create the node for A in the symbol tree even before the actual type for A is known, ensuring that the type system can continue to function while waiting for the missing type to be defined.

Next, let's consider this case:
```
typedef D: (A)
```

In this example, the type `A` is hidden behind a tuple type.
Let's examine how the type name resolution would work in this case:
```
D => Type::Tuple((A)) -> Type::Named(A) -> Node::TypeDef(A) -> Type::Unresolved(B, scope)
```

Here, we see how we can still use `Type::Named(A)` even though the actual type for `A` has not yet been resolved. The temporary unresolved type allows the type checker to continue processing the tuple type `D` without being blocked by the unresolved reference to `A`.


Now, there is another case to consider:
```
typedef E: (F)
```

In this example, not only is the type `F` unknown, but it is also hidden behind a tuple type, making the resolution even more complex.
Let's try diagramming the resolution process for this case:
```
E => Type::Tuple((F)) -> Type::Unresolved(F, scope)
```

In the previous examples, the unresolved type was preceeded by a `Node` object.
But this time, it is a `Type` object storing the unresolved type directly.
This means that we cannot simply use `Node` objects to track our unresolved types.

Another option would be to make the whole tuple unresolved until the type `F` can be resolved, at which point the entire tuple would be replaced with the correct type:
```
E => Type::Unresolved((F), scope)
```

This could work as well.
We get to use the `Type::Named(E)` while we wait for the unresolved type to be resolved.
It also works well with our current type annotation system, which returns an empty `std::any()` when it

This approach has a severe drawback:
Type objects are stored by all kinds of constructs in our compiler, including expressions, statements, bindings, nodes, etc.
And to make tracking of unresolved types possible, we would need to handle every case where a type object is stored and check if it is an unresolved type, which would be a huge amount of work and would make our code very messy and error-prone.

But we do have another approach that may work.


### Placeholder Node Approach

It may seem as though creating a placeholder node would be impossible since nodes require a place in the symbol tree, meaning they must have a scope.
And we cannot tell from a name alone what scope the node would belong to.

But this isn't a problem as long as we don't *use* the node as a real node.

It would have no name, no symbol, no parent, and no children.
Attempting to call any of the node's normal functions would result in an error.

Much like our placeholder type approach, we would be tracking:
- The type annotation that was attempted to be resolved
- The scope in which the resolution was attempted

And we would track one more item:
- A back pointer to the type object that references this node, so that, when the node is resolved, we can update the type object to point to the correct node.

```
A => Node::TypeDef(A) -> Type::Named(B) -> Node::Unresolved(B, scope)
```

Keep in mind that, two bindings having the same type does not necessarily mean the type pointers point to the same type object instance in memory.
Creating a `Type::Int(32)` in two different places may result in two different type objects, but we consider them to be the same type since they have the same properties.
The same can be held for named types: We can create two `Type::Named(B)` objects in two different places.
In order to make sure that they can be compared for equality, we just need to make sure that they both point to the same symbol node.

```
G => Node::TypeDef(G) -> Type::Named(B) -> Node::Unresolved(B, scope)
```
In the above example, we create a second `Type::Named(B)`.
This type object instance will be different from the one we create in `A`, but that's okay.
Both will point to their own `Node::Unresolved(B, scope)` until we reach B's declaration.
Then, if these two `Type::Named(B)` instances are indeed the same, then their unresolved nodes will be corrected to point to the same resolved node.

Also, if you think about it, it could be that two `Type::Named(B)` instances are not the same.
This can happen if `B` is declared in two non-overlapping scopes.
Our system tracks both `Node::Unresolved(B, scope1)` and `Node::Unresolved(B, scope2)`, and it will resolve them to different nodes if they are different.

To enable this solution, we need to avoid accessing the node field until the end of declaration space.
In face, we should avoid any case where we attempt to access a type's properties.

This means:
- No checking the size of types
- No checking the compatibility of types
- No checking the fields of types

These checks can be deferred until the next stage of the compiler, which runs after declaration space has been processed.

This also means we cannot check *expressions* in declaration space since expression checking involves checking the types of expressions, which may involve checking the properties of types.
But declaration space is not meant for expression checking anyway, so this is not a problem.

This approach is more effective than the placeholder type approach since it wouldn't require double indirection of types to track unresolved types.
We only need to track unresolved nodes, which can be used to update the types that reference them.

## Type Resolution Problems

Here we discuss some of the problems that arise with type name resolution, and how to approach them.

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
types::instance_of<Type::Tuple>(expr->type);
auto tuple_type = types::as<Type::Tuple>(expr->type);
types::are_directly_compatible(expr->type, other->type);
```

These functions will take the place of `std::dynamic_pointer_cast` and `Type::operator==` and will check for direct compatibility instead of type equivalence.
The reason we use global functions is because C++ makes it difficult to use template member functions with inheritance.
And we need the Type subclasses fully defined to properly implement these functions, so we cannot define them as member functions of the Type base class.


### Transitive Type Compatibility Problem

This problem is similar to the above problem, but with a small twist.
Consider the following code:
```
typedef CoolInt: i32
typedef EpicInt: CoolInt
```

Given the above code, is `EpicInt` directly compatible with `i32`?

The answer should be yes because `EpicInt` should behave in the same way as `CoolInt`, which behaves in the same way as `i32`.

This introduces the idea that direct compatibility should be transitive.
If `A` is directly compatible with `B`, and `B` is directly compatible with `C`, then `A` should be directly compatible with `C`.

We can draw out this relationship like so:
```
EpicInt -> CoolInt -> i32

Named   -> Named   -> Int
```

We can create longer and longer chains of type aliases as long as they eventually resolve to a non-named type.
```
Named1 -> Named2 -> Named3 -> ... -> Int
```

There is always a non-named type at the end of the chain, and it is essential that these named types all resolve to the same non-named type for direct compatibility to hold.

These examples reveal an important aspect of these chains: we only want the non-named type at the end of the chain.
If this is the case, then why bother preserving `Named1 -> Named2` or `Named2 -> Named3`?

It would be more efficient to preserve only the relationship between named types and non-named types:
```
Named1 -> Int
Named2 -> Int
Named3 -> Int
...
```

By keeping only these relationships, we avoid having to navigate through long chains of named types to determine direct compatibility with a non-named type.

This does not mean that we *must* use this techique; only that we *can* use this technique to optimize our type checker, and it would behave the same as if we preserved the full chain of named types.
This fact is possibly important for cases such as pointers to named types, which may not need to be fully resolved.

### Self-Referential Problem

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


### Infinite Pointer Problem

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

### Circular Reference Problem

The circular reference problem is a generalization of the self-referential problem.
In the self-referential problem, we have a single named type that references itself.
In the circular reference problem, we have multiple named types that reference each other in a circular manner.
For example:
```
typedef Foo: Bar
typedef Bar: Foo
```

As we've seen in the self-referential problem, this is invalid because neither `Foo` nor `Bar` have a finite size.

A more valid example is this:
```
typedef Foo: (@Bar)
typedef Bar: (@Foo)
```

This is valid because both `Foo` and `Bar` have a finite size due to the presence of the pointer types.

In C++, you can create classes that reference each other in a circular manner.
When doing this, you need to use forward declarations to guarantee that the compiler knows about the existence of the other class before it is fully defined.
```cpp
class Bar;  // Forward declaration of Bar

class Foo {
    Bar* bar;  // Pointer to Bar
};

class Bar {
    Foo* foo;  // Pointer to Foo
};
```

Technically, we could write a forward declaration of `Foo` as well, even if it is not necessary in this case:
```cpp
class Foo;  // Forward declaration of Foo
class Bar;  // Forward declaration of Bar

class Foo {
    Bar* bar;  // Pointer to Bar
};
class Bar {
    Foo* foo;  // Pointer to Foo
};
```

We can use a similar idea in our algorithm for type name resolution.
When can first "collect" all named type declarations, then begin resolving their definitions.

Another approach is to create a list of unresolved types, then, when all type declarations have been processed, we can iterate through the list of unresolved types and try to resolve them.
- If we can resolve a type, we remove it from the list.
- If we cannot resolve a type, we keep it in the list and try again in the next iteration.
- If we go through an entire iteration without resolving any types, then we have a circular reference and we can report an error.

This algorithm is similar to the topological sorting algorithm used in graph theory. 
This idea will be further discussed in our solutions section.


### Multiple Definitions Problem

This problem describes a case that our solution must be able to handle.

Consider the following code:
```
typedef Foo: Bar
typedef Foo: i32
typedef Bar: bool
```

This code should be invalid because `Foo` is defined multiple times with different definitions.
But we want to make sure that our type name resolution algorithm can detect this and report an error.

When the type checker encounters the first declaration of `Foo`, it will not be able to fully resolve it because `Bar` has not been declared yet.
But we have to make sure `Foo` doesn't get declared anywhere else, even while it is still unresolved.
When the type checker encounters the second declaration of `Foo`, it should check if `Foo` has already been declared, and if it has, it should report an error about multiple definitions of `Foo`.

In other words, unresolved does not mean undeclared.
Our solution has to assume that the declaration is valid until it is proven otherwise, and it has to keep track of all declarations, even the unresolved ones, to ensure that there are no multiple definitions.


### Use Before Declaration Problem

What happens our declaration-space type checker needs to resolve a named type that has not been fully resolved yet?

For example, consider this code:
```
static x: (i32, Foo) = (42, (1, 2))
```

The expression on the right is a `(i32, (i32, i32))` tuple, but we cannot determine yet if this is directly compatible with `(i32, Foo)` because we have not yet resolved `Foo`.

This is tricky because we cannot just wait until `Foo` is resolved to check the validity of this declaration.

What if, instead, we avoid checking the expression at this time?
```
static x: (i32, Foo)
```

This is okay because we can create the named type `Foo` without fully resolving it.
Once `Foo` gets resolved, a different stage of the compiler can visit this declaration and see the fully resolved `Foo` type.

We do not need to track that `(i32, Foo)` is unresolved. Only that `Foo` is unresolved.
Once `Foo` is resolved, the tuple is guaranteed to be resolved as well.

Though for a better user experience, we may want to at least track the location of `(i32, Foo)`.
In the event that `Foo` cannot be resolved, we want to report an error at the location of the tuple type, not just at the location of `Foo` (if it even exists).


## Using Types Before They Are Fully Resolved

This concept was discussed earlier in our discussion of using types when names are not yet known.
However, we'd like to expand on this concept further.

Our type resolution system will rely on a key assumption:
When we encounter a type that has not yet been resolved, we assume that it is valid and will eventually be resolved.

First, let's consider the following code:
```
typedef Bar = @Foo
```

Question: is `@Foo` considered resolved, even if we haven't resolved `Foo` yet?

To make this easy to understand, let's imagine a middle ground between "definitely resolved" and "not resolved": "partially resolved".
This middle ground is useful to think about when working through these problems, but we'll see that, in practice, we don't need to explicitly track this middle ground in our implementation.

- `@Foo` has a finite size, but we haven't fully resolved `Foo` yet, so we can consider `@Foo` to be partially resolved.
- We can establish a named type `Foo` and assume that it will be resolved eventually. 
- When `Foo` is eventually resolved, `@Foo` will automatically become fully resolved as well.

Now at this point, one of three things can happen:
1. `Foo` is eventually resolved.
2. `Foo` is never declared.
3. `Foo` is declared, but it can never be resolved due to circular references.

If case 2 or 3 is true, then `@Foo` can never be fully resolved and is invalid.
If case 2 and 3 are false, then `Foo` must resolve at some point, and `@Foo` will become fully resolved.

In other words, `@Foo` being valid is logically equivalent to cases 2 and 3 being false.
So we just need to check for cases 2 and 3.

- Case 2 is easy enough to check for: we can keep track of all named types that we encounter. If we encounter a type that is not resolved yet, we'll save it in a list and check that it gets resolved later.
- Case 3 is a bit more tricky, but we can use a similar approach of tracking unresolved types. We explain the algorithm for this in more detail in the next section.

As we can see, once we know that `@Foo` has a finite size, we only need to check for cases 2 and 3.
There's no need to "upgrade" `@Foo` from being partially resolved to being fully resolved.
We just consider `@Foo` resolved.

This brings us to the definition of a resolved type.
A resolved type is one of the following:
- A basic type (e.g., `i32`, `f64`, `bool`).
- A type with a determinable size where we assume that all of its dependent types are resolved (e.g., `@Foo`).

This definition seems weird, being recursive and carrying an assumption about the resolution of other types.
However, it is useful to us because of how our compiler works:
If we erroneously classify a type as resolved, we will eventually find out when we try to resolve its dependent types.


## Topological Sorting Algorithm

In this section, we describe topological sort, an algorithm that can be used to solve multiple problems in type name resolution, including circular references.

A **topological sort** is a linear ordering of the vertices of a directed graph such that for every directed edge `uv` from vertex `u` to vertex `v`, `u` comes before `v` in the ordering.

Topological sort is useful in our situation since dependency relationships can be represented as a directed graph, where vertices represent different types and edges represent dependencies between types.

The algorithm fairly straightforward:
1. Let `L` be an empty list that will contain the sorted elements.
2. Let `S` be a set of all vertices with no incoming edges (i.e., vertices that do not depend on any other vertices).
3. While `S` is not empty:
   a. Remove a vertex `n` from `S`.
   b. Add `n` to the end of `L`.
   c. For each vertex `m` with an edge `e` from `n` to `m`:
      i. Remove edge `e` from the graph.
      ii. If `m` has no other incoming edges, add `m` to `S`.
4. If the graph has edges, then there is a cycle in the graph, and the algorithm cannot produce a valid topological sort. Otherwise, `L` contains a valid topological sort of the vertices.

We will tweak this algorithm to fit our specific use case of type name resolution, but the general idea remains the same: we will iterate continually through the list of unresolved types until we can no longer resolve any types. If there are still unresolved types at this point, then we have a circular reference and we can report an error. Otherwise, all types have been resolved successfully.

We also won't track the topological ordering.
Although this is a sorting algorith, we are not actually interested in the order of the types; we just want to be able to resolve them all.

### Type Name Resolution: Topological Sort

Here is our algorithm:
1. Create an empty list `unresolved_types` to keep track of all unresolved types.
2. For each named type declaration:
   1. Attempt to resolve the type immediately.
   2. If the type cannot be resolved, add it to the `unresolved_types` list.
3. Continue until all named type declarations have been visited.
4. While `unresolved_types` is not empty:
   1. Create a variable `resolved_any` and set it to `false`.
   2. For each type `T` in `unresolved_types`:
      1. Attempt to resolve `T`.
      2. If `T` is resolved successfully:
         1. Remove `T` from `unresolved_types`.
         2. Set `resolved_any` to `true`.
   3. If `resolved_any` is `false`, then there is a circular reference among the types in `unresolved_types`, and we can report an error about circular references and exit the algorithm.
5. If we exit the loop successfully, then all types have been resolved.

The key idea here is that we keep trying to resolve the unresolved types until we can no longer resolve any of them.
If we iterate through the unresolved types and cannot resolve any of them, it means that there is a circular reference among those types, and we can report an error.

Note that this algorithm involves modifying a list as we iterate through it.
This is somewhat inefficient if we keep removing elements from the middle of the list.

One idea to optimize this is to use a linked list instead of a vector for `unresolved_types`, which allows for efficient removal of elements from the middle of the list.
However, linked lists are generally less cache-friendly than vectors and involve extra memory overhead for storing pointers, so this may not be the best choice.

A better approach is to use two vectors like this:

1. Create an empty vector `pending_resolution` to keep track of all unresolved types.
2. While `pending_resolution` is not empty:
   1. For each named type declaration:
      1. Create a second vector `next_pending` to keep track of types that cannot be resolved in the current iteration.
      2. Attempt to resolve the type immediately.
      3. If the type cannot be resolved, add it to the `next_pending` vector.
   2. If `next_pending` and `pending_resolution` are the same size, then no types were resolved in this iteration, which means there is a circular reference among the types in `pending_resolution`.
   3. Otherwise, we swap `next_pending` and `pending_resolution`.
3. If we exit the loop successfully, then all types have been resolved.

Vector swapping in C++ is a constant time operation, so this approach allows us to avoid the inefficiency of removing elements from the middle of a vector while still maintaining good cache performance and low memory overhead.
