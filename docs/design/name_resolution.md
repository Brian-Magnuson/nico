# Name Resolution

The following is an explanation of the name resolution process in the Nico programming language. This process is crucial for determining the meaning of identifiers in the code, such as variable names, function names, and type names.

> [!WARNING]
> This document is a work in progress and is subject to change.

Currently, many of the features associated with name resolution are not yet allowed in the language (e.g. generics, inner classes, etc.). However, the design of these features is necessary to avoid refactoring issues later on.

## Introduction

The lexer and parser, which make up the first two phases of the Nico compiler, have a clear purpose: to tokenize the source code and to build an abstract syntax tree (AST) from the tokens. These facts are undisputed.

The abstract syntax tree (AST) is meant to be a representation of the source code that is easy to analyze and manipulate. When the AST is built, it contains nodes that represent the structure of the code, but it does not yet contain any information about the meaning of the identifiers used in the code. Later phases of the compiler are designed to fill in this information, allowing the compiler to understand what each identifier refers to.

For example, here is a sample line of code:
```
let var x: i32 = 42;
```

An AST node for this line might look like this:
```
Stmt::Let {
    is_var: true,
    identifier: Token {
        kind: Tok::Identifier,
        lexeme: "x",
    }
    annotation?: Annotation::Base {
        identifier: Ident {
            parts: [ Ident::Part {
                token: Token {
                kind: Tok::Identifier,
                lexeme: "i32",
            }, args: []],
        },
    }
    value: Expr::Literal {
        token: Token {
            kind: Tok::Int,
            lexeme: "42",
        }
    },
}
```

As explained in the following sections, the variables, scopes, and types in the AST may have multiple names associated with them, and thus need to be resolved carefully and efficiently. An efficient design of this name resolution process should follow these priciples:
- All names can be matched to a single, unique symbol.
- Matching a name to a unique symbol should only occur once per name in the entire pipeline.
- Avoid multiple sources of truth when determining the structure of a type.

## Identifier vs Name vs Symbol

> [!CAUTION]
> The design goals outlined in this section are *inconsistent* with the current implementation of the compiler. The compiler should be *updated* based on these goals.

The terms "identifier", "name", and "symbol" are often used interchangeably in the context of programming languages and programming language concepts. 
However, some programming languages make a distinction between these terms. For example, in Java, `x` is an identifier while `com.example.myproject.x` is a fully qualified name.
The Nico programming language, like Java, has different name-like constructs used to refer to variables, types, and functions.
We choose to distinguish between the terms "identifier", "name", and "symbol" to avoid confusion when discussing these concepts.

An **identifier** (or **ident** for short) is a single-part string used to introduce a binding in the source code, i.e., a declaration. Identifiers are not globally unique. 
They must also obey the naming rules of the language, which are the same as those of the C language (matching the regular expression `[A-Za-z_][A-Za-z0-9_]*`).
- Example: `x`

A **name** or **qualified name** is a string consisting of one or more parts separated by `::` and used to refer to something in another scope. 
Names are not necessarily globally unique, but contain enough information to point to a single variable/type/function.
A variable/type/function may also have multiple names depending on where it is being referenced.
- Example: `s::t::x`

A **symbol** or **unique symbol** is a globally unique string consisting of one or more parts and used internally by the compiler to refer to a variable/type/function.
The association is one-to-one, i.e., each variable/type/function has exactly one unique symbol, and each unique symbol refers to exactly one variable/type/function.
Users of the language will typically not use unique symbols directly.
- Example: `::s::t::1::2::y`

Some programming languages use the term "mangled name" to refer to what we call a symbol. To avoid confusion, we should avoid using the term "mangled name".

## Identifiers

When a variable or type name is declared, the name can only consist of a single part.
```
let var x: i32 = 42 // OK
let var s::x: i32 = 42 // Bad; `::` cannot be used in a declaration
```

Internally, every variable or type is associated with a unique symbol, which is a long string representing the identifier and all of the surrounding scopes.
An example of a unique symbol might look like this:
```
::s::MyType::my_function::1::2::x
```

This unique symbol is useful during code generation to keep names unambiguous. Obviously, using this name in the source code would be highly impractical.

When referencing a variable or function, the name may consist of multiple parts.
```
let var y = s::x // OK
let var z = s::t::y // OK
let var a = s::MyType::my_function() // OK
```

When referencing a named type, the name may consist of multiple parts.
```
let var y: s::MyType = new s::MyType { x: 42 } // OK
```

A variable or type name may be referred to by its short name, if it is in scope. The name may also include the surrounding namespaces, if these namespaces are also in scope. Here is an example:
```
namespace s:
    namespace t:
        let var x = 42
        // Valid in this scope:
        // x
        // t::x
        // s::t::x

    // Valid in this scope:
    // t::x
    // s::t::x

// Valid in this scope:
// s::t::x
```

Whatever name resolution algorithm is used, it must be able to resolve all of these names to the same unique identifier, which is `::s::t::x` in this case.

## Type Annotations

Because type annotations may use multiple parts in their identifiers, it is possible that two variables will have two different-looking type annotations that refer to the same type. For example:
```
let var x: s::MyType = new s::MyType { x: 42 }
let var y: MyType = new MyType { x: 42 }
```
This also means that named types cannot be compared by their names alone.

A type annotation cannot purely be a name; else it would not be possible to use type modifiers such as `&` or `[]` in the type annotation.
A type annotation cannot also the same type objects used by the type checker. Type objects should be final; they should not need resolving.

Since a type annotation cannot be an identifier or a type object, it must be its own class, which can store the identifier and any modifiers. The annotation class must then map to a type object, which can then map to an LLVM type.

## LLVM Type Mapping

Nico is designed such that its type system closely maps to the LLVM type system. For some basic types like `i32` and `bool`, this is straightforward. For other types, the mapping is slightly more complicated:

- Sized arrays map to LLVM arrays, not to be confused with LLVM vectors.
- Unsized arrays map to LLVM pointers. The memory for unsized arrays can be managed similarly to dynamically allocated memory in C/C++.
- Pointers map to LLVM opaque pointers. Notably, with opaque pointers, the type information is not stored in the pointer itself.

Note that the mapping is not necessarily bidirectional. Because an LLVM pointer does not carry type information, it is not possible to map an LLVM pointer back to a Nico type object.

The key issue involves structs. Many programming languages use "nominal typing", meaning that structs can be identified by their names. When a variable has a struct type, storing the name only is sufficient to identify the type.

LLVM supports both named structs and anonymous structs. Named structs are defined with a name, while anonymous structs are defined without a name. Nico will support something similar, but will have a large focus on named structs.

To resolve a type annotation to a type object, the type checker must be able to use the name in the annotation to look up the type in the symbol table/tree. If the type's unique symbol is found, a type object is created from the unique symbol for the type.

Under the rules of nominal typing, the unique symbol should be sufficient to determine if two types are the same. However, the type checker must also be able to quickly reference struct members. Thus, the type object for a struct must also contain a reference to the struct definition, which is stored in the symbol table/tree.

## Name Resolution

The Nico compiler must meet the following requirements for name resolution:
- Parser
  - When the parser encounters a variable declaration, it must be able to store the variable identifier, type annotations, initializers, and other relevant information in the AST.
  - When the parser encounters a type annotation, the annotation is stored as an annotation object in the AST node.
  - When the parser encounters a variable reference, it must be able to store all parts of the name in the AST.
- Type checker
  - When the type checker encounters a variable declaration, it must be able to use the AST node information to create a symbol table/tree entry for the variable, generating a unique string for the variable and creating a type object from the annotation.
  - When the type checker encounters a struct definition, it must be able to create a symbol table/tree entry for the struct, generating a unique string for the struct.
  - When the type checker encounters a variable reference, it must be able to look up the variable in the symbol table/tree and retrieve the unique symbol and type information.
- Code generator
  - Type objects must be directly convertible to LLVM types, which are used in the code generation phase.
  - Special names must be in a format that makes it impossible for them to collide with other names.

## Internal Use Symbols

According to the LLVM docs:

> Named values are represented as a string of characters with their prefix. For example, %foo, @DivisionByZero, %a.really.long.identifier. The actual regular expression used is ‘[%@][-a-zA-Z\$._][-a-zA-Z$._0-9]*’. Identifiers that require other characters in their names can be surrounded with quotes. Special characters may be escaped using "\xx" where xx is the ASCII code for the character in hexadecimal. In this way, any character can be used in a name value, even quotes themselves. The "\01" prefix can be used on global values to suppress mangling.

As mentioned previously, unique symbols may look like this.
```
::s::MyType::my_function::1::2::x
```

Unique symbols are used internally by the code generator to prevent naming collisions and ensure that generated code is unique and unambiguous. There is no need to transform the string into an "LLVM safe" name since any character is allowed in an LLVM identifier.

It is impossible for users to access these symbols directly, largely because they all start with `::`, which is not a valid starting token for user-defined identifiers.
Additionally, any `:` is not allowed in an identifier.
It is also impossible for users to write two identifiers that have the same unique symbol; any attempt to do so would be caught by the type checker before the code is generated.

The construction of unique symbols also mean that certain names that are used internally cannot be accessed by users. For example, if the user attempts to write a function named `main` at the top level, the unique symbol generated would be `::main`, which will not collide with the internal `main` function.

For Nico, there is a need for these **internal use symbols**, symbols that are used by the compiler and are named in such a way that they cannot be accessed by users.

Here is a list of the the current internal use symbols:
- `main`
- `script`
- `$retval`
- `$yieldval`
