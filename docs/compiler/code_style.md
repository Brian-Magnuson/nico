# Code Style

This document outlines the coding conventions and style guidelines for this project. 
Please read this document carefully before contributing code.

The goal of this document is to establish a consistent coding style that enhances readability and maintainability.
It is not a strict guideline; we recognize that exceptions may occur.
Exceptions will be considered on a case-by-case basis.

This document does not cover contributing guidelines related to git usage, commit messages, branching strategies, pull request formatting, code review, or the code of conduct.

As this project was originally built for fun, many of the existing styles are a reflection of how I like to write my code.
This document represents my best attempt at formalizing those styles into a coherent set of guidelines.
If this project becomes more widely used in the future, these guidelines may be revisited and updated for a higher level of strictness.

Guiding principles:
- Design for maintainability. Code that cannot be maintained cannot hope to be corrected.
- Design for readability. Code is read more often than it is written.
- Be consistent. Consistency is key to readability.
- Design code for ease of testing. If you have to jump through hoops to test something, then rethink your design.
- Keep it simple, but leave things open for future extension. Sometimes, the simplest solution can make things much harder to extend later.
- Consider how your code can be misused. Bonus points if you make it impossible to misuse.
- If it might fail, make it fail as quickly (and safely) as possible.
- Just because something is obvious to you does not mean it is obvious to others. Explain yourself when necessary.
- Do not write code that does too many things or has too many outputs. Functions should ideally do one thing and do it well.
- Design a good interface, even if your code interfaces with only a few things. This makes it easier to change things later.
- There are many ways to do things, but few that satisfy all of the above. Think critically about your choices, even if it means writing a paper about it.

## C++ Version

Use C++ features up to and including `C++20`. This may change in the future.

## Comments

For comments:
- Use `//` for comments 1-2 lines long.
- `//` may be used to comment out blocks of code. Some IDEs have a keyboard shortcut for this.
  - Avoid keeping commented-out code in the project.
- Use `/* */` for longer comments.
- Use `TODO` for things that need to be done in the future.
- Use `FIXME` for things that need to be fixed.
  - Try to address broken code as soon as possible rather than leaving it for later.

Comments should be used:
- When code is complex and the intent is unclear.
- When using quirks/less-known features of the language.
- When a more obvious implementation is not possible.
  - E.g., "We can't do X because of Y, so we do Z instead."
- When code appears to be missing, but is actually absent for a reason.
  - E.g., "This check is not needed since X already does this."

Comments should be professional and respectful. We generally **discourage** humorous or silly comments.
If you choose to write humorous comments:
- Keep it respectful; nothing offensive or inappropriate.
- Write them only inside functions (function documentation comments do not count).
- Keep it relevant to the code.
- Keep it short; no more than 1 line.
- Avoid sarcasm, as it can be easily misinterpreted.
- Do not write negative comments, complaints, rants, or frustrations.
  - Examples of inappropriate comments:
    - "// This code is terrible!"
    - "// I hate writing this code!"
    - "// I have no idea what this code does!"
    - "// Good luck understanding this code!"
- Avoid anything that detracts from code quality or readability.
- Do not overdo it. Too many humorous comments can be distracting.

For documentation comments:
- Use `/** */` for documentation comments. Each line within the comment should begin with a `*` and be indented to align with the `/**`.
- All classes/structs should have a documentation comment describing their purpose and general structure.
  - Use `@brief` to provide a brief description. Provide a longer description unless the usage is very obvious.
- All public member functions should have a documentation comment describing their purpose, parameters, and return value.
  - Use `@brief` to provide a brief description. Provide a longer description unless the usage is very obvious.
  - Use `@tparam` followed by the template parameter name and description for each template parameter.
  - Use `@param` followed by the parameter name and description for each parameter.
  - Use `@return` to describe the return value.
  - Make it clear when a function may not behave as expected for when it needs specific inputs.
  - Use `@warning` if the function has potential pitfalls or important considerations.
- Some functions such as the visit functions of the visitor pattern may not require extensive documentation.

Class member variables may not need documentation comments if their purpose can be expressed in 1 or 2 lines.

## AI Tools

This section refers to AI tools that provide code generation and chat-based assistance.
These tools typically integrate with the IDE or are web-based.

AI tools such as GitHub Copilot, ChatGPT, and others can be valuable aids in writing code.
Particularly:
- Generating code quickly.
- Generating repetitive/boilerplate code.
- Providing suggestions for complex algorithms or data structures.
- Offering alternative implementations or optimizations.
- Generating test cases.
- Explaining complex code snippets.
- Using library features unfamiliar to the programmer.
- Providing discussion on different approaches to a problem.
- Recommending best practices.

However, AI tools also have limitations and potential pitfalls:
- Generated code may not always follow project-specific coding styles or conventions.
- Generated code may not always be correct or optimal.
- AI tools may not understand the full context of the project or specific requirements.
- AI tools may use library features that, while technically correct, are outdated or even deprecated.
- Over-reliance on AI tools may lead to a lack of understanding of the code being written.
- AI tools may inadvertently introduce security vulnerabilities or bugs.

To ensure the quality and maintainability of the codebase, we provide the following guidelines for using AI tools.

**Do:**
- **Review all generated code thoroughly.** Always read and understand the code generated by AI tools before integrating it into the codebase. Ensure you understand how it works and why it was generated.
- **Test generated code.** Ensure that any code produced by AI tools is adequately tested to verify its correctness and performance.
- **Use AI tools for brainstorming and idea generation.** They can be helpful in exploring different approaches to a problem.
- **Use AI as a sounding board for your ideas.** AI can provide good feedback on your proposed solutions and help you consider arguments for and against your approach. Defending your ideas to an AI can help you think more critically about your choices.
- **Be skeptical of AI suggestions and code.** AI can make mistakes. Be willing to disagree with AI outputs.
- **Understand why AI sometimes makes mistakes.** AI tools commonly use large language models (LLMs) that predict text based on patterns in training data. Understanding the weaknesses of LLMs can help you understand how to properly use AI tools and when to be cautious.
- **Use AI tools to generate tests.** AI is great for generating test cases, but remember that tests may not cover all edge cases.
- **Leverage AI tools for repetitive code patterns.** AI tools can assist in refactoring or transforming repetitive code snippets, especially when a simple find-and-replace is insufficient. However, always review the generated output to ensure correctness, readability, and adherence to the project's coding standards before committing.
- **Add comments when necessary.** If AI-generated code is complex or non-obvious, add comments to explain its purpose and functionality.
- **Ensure code follows the project's coding style.** Modify AI-generated code as needed to adhere to the established coding conventions and styles of the project.

**Do not:**
- **Blindly accept AI-generated code.** Never assume that code generated by AI tools is correct or optimal without review.
- **Generate large portions of code at a time.** Avoid using AI tools to generate entire classes, modules, or systems without careful consideration and review. Prefer generating smaller snippets at a time. This makes it easier to review the code.
- **Generate code that you do not understand.** If you cannot explain how the code works, do not use it.
- **Assume AI tools fully understand the project context.** AI tools may not have access to the full project context, so they may generate code that is not suitable for the specific use case.
- **Assume AI tools are up-to-date.** Always verify that generated code uses current recommended features and follows current best practices.
- **Violate licensing terms or intellectual property rights.** Be aware of the licensing terms of any AI tools used and ensure that generated code does not infringe on intellectual property rights.

Note that when we say "accept" or "use" code, we generally mean when it is committed to the codebase.

Always consider whether you are over-relying on AI tools, and ask yourself two questions:
1. Could I explain this code to someone else?
2. Could I have written this code myself without AI assistance?

By following these guidelines, we can leverage the benefits of AI tools while minimizing their risks and ensuring the quality of our codebase.

## Code Structure

### File Naming

When creating new files, please follow these naming conventions:

- Use `snake_case` for file names (e.g., `my_file.cpp`).
- Avoid using special characters or spaces in file names.
- If the file is for a specific class, name the file after the class
  - E.g., `CodeGenerator` -> `code_generator.h`
- Use `.h` for header files and `.cpp` for implementation files.
- Implementation files should have the same base name as their corresponding header files
  - E.g., `code_generator.h` -> `code_generator.cpp`

### Includes and Include Guards

For all header files, use the following include guard pattern:

```cpp
#ifndef NICO_FILE_BASE_NAME_H
#define NICO_FILE_BASE_NAME_H

// Header file content

#endif // NICO_FILE_BASE_NAME_H
```

The `#endif` comment should always match the `#ifndef` macro name.

View existing header files for more examples.

For `#include` directives, include files in the following order.

1. If the current file is an implementation file, include the corresponding header file first.
2. C++ standard library headers (e.g., `<vector>`, `<string>`).
3. Third-party library headers (e.g., `<llvm/IR/Function.h>`).
4. Project-specific public headers (e.g., `"nico/frontend/code_generator.h"`).
5. Project-specific internal headers (e.g., `"test_utils.h"`).
6. Platform-specific headers (e.g., `<unistd.h>`, `<io.h>`)

Example:
```cpp
#include "nico/frontend/code_generator.h"

#include <string_view>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>

#include "nico/shared/utils.h"
#include "nico/shared/logger.h"
```

For C standard library headers with a C++ counterpart, prefer using the C++ counterpart.
- E.g., use `<cmath>` instead of `<math.h>`, `<cstdlib>` instead of `<stdlib.h>`, etc.
- Prefix names from these headers with `std::`.

For platform specific headers:
- Use conditional preprocessor directives to include the correct headers for each platform.
  - For Windows, use `#if defined(_WIN32) || defined(_WIN64)`
  - For Unix-like systems, use `#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))`
- Always have a fallback option in case of unsupported platforms.

Include what you use in a file.
- E.g., if the file uses `std::any`, it should include `<any>`. If it does not use `std::any`, it should not include `<any>`
- Avoid relying on transitive includes.
  - An exception to this is with header-source file pairs; if an include is in the header file, it does not need to be re-included in the implementation file.

### Namespaces

For names in the `std` namespace, `llvm` namespace, or other external namespaces:
- Use the fully qualified name (e.g., `std::cout`, `std::string`, `llvm::Function`).
- Do not use using directives, i.e., `using namespace`.
- Do not use using declarations, i.e., `using std::cout;`.

With the exception of test files and `main.cpp`, all declarations and definitions should be inside the `nico` namespace.
This includes the special "helper" macros that we define, such as `PTR_INSTANCEOF` and `IS_VARIANT`.

We may use nested namespaces for declarations that have very simple/common names.
- An example of this is the `colorize` namespace, which contains stream manipulators with simple names like `red`, `blue`, and `reset`.
  - Without the nested namespace, these names would be too generic and could easily conflict with other names.
- In other cases, we try to avoid nested namespaces to avoid overly long names.

Namespaces should be opened and closed like this:

```cpp
namespace nico {

} // namespace nico
```

The closing comment is required and must match the namespace name.

Outside of the `nico` namespace:
- Use the fully qualified name (e.g., `nico::Logger`, `nico::Type`).
- Do not use using directives, i.e., `using namespace nico;`.
- Do not use using declarations, i.e., `using nico::Logger;` in header files.
- Avoid using declarations in implementation files.
  - Exceptions include `nico::Tok` and `nico::Err` as they are used frequently in testing.

### Names and Identifiers

For macros and compile-time constants:
- Use `UPPER_SNAKE_CASE`
  - E.g., `PTR_INSTANCEOF`

For variable names, function names, parameter names, and namespace names:
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

### Error Handling and Exceptions

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
- Avoid using `nullptr` to represent optional values. Use `std::optional` instead (see Optional Values section below).

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

The `std::any` type is relevant here since many visitor classes use `std::any` to keep the return type flexible.
Normally, when working with `std::any`, one would use `std::any_cast` to retrieve the value of the expected type.
However, `std::any_cast` requires the type to be exact, even when working with inheritance hierarchies.

For example, this code will not work despite making intuitive sense:
```cpp
auto alloca = builder->CreateAlloca(...);
std::any value = alloca; // Storing a llvm::AllocaInst in std::any
auto retrieved = std::any_cast<llvm::Value*>(value); // This will throw std::bad_any_cast
```
Because `auto` is used here, the type of `alloca` is inferred to be `llvm::AllocaInst*`.
When storing it in `std::any`, the exact type `llvm::AllocaInst*` is stored.
When attempting to retrieve it using `std::any_cast<llvm::Value*>`, it fails, even though `llvm::AllocaInst` is a subclass of `llvm::Value`.

Therefore, one must avoid using `auto` when storing values in `std::any` as this can lead to subtle bugs and runtime errors.

In general, 
- Try to avoid cases where a bad any cast is possible. 
- Try and be sure of the type stored in an `std::any` before attempting to retrieve it.
- Use `std::any()` to construct an empty `std::any` object in situations where no value is needed.
  - Do not use `nullptr`, `NULL`, `0`, or `false` when the intent is to have no value.

### Optional Values

- Prefer using `std::optional` for values that may or may not be present.
  - An exception is if `std::any` is already being used for flexibility (see above section on Auto and std::any).
- Use `std::nullopt` to represent the absence of a value.
- Do not use `nullptr`, `NULL`, `0`, or `false` to represent the absence of a value.

To reduce the size of class instances and leverage polymorphism, we often use pointers to various types like `Type`, `Expr`, and `Stmt`.

For cases where the value is optional, it may be tempting to use `nullptr` to represent the absence of a value.
However, this can lead to ambiguity and potential null pointer dereferencing.

Pointers are expected to point to valid objects. If we start using `nullptr` to represent optional values, it becomes harder to distinguish `nullptr`-bugs from intentional absence of values.

Using `std::optional<T>` makes it clear that the value may or may not be present, and provides a safer way to handle such cases.
- It provides member functions like `has_value()` and `value()` to make it obvious when we are checking for presence and accessing the value.
- It gives us a single, clear way to represent optional values, reducing the need for redundant checks.
- It allows us to clearly distinguish between `nullptr`-bugs and intentional absence of values.
  - When the value is `std::nullopt`, we can be confident that it was intentionally set to be absent.

### Const Qualifiers

Regarding the use of `const`:
- Prefer using `const` for variables that should not be modified after initialization.
  - For local variables and non-reference parameters, `const` is not strictly necessary.
  - For reference parameters, always consider whether `const` should be used.
- Use `const` member functions to indicate that they do not modify the object state.
- Be mindful of `const` correctness when working with pointers and references.
- For constant pointers, `const T*` is sufficient.
  - `const T const *`, while technically safer, should generally be avoided for simplicity.

## Code Formatting

This project uses a [`.clang-format`](../../.clang-format) file to enforce the following coding style guidelines. 
We recommend setting your IDE to format code automatically on save in accordance with this file.

### Line Length

- **Maximum line length is 80 characters.**
- This improves readability in side-by-side diffs, terminals, and small displays.

### Indentation and Tabs

- **Indentation:** Always use **4 spaces** for indentation.
- **Tabs:** Never use tabs; always spaces.
- This ensures the code looks the same in all editors and platforms.

### Braces

- **Braces style:** We generally keep opening braces on the same line as the declaration.
- `else` and `catch` always start on a new line.

  ```cpp
  if (x) {
      doSomething();
  }
  else {
      doSomethingElse();
  }
  ```

### Line Breaking for Brackets and Arguments

For function parameters, arguments, and list initializations:
- If they fit on one line, keep them on one line.
- If they don’t, put each element on its own line, indenting like a block.

  ```cpp
  myFunction(
      longArgumentName,
      anotherArgument,
      moreArguments
  );
  ```

For constructor initializer lists:
- Write constructor initializers starting on the next line.
- If they fit on one line, keep them on one line.
- If they don’t, put each initializer on its own line, aligned with the first initializer.

  ```cpp
  MyClass::MyClass() 
      : memberA(1), memberB(2) {
      // Constructor body
  }

  MyClass::MyClass()
      : memberA(1),
        memberB(2),
        memberC(3),
        memberD(4) {
      // Constructor body
  }
  ```

### If Statements

- Do not keep short if-statements on one line.
- If braces are omitted, the statement must be on a new line.

  ```cpp
  if (condition)
      doSomething();
  ```

### Short Functions

- **One-line functions** are only allowed inside **class definitions** (e.g., trivial getters/setters).
- Outside classes, function bodies must break onto a new line after the opening brace.

  ```cpp
  // Allowed inside class
  class Foo {
      int getValue() { return value; }
  };

  // Required outside class
  int Foo::getValue() {
      return value;
  }
  ```

### Template Declarations

- **Template declarations** should always be on their own line.

  ```cpp
  template <typename K, typename V, typename Hash = std::hash<K>>
  class Dictionary {
      // ...
  };
  ```

### Access Modifiers and Case Labels

- **Access modifiers (`public`, `protected`, `private`)** are aligned with the `class` keyword (indented **-4** from the class body).

  ```cpp
  class Foo {
  public:
      void bar();
  };
  ```
- **Case labels** inside `switch` statements align with the `switch` keyword rather than being indented.

  ```cpp
  switch (x) {
  case 1:
      break;
  default:
      break;
  }
  ```

### Pointers and References

- Pointer and reference symbols stick to the type, not the variable.

  ```cpp
  int* ptr;   // Correct
  int *ptr;   // Please don't
  int * ptr;  // Please don't
  ```

### Namespaces

- Close namespaces with a comment indicating the namespace name.
- Keep code inside namespaces aligned with the namespace declaration.

  ```cpp
  namespace nico {

  void my_function() {
      // ...
  }

  } // namespace nico
  ```
