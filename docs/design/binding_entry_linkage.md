# Binding Entry Linkage

This document discusses how linkage works with global variables and functions in the Nico programming language.
Most binding entries in Nico are managed using the same `BindingEntry` structure, and, depending on the linkage and "globalness" of the entry, must have its LLVM allocation retrieved correctly.

> [!WARNING]
> This document is a work in progress and is subject to change.

## Bindings

In the Nico compiler, the `Binding` structure is used to represent the binding of a symbol to its type and other metadata. 
It is used to represent the following:
- Execution-space variables (also called local variables)
- Function parameters
- Declaration-space variables (also called global variables)
- Functions

We use the same `Binding` structure for these constructs because they all have the essence of a "variable":
- They all have a name and symbol.
- They can be referenced in expressions.
- They all have a type.
- They all have a value (even if it's just a function pointer for functions).

Additionally, the visitor pattern allows the compiler components, like the type checkers and code generator, to handle their individual differences in a clean way.
So we can afford to reuse the same `Binding` structure for all of these constructs without losing clarity or correctness.

## Binding Entries

A name reference AST node does not have a `Binding`. 
One cannot look at a name reference to determine its type, mutability, or other properties.
A name reference *references* a declaration that has a `Binding`.

The type checker requires a construct shared between a name reference and the declaration that it references.
This allows the type checker to link the name reference to that declaration.

For this, we use a `BindingEntry` node, which is a wrapper around a `Binding` that also contains the symbol and source location of the declaration.
The node is stored in the symbol tree and is referenced by both the declaration and any name references to that declaration.
When the type checker resolves a name reference, the goal is to find the corresponding `BindingEntry` node.

## LLVM Allocations

There are two ways in LLVM to allocate space for a binding:
- Global variables
- Alloca instructions

Global variables are allocated at the module level and are accessible throughout the entire program.
Alloca instructions are allocated on the stack and are only accessible within the function they are defined in.

Here is an example of how each is declared, initialized, and loaded:
```cpp
// Global variable
new llvm::GlobalVariable(
    *ir_module,
    llvm_type,
    is_constant,
    linkage_type,
    constant_initializer,
    "global_var"
);
ir_module->getGlobalVariable("global_var", allow_internal)
builder->CreateLoad(llvm_type, global_var);

// Alloca instruction
auto alloca_instr =
builder->CreateAlloca(
    llvm_type,
    nullptr, // array size (for arrays)
    "local_var"
);
builder->CreateStore(initializer, alloca_instr);
builder->CreateLoad(llvm_type, alloca_instr);
```

Let us examine how these methods are different:
- With global variables,
  - The variable may or may not be constant
  - The initializer must be an LLVM constant value
  - The variable can have internal or external linkage
  - The address is stored in the module's global variable table
  - The variable can be retrieved by name from the module
- With alloca instructions,
  - The address is stored in a variable (the `alloca_instr` pointer)
  - Linkage is not relevant, as the variable is only accessible within the function
  - The initializer is set using a separate `store` instruction
  - An LLVM constant value is not required; it can be any value computed at runtime.
  - The variable cannot be retrieved by name from the module; the address must be stored, then loaded from that stored address when needed.

Obviously, alloca instructions cannot be used for global variables since they are only accessible within the function they are defined in.
It might be possible to use global variables for local variables. There's a few mechanisms in place that make this possible:
- The type checker ensures every binding has a unique symbol.
- The type checker also ensures that every name reference references a binding that it can access.

But this is still a bad idea for a few reasons:
- Global variables live for the entire duration of the program, so using them for local variables would lead to memory bloat and potential memory leaks.
- Global variables are not thread-safe, so using them for local variables would lead to potential race conditions.
- Global variables are not optimized as well as local variables, so using them for local variables would lead to worse performance.

The ideal move is to use each method for its intended purpose:
- Use global variables for declaration-space variables (global variables) and functions.
- Use alloca instructions for execution-space variables (local variables) and function parameters.

Using both isn't entirely bad; there are some similarities between both methods that can be leveraged to make the implementation cleaner.
Mainly, both, in essence, use a memory address that can be loaded from and stored to.

For example, with an assignment to a name reference, we can apply the same store instruction, regardless of whether the name reference is to a global variable or a local variable.
And when we have to load a name reference, we can apply the same load instruction.

```cpp
// Getting the memory address
llvm::Value* address = ???

// Storing to the address
builder->CreateStore(value, address);

// Loading from the address
builder->CreateLoad(llvm_type, address);
```

If we can design a common interface for retrieving the memory address of a binding entry, then we can use the same code for storing to and loading from that address, regardless of whether it's a global variable or a local variable.

The following problems must be considered when designing this interface:
- How do we store functions, which are not variables?
- If the binding is to be stored as a global variable, how do we determine its linkage?
- How do we handle initializers?

## Storing LLVM Functions

Functions are not variables; they cannot be loaded from or stored to, like variables can.
However, we do want to be able to treat them like variables.
We want to be able to reference them in expressions, pass them as arguments, and even return them from other functions.

The basic way to implement this is to create global variables that store the function pointers of the functions.
This way, we can treat functions like variables, while still being able to call them as functions.
This also allows us to leverage the same interface for retrieving the memory address of a binding entry, regardless of whether it's a function or a variable.

Global variables need to be looked up by name, so the name cannot conflict with the function name.
For this, we keep using the same symbol for the function, and append a suffix to it for the global variable that stores the function pointer.

For example, if we have a function named `foo`, we can create a global variable named `foo$var` that stores the function pointer of `foo`.

## Linkage

Global variables have an additional property called linkage that must be considered.

Variables with **internal linkage** are only accessible within the module they are defined in.
Variables with **external linkage** are accessible from other modules.

There's a few reasons to give a variable external linkage, which we'll discuss shortly. 
Importantly, we should use *both* internal and external linkage carefully.
Giving external linkage to a variable that doesn't need it can lead to potential name clashes and security vulnerabilities.
Giving internal linkage to a variable that needs external linkage can lead to linker errors and other issues.

Simply put, we should use internal linkage unless we have a reason to use external linkage.

Currently, there are two reasons to give a variable external linkage:
- The binding is introduced from within an extern block.
- The compiler is in REPL mode.

The first reason should be obvious; extern blocks are for declaring symbols that are defined in other modules, so they need to have external linkage.

The second reason has to do with how the REPL works.

In a REPL, the user enters code in a loop, and the compiler compiles and executes that code on the fly.
Internally, each iteration of the loop is treated as a separate module, and the variables defined in one iteration need to be accessible in subsequent iterations.
If we give these variables internal linkage, they would not be accessible in subsequent iterations, which would break the REPL.

There is also one more reason to give a variable external linkage, though this is actually enforced by the LLVM verifier: If a global variable or function is declared but not defined in the current module, it must have external linkage.

In LLVM, global variables that do not have a linkage specified default to external linkage.
```llvm
@"::counter_1" = internal global i32 42 ; internal linkage
@"::counter_2" = global i32 42 ; external linkage (default)
```

## Initializers

An initializer is a value that is assigned to a variable at the time of its declaration.
For example, in the following code, `x` is initialized to `5`:
```nico
let x: i32 = 5
```

### Initializing Global Variables

Local variables can be initialized with any value, including values computed at runtime.
A simple store instruction can be used to set the value of a local variable after it has been allocated with an alloca instruction.

We *could* use the same approach for global variables, but this is slightly more complicated since declaration-space variables in Nico can be referenced *before* they are declared.
For example, in the following code, `x` is referenced before it is declared:
```nico
func my_func() -> i32 {
    return x
}

static x = 5
```

To allow this feature, we have a strict requirement that the initializer of a global variable must be an LLVM constant value.

This will require a mechanism in the type checker to confirm that the initializer of a global variable is an LLVM constant value, and to compute that constant value at compile time.

### Initializing Externally-Linked Variables

Initializers are important because to give a variable an initial value is to *define* that variable.

There are two sides to externally linked variables: the declaration and the definition.

An externally linked variable can be declared many times, but it should only be defined once.

Let us observe the following code:
```
extern foo:
    static x: i32 = 5  // Error: Extern namespaces cannot contain definitions
    static var y: i32
    static z: i32

namespace bar:
    static x: i32 = 5
    static var y: i32
    static z: i32      // Error: Immutable variables must have an initializer
```

Here, we have 6 variables.
- `foo::x` is a variable in an extern namespace with a definition. We actually do not allow extern namespaces to contain definitions, so this is an error.
- `foo::y` is a variable in an extern namespace without a definition. And we shouldn't give it a definition, since it's meant to be defined in another module. It is only a *declaration*.
- `foo::z` is an immutable variable in an extern namespace without a definition. Despite being immutable, this is okay since the variable is defined elsewhere. It is only a *declaration*.
- `bar::x` is a variable in a normal namespace with a definition. It is a *definition*.
- `bar::y` is a variable in a normal namespace without an explicit definition. We allow the absense of a definition for mutable variables. If the user tries to use them anyway, they get the default value for the type. So even without an explcit definition, this is still a *definition*.
- `bar::z` is an immutable variable in a normal namespace without a definition. This is an error since immutable variables must have an initializer, and thus a definition.

From this, we can see that the rules for initializing externally linked variables are as follows:
- Global variables introduced from within an extern namespace *should not* have an initializer.
- Global variables introduced from outside of an extern namespace *should* have an initializer.

It's as simple as that.

Next, let us observe the following code from the REPL:
```
>> static x = 5
>> x
5
```

On the second line, we reference `x`. 
Internally, the second line is a separate module from the first line.
This means that `x` is actually declared twice here: once in the first module where it is defined, and once in the second module where it is only declared.

For the REPL to work correctly, we must ensure `x` is initialized only once.
There is a trick we can do to achieve this:

1. When we attempt to get the LLVM allocation for a binding entry, we check if it we can lookup the global variable by name in the module.
2. If we can find the global variable, then we return that global variable as the LLVM allocation for the binding entry, and we are done.
3. If we cannot find the global variable, then we create a new global variable *uninitialized*.
4. If this is called from the visit function for the declaration, then we can set the initializer of the global variable to the constant value of the initializer expression.
5. If this is called from the visit function for a name reference, then we can leave the global variable uninitialized, since it should have already been initialized by the declaration.
