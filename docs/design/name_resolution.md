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

## LLVM Identifiers

According to the LLVM docs:

> Named values are represented as a string of characters with their prefix. For example, %foo, @DivisionByZero, %a.really.long.identifier. The actual regular expression used is ‘[%@][-a-zA-Z\$._][-a-zA-Z$._0-9]*’. Identifiers that require other characters in their names can be surrounded with quotes. Special characters may be escaped using "\xx" where xx is the ASCII code for the character in hexadecimal. In this way, any character can be used in a name value, even quotes themselves. The "\01" prefix can be used on global values to suppress mangling.

As mentioned previously, unique symbols may look like this.
```
::s::MyType::my_function::1::2::x
```

Unique symbols are used internally by the code generator to prevent naming collisions and ensure that generated code is unique and unambiguous. There is no need to transform the string into an "LLVM safe" name since any character is allowed in an LLVM identifier.

## Accessing Foreign Variables and Functions

In LLVM, code is organized into modules. Functions and variables with *internal linkage* are only accessible within the module they are defined in. Functions and variables with *external linkage* can be accessed from other modules.

To access an external variable or function, the current module must declare the variable or function with the same name and type as the external definition.
The linker then resolves the reference to the external definition during the linking phase.

Nico is planned to support "exporting" and "importing", which handles this much of this process automatically for Nico code in the same project.
Imported code is safer to use and the linking is done seamlessly.

However, we also plan to support *foreign function interfaces* (FFI), which allows Nico code to call functions and access variables defined in external libraries, such as C libraries.
Nico users use an external declaration block where they can declare the external variables and functions they want to access (e.g., `printf` or `malloc` from the C standard library).

For simplicity, we will use the term "external" to refer to foreign code defined outside of Nico code and accessed via FFI.

FFI complicates the name resolution process, because the identifier used for the declaration must resolve to a symbol that matches the external definition exactly.
```
func my_func() {}                        // Symbol: "::my_func$1"

namespace s:
  func my_other_func(x: i32) {}          // Symbol: "::s::my_other_func$2"

extern:
  func printf(fmt: anyptr, ...) -> i32   // Symbol: "printf"
```

Let us start tackling this problem by reviewing name mangling. **Name mangling** is the process of transforming a function or variable name into a unique string that encodes additional information about the function or variable, such as its namespace, parameter types, and return type.
The main purpose of name mangling is to allow symbol table entries to be unique from other entries declared with the same identifiers.
This additional makes code generation easier, because the code generator can simply use the mangled name as the symbol name in the generated code.

Because the code generator uses the mangled name as the symbol name, it makes it harder for external code to call Nico functions (the external code would have to understand the mangling scheme to know the correct symbol name to call).
But if the symbol were *unmangled*, then external code could easily call the Nico function by using the unmangled name.

This introduces a new idea: for any entry in the symbol tree, we can choose to either have a *mangled symbol* or an *unmangled symbol*.
For normal Nico code, we use mangled symbols by default.
But we can "opt-in" to unmangled symbols if we want.
And for external declarations, we always use unmangled symbols.

## Unmangled Symbols

An **unmangled symbol** or **custom symbol** is a symbol that is not mangled by the compiler.
Unmangled symbols are mainly used for linking with external code, which may expect certain symbol names to be present in the compiled code.
Because they are not restricted to the mangling scheme, they can potentially conflict with other symbols in the symbol tree.

Unmangled symbols must still be unique within the symbol tree. Thus, if a user attempts to introduce a symbol that collides with an existing unmangled symbol, the type checker must raise an error, regardless of where that existing symbol was declared.
For example, consider this code:
```
namespace x:
  func foo() -> i32 {}   // Symbol: "::x::foo$1"
  extern:
    func bar() -> i32    // Symbol: "bar"
namespace y:
  func foo() -> i32 {}   // Symbol: "::y::foo$1"
  extern:
    func bar() -> i32    // Symbol: "bar"  <-- Error: symbol collision
```

As shown in the above example, mangled symbols do not conflict if they are declared in different namespaces.
However, unmangled symbols can still conflict with each other.
This is a **symbol collision**.

- A **name collision** occurs when a symbol tree entry's fully-qualified name matches that of another entry.
- A **symbol collision** occurs when a symbol tree entry's unique symbol matches that of another entry. This can occur even if the fully-qualified names are different.

Now, users not only have to worry about name collisions, but also symbol collisions.
This is the trade-off we make to allow the use of unmangled symbols.

Unmanged symbols can still be referenced using qualified names.
```
namespace c_funcs:
  extern:
    func printf(fmt: str, ...) -> i32   // Symbol: "printf"

let var result = c_funcs::printf("Hello, world!\n")
```

When we declare an external variable or function, the entry in the symbol tree will be placed within the current global scope.
This is possible because the name lookup algorithm does not require the symbol to perform a search.
In the above example, the name `c_funcs::printf` is resolved to the symbol `printf`, and not `::c_funcs::printf$1`. The symbol is actually shorter than the name used to reference it.

By allowing unmangled symbols to be introduced in namespaces, we allow the user to organize their external declarations better to avoid name collisions.

For example, consider this code:
```
namespace c_funcs:
  extern:
    func malloc(size: u64) -> anyptr     // 1

  func malloc(size: i32) -> anyptr:      // 2
    return c_funcs::malloc(size as u64) 

func malloc(size: u64) -> anyptr:        // 3
    return c_funcs::malloc(size)

malloc(1024_u64) // Calls function 3
c_funcs::malloc(512_i32) // Calls function 2
c_funcs::malloc(256_u64) // Calls function 1
```

Here, we have three different `malloc` functions:
1. The external declaration for the C `malloc` function, which has the unmangled symbol `malloc`.
2. A Nico wrapper function in the `c_funcs` namespace that overloads the first `malloc` function to accept an `i32` size and has the symbol `::c_funcs::malloc$1`.
3. A top-level Nico wrapper function that accepts an `u64` size and has the symbol `::malloc$1`.

The use of namespaces combined with external declaration blocks allows all three functions to coexist without symbol collisions or name collisions.

## Reserved Words and Conflict Checking

An identifier cannot be used if it matches a keyword or is otherwise reserved.
The lexer already handles the former case by tokenizing keywords differently from identifiers.

There are three categories of reserved words:
- **Reserved identifiers**: Words that cannot be used as identifiers. 
  - The strictest category.
  - These words cannot be used at all.
  - Enforced by the lexer.
  - Generally used for words that may become keywords in the future.
- **Reserved names**: Words that cannot be used as names. 
  - Less strict than reserved identifiers.
  - Reserved names can be referenced, but cannot be used to introduce new bindings.
  - Enforced by the type checker.
  - Used for words that reference some existing construct in the language, such as built-in types.
- **Reserved symbols**: Words that cannot be used as symbols.
  - Least strict category.
  - Reserved symbols cannot be referenced, but new bindings generally will not conflict with them.
  - Due to the name mangling scheme, only unmangled symbols can conflict with reserved symbols.
  - Enforced by the type checker.
  - Used for words that are needed for code generation, like "main" and "$script".

A *name* is, in essense, a position in the symbol tree.
You could say that an entry is bound to a name from the moment the entry is inserted into the symbol tree.
The symbol is then assigned to the entry afterward.

In other words:
- A symbol tree entry first gets its name, then its symbol.

This also means that:
- A new entry is checked for *reserved name* conflicts first,
- Then checked for *reserved symbol* conflicts second.

This should make intuitive sense: you can insert a node in the tree and have a symbol conflict, but if there is a name conflict, you cannot insert the node at all.

*Reserved names*, like "i32" or "f64", are designed to be referenced.
Therefore, they must have a real entry somewhere in the symbol tree.
This entry must be checked first before any new bindings are introduced.

We can accomplish this in a few ways:
- Create a mapping of reserved names to their entries using an internal hash map.
  - An effective way to look up reserved names quickly.
- Pre-insert reserved name entries into the global scope of the symbol tree.
  - Keeps the tree symbol, but makes it difficult to distinguish between user-defined entries and reserved name entries.
- Create a separate reserved names tree that is checked before the main symbol tree.
  - Allows us to reuse our tree lookup algorithm, but adds an extra tree to maintain.

We will choose the third option, as it allows us to keep the symbol tree structure and lookup algorithm consistent.

*Reserved symbols*, like "main" or "$script", are not meant to be referenced directly.
After all, users cannot use symbols to reference variables or functions; only names.
Therefore, they do not need real entries in the symbol tree.
When reporting symbol collisions, if the conflicting symbol is a reserved symbol, we can simply report the conflict without needing to point to an entry in the tree.

We can accomplish this by:
- Creating a set of reserved symbols separate from the symbols map.
  - This is analogous to our tree, where we have a main tree for user declarations and a separate tree for reserved names.
