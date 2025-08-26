# Code Style

This document outlines the coding conventions and style guidelines for this project.

The goal of this document is to establish a consistent coding style that enhances readability and maintainability.
It is not a strict guideline; we recognize that exceptions may occur.
Exceptions will be considered on a case-by-case basis.

As this project was originally built for fun, many of the existing styles are a reflection of how I like to write my code.
This document represents my best attempt at formalizing those styles into a coherent set of guidelines.
If this project becomes more widely used in the future, these guidelines may be revisited and updated for a higher level of strictness.

## C++ Version

Use C++ features up to and including `C++20`. This may change in the future.

## File Naming

When creating new files, please follow these naming conventions:

- Use `snake_case` for file names (e.g., `my_file.cpp`).
- Avoid using special characters or spaces in file names.
- If the file is for a specific class, name the file after the class
  - E.g., `CodeGenerator` -> `code_generator.h`
- Use `.h` for header files and `.cpp` for implementation files.
- Implementation files should have the same base name as their corresponding header files
  - E.g., `code_generator.h` -> `code_generator.cpp`

## Include Guards

For all header files, use the following include guard pattern:

```cpp
#ifndef NICO_FILE_BASE_NAME_H
#define NICO_FILE_BASE_NAME_H

// Header file content

#endif // NICO_FILE_BASE_NAME_H
```

View existing header files for more examples.

## Names and Identifiers

For names in the `std` namespace, `llvm` namespace, or other external namespaces:
- Use the fully qualified name (e.g., `std::cout`, `std::string`, `llvm::Function`).
- Do not use `using namespace`.
- Do not use `using` for the purpose of avoiding namespace qualification.
  - E.g., do not use `using std::cout;`.

For macros and compile-time constants:
- Use `UPPER_SNAKE_CASE`
  - E.g., `PTR_INSTANCEOF`

For variable names, function names, parameter names:
- Use `lower_snake_case`
  - E.g., `my_variable`, `compute_sum()`
- It may be helpful to change your IDE settings to use different colors for class member variables and static/global variables.

For concrete types:
- Use `CamelCase` with no additional prefixes
  - E.g., `MyClass`, `YourStruct`
- Some interface-like classes such as `Expr` use this style despite not being concrete. This is mainly for base classes that are core to the compiler and have a complex inheritance hierarchy.

For interface-like types:
- Use `CamelCase` and prefix the type with `I`.
  - E.g., `ILocatable`, `INumeric`
- Use this pattern for types that are not designed to be instantiated directly.

## Error Handling and Exceptions

General rules regarding exceptions:
- Do not use exceptions, except when handling exceptions thrown by standard library functions.
  - The motivation behind this is to avoid unexpected control flow changes and to make error handling more explicit.
- Avoid rethrowing exceptions thrown by standard library functions
  - Consider converting to error objects or panicking.

Instead of using exceptions, prefer using error codes or other mechanisms to indicate failure.
For example, many functions use `std::optional` and `std::any` to represent values that may or may not be present.

For unrecoverable errors, use the `panic` function to terminate the program and provide an error message.
- Panic error messages should include the name of the function where the error occurred.
  - If the function has many overloads (such as the visit functions for the visitor pattern), include the parameter types.
  - E.g. `Parser::parse: error message here.`, `LocalChecker::visit(Stmt::Expr*): error message here.`

In addition to errors generally perceived as unrecoverable, 
errors caused by any of the following should also be treated as unrecoverable:
- A bug in the compiler, not in the inputted source code
- An unimplemented feature
- An impossible state
- Invalid input that should have been caught by earlier stages of the compiler

In any of the above cases, `panic` should be called to abort the program.

For errors in the user's source code:
- Log the error using the `Logger` singleton.
- Always try to point out the location of the error when applicable.
- Provide a clear and concise error message that explains the issue.
- For suggestions or extra guidance, use the logger to log a note.
- Avoid panicking or otherwise aborting the program immediately.
- Messages logged should use proper sentence case and end with a period.

## Code Structure

### Inner Classes

For several classes, we use a pattern where subclasses are also inner classes of the deepest base class.
This is the case for these classes:
- `Stmt`
- `Expr`
- `Annotation`
- `Type`
- `Node`
- `Block`

This pattern is used to group related classes together and to make it clear that these classes are part of a larger hierarchy.
- E.g., we use `Expr::Assign` instead of just `Assign`.

### Pointers and Memory Management

Regarding memory management:
- Avoid using `new`, `delete`, `malloc`, or `free` whenever possible.
- Prefer using smart pointers (e.g., `std::unique_ptr`, `std::shared_ptr`) for dynamic memory management.
- Know how to use `std::weak_ptr` to avoid circular references.
  - There are a few instances already where `std::weak_ptr` has been used effectively to break circular dependencies. Be extra careful when working with this code.
- Avoid using a reference type (types that end with a `&`) as class members or static/global variables.

We have provided a `PTR_INSTANCEOF` macro to help with type checking and downcasting of smart pointers.

A common pattern to both check the type and downcast is:

```cpp
if (auto derived = std::dynamic_pointer_cast<Derived>(ptr)) {
    // Use derived
}
```

### Auto and std::any

- Prefer using `auto` for type inference when the type is obvious from the context or the type cannot be easily expressed.
- For range-based for-loops, know how to use `auto` and `auto&` to avoid writing explicit types.

The `std::any` type is relevant here since many visitor classes use to keep the return type flexible.
Normally, when working with `std::any`, one would use `std::any_cast` to retrieve the value of the expected type.
However, `std::any_cast` requires the type to be exact, even when working with inheritance hierarchies.

For example, `std::any_cast<llvm::Value*>(some_any)` will not work if `some_any` contains an `llvm::AllocaInst*`, despite `llvm::AllocaInst` being a derived type of `llvm::Value`.
Therefore, one must avoid using `auto` when storing values in `std::any` as this can lead to subtle bugs and runtime errors.

In general, 
- Try to avoid cases where a bad any cast is possible. 
- Try and be sure of the type stored in an `std::any` before attempting to retrieve it.
- Use `std::any()` to construct an empty `std::any` object in situations where no value is needed.
  - Do not use `nullptr`, `NULL`, `0`, or `false` when the intent is to have no value.

### Const Qualifiers

Regarding the use of `const`:
- Prefer using `const` for variables that should not be modified after initialization.
  - For local variables and non-reference parameters, `const` is not strictly necessary.
  - For reference parameters, always consider whether `const` should be used.
- Use `const` member functions to indicate that they do not modify the object state.
- Be mindful of `const` correctness when working with pointers and references.
- For constant pointers, `const T*` is sufficient.
  - `const T const *`, while technically safer, should generally be avoided for simplicity.

## Comments

For comments:
- Use `//` for comments 1-2 lines long.
- `//` may be used to comment out blocks of code. Some IDEs have a keyboard shortcut for this.
  - Avoid keeping commented-out code in the project.
- Use `/* */` for longer comments.

For documentation comments:
- Use `/** */` for documentation comments. Each line within the comment should begin with a `*` and be indented to align with the `/**`.
- All classes/structs should have a documentation comment describing their purpose and general structure.
  - Use `@brief` to provide a brief description. Provide a longer description unless the usage is very obvious.
- All public member functions should have a documentation comment describing their purpose, parameters, and return value.
  - Use `@brief` to provide a brief description. Provide a longer description unless the usage is very obvious.
  - Use `@param` followed by the parameter name and description for each parameter.
  - Use `@return` to describe the return value.
  - Make it clear when a function may not behave as expected for when it needs specific inputs.
  - Use `@warning` if the function has potential pitfalls or important considerations.
- Some functions such as the visit functions of the visitor pattern may not require extensive documentation.

Class member variables may not need documentation comments if their purpose can be expressed in 1 or 2 lines.

## Other Code Style Preferences

### Braces

- Omitting braces for control structures with a single statement is okay in most cases.
  - Use proper indentation for readability.
- For `switch` cases, use braces for the case body only if necessary (when declaring variables).
- Prefer putting the opening brace on the same line (see example below).
- For `else` and `catch` blocks, write the keyword either on the same line as the closing brace of the preceding block or on a new line (see example below).
  - Be consistent for the entire group of blocks.

```cpp
if (condition) {
    // then block
} else if (other_condition) {
    // else if block
} else {
    // else block
}

// This is also okay
if (condition) {
    // then block
}
else {
    // else block
}
```

### Parentheses and Function Parameters

For function parameters,
- Write the parameters on the same line as the function name if the line can fit within 80 characters.
- Use a single space after the comma in parameter lists.
- For default parameters, using spacing as you normally would for any other assignment.
  - E.g. `void foo(int x = 42, float y = 3.14f) { ... }`
- For long parameter lists, add a new line after the opening parenthesis and before the closing parenthesis, putting each parameter on its own line with indentation.

```cpp
void foo(
    int x,
    float y,
    std::string z
) {
    // function body
}
```

For function calls,
- Write the arguments on the same line as the function name if the line can fit within 80 characters.
- Use a single space after the comma in argument lists.
- For long argument lists, add a new line after the opening parenthesis and before the closing parenthesis, putting each argument on its own line with indentation.

```cpp
foo(
    x,
    y,
    z
);
```

