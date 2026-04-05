# Modifiers

Various programming languages feature modifiers, annotations, decorators, and attributes, which can be used to specify additional information about classes, methods, and other code elements. 
These modifiers can indicate access levels (e.g., public, private), behavior (e.g., virtual, static), and other characteristics (e.g., unsafe, external).
Depending on how these constructs are implemented, they can potentially be difficult to parse and represent.

This document proposes a new syntax for Nico that unifies the concept of modifiers, annotations, decorators, and attributes into a single, consistent format.

# Introduction

Various programming languages have different ways of specifying modifiers, annotations, decorators, and attributes.
For example, in C#, you can prepend methods and variables with modifiers like `public`, `static`, `unsafe`, and `extern` to indicate their access level, behavior, and other characteristics.
```cs
class MyClass
{
    public static unsafe extern void MyMethod();

    protected virtual internal void MyOtherMethod() { }
}
```
The order of modifiers generally does not matter, but there is usually a recommended order for readability.

There are also annotations and decorators in languages like Java and Python, which can be used to add metadata or modify the behavior of classes and methods.
```java
public class MyClass {
    @MyAnnotation
    public void myMethod() { }

    @AnotherAnnotation
    protected void myOtherMethod() { }
}
```
```python
class MyClass:
    @my_decorator
    def my_method(self):
        pass

    @another_decorator
    def my_other_method(self):
        pass
```

C# uses attributes, which are specified in square brackets and can be applied to various code elements.
```cs
[MyAttribute]
public class MyClass
{
    [MyMethodAttribute]
    public void MyMethod() { }

    [MyOtherMethodAttribute]
    protected void MyOtherMethod() { }
}
```

Rust has "attribute-like macros" that can be used to modify the behavior of code elements.
```rust
#[derive(Debug)]
struct MyStruct {
    #[serde(skip)]
    field: String,
}
```

All of these constructs serve a similar purpose: they *modify* the behavior or other characteristics of code elements. 
However, they are implemented in different ways across languages, which can make it difficult to parse and represent them in a consistent manner.

Here, we have an opportunity to design a new syntax for Nico that unifies these concepts into a single, consistent format.

# C# and Java's Modifier Syntax

Before we discuss our proposed syntax, we will give special attention to C# and Java's modifier syntax, as they provide a clean way to specify certain modifiers.
We will use our example from before:
```cs
class MyClass
{
    public static unsafe extern void MyMethod();

    protected virtual internal void MyOtherMethod() { }
}
```

Let us make a few observations about this syntax:
- Modifiers are keywords.
- Modifiers are prepended to the declaration and are separated by spaces.
- The order of modifiers does not matter.

What makes this syntax particularly nice is how it feels "connected" to the declaration.
The modifiers are not just annotations that are attached to the declaration; they are an integral part of the declaration itself.
It encourages the programmer to think of the modifiers as part of the declaration.

We can attempt a similar syntax in Nico:
```
class MyClass:
    public unsafe extern func my_method() -> void {}

    protected virtual internal method my_other_method() -> void {}
```

One way to parse this is to treat modifiers as belonging to a set of keywords that can be prepended to declarations.
When the parser encounters a modifier keyword, it can add it to a temporary list of modifiers to apply to the next declaration it encounters.
This is especially easy if:
- Modifiers do not require additional arguments.
- Modifier keywords are not already used for other purposes in the language.

Unfortunately, we *do* use some of these keywords for other purposes in Nico. 
`unsafe`, `extern` are already used in unsafe blocks and extern namespace declarations.
Additionally, we want to support modifiers that can take arguments, such as `linkage("external")` and `symbol("MyMethod")`.

Another way to parse modifiers is to use them as "wrappers" around declarations.
For example, suppose we have the following:
```
extern func my_method() -> void {}
```

We can parse this as an `extern` node that wraps a `func` declaration node.
```
(stmt:externdecl (stmt:func my_method void => (block (stmt:yield => void))))
```

While this approach allows each modifier to be handled independently, it requires more AST node types, making the AST more complex.
It is also more difficult to extend as each modifier requires a new node type.

# Proposed Syntax

Our proposed syntax is inspired by Rust's attribute-like macros, but is used with all modifiers.

It is a written using a hash `#` symbol followed by a list of modifiers enclosed in square brackets `[]`.
Modifiers can be separated by commas, and can optionally take arguments in parentheses `()`.
These arguments can be any token or group of tokens, allowing for great flexibility in specifying modifier parameters.
```
class MyClass:
    #[public, unsafe, linkage("external"), symbol("MyMethod")]
    func my_method() -> void {}

    #[protected, virtual, internal]
    method my_other_method() -> void {}
```

Obviously, modifiers cannot be apply to just any declaration.
Either the type checker or parser will need to enforce that modifiers are only applied to valid declarations, and that the modifiers themselves are valid for the declaration they are applied to.

This syntax has several advantages:
- It is visually distinct from the rest of the code, making it easy to identify modifiers at a glance.
- It allows for modifiers to be specified in any order, as they are all contained within the square brackets.
- It supports modifiers that take arguments, allowing for greater flexibility in specifying modifier parameters.
- It is easy to parse, as the bracket tokens clearly indicate the start and end of the modifier list, and the hash symbol indicates that it is a modifier declaration.
- It unifies the concept of modifiers, annotations, decorators, and attributes into a single format.
- It is easy to extend, as new modifiers can be added without requiring new syntax or AST node types.

This syntax also has a unique advantage:
- Modifiers do not need to be keywords in the language. 
  - This is because the `#[]` tokens create a special context where only modifiers are expected, allowing for any identifier to be used as a modifier without conflicting with existing keywords.

Because of this, you can have cases like this:
```
#[public]
func public() -> void {}

#[symbol("symbol")]
static var symbol: i32 = 0;
```

This code may be unusual, but it is perfectly valid and demonstrates the flexibility of this syntax.

There are a few potential disadvantages to this syntax:
- It is detatched from the declaration it modifies, making it feel less like "normal" code and more like "meta" code.
- Familiar modifiers are put in the same syntax as more advanced modifiers.
  - The user is "playing with knives"; some are safe, some are essential, some are just decorative, and some are dangerous.
- Wrong modifiers are not caught until the type checking phase.
- It may be more verbose than other modifier syntaxes, especially for simple modifiers that do not take arguments.

There are ways to mitigate these disadvantages:
- Standardize a "default" behavior for declarations, reducing the need for modifiers in simple cases.
  - For example, struct members could be `public` by default and class members could be `private` by default, reducing the need for access modifiers in common cases.
- Provide clear documentation and error messages for modifiers, especially for those that are more advanced or dangerous
- Encourage the use of modifiers when teaching the language, so that users become familiar with them and understand their purpose and potential risks.

# List of Modifiers

Here, we describe some of the modifiers that we may support in Nico, along with their syntax and semantics.

## Scope Access Modifiers

- `public` - The declaration that follows is accessible from outside of its scope. This does not affect its visibility outside the module.
- `private` - The declaration that follows is not accessible from outside of its scope. This does not affect its visibility outside the module.
- `protected` - The declaration that follows is a class member that is not accessible from outside of the class, except for derived classes. This does not affect its visibility outside the module.
- `friend(SCOPE)` - The declaration that follows, in addition to the current scopes from which it is accessible, is also accessible from the specified scope.
  - SCOPE - A name that specifies the scope from which the declaration is also accessible. The type checker will error if the specified scope does not exist or is not a valid scope to be a friend of the declaration.
- `not_safe` - The function that follows must only be called from an unsafe block.

## Linkage Modifier

- `linkage(LINKAGE)` - The static variable or function that follows shall have the specified linkage.
  - LINKAGE - A string literal that is one of the following:
    - `"internal"` - The declaration is only visible within the current module. This is the default linkage for all declarations.
    - `"external"` - The declaration is visible to other modules and can be linked to from other modules.

## Symbol Modifier

- `symbol(SYMBOL)` - The static variable or function that follows shall have the specified symbol name in the generated code.
  - SYMBOL - A string literal that specifies the symbol name to use for the declaration in the generated code. The string can be anything, but the type checker will error if it conflicts with an existing symbol in the same module.

## Object-Oriented Programming Modifiers

- `virtual` - The method that follows is virtual, meaning it can be overridden by derived classes.
- `override` - The method that follows should override a virtual method in a base class. The type checker or parser will error if there is no virtual method with the same name and signature in a base class.
- `final` - The method that follows cannot be overridden by derived classes. The type checker or parser will error if a derived class attempts to override a method marked as `final`.
- `abstract` - This modifier behaves differently depending on the declaration it is applied to:
  - When applied to a class, it indicates that the class cannot be instantiated and may contain abstract methods.
  - When applied to a method, the enclosing scope must be a class, the class is required to be marked as `abstract`, and the method must not have an implementation. Derived classes are either required to provide an implementation for the method or be marked as `abstract` themselves.
- `sealed` - The class or struct declaration that follows is sealed; no other class or struct may derive from it.
- `deleted` - The following method header represents a method that is implicitly defined in the current scope and shall be deleted. Deleted methods cannot be called.

## Testing Modifiers

- `test(NAME)` - The function that follows is a test function. Its return type must be `void` and it must not take any parameters. The test runner will execute all functions marked with this modifier as part of the testing process.
  - NAME - A string literal that specifies the name of the test. This is used for reporting test results and can be any string.
- `test_suite(NAME)` - The namespace that follows is a test suite. The test runner will execute all test functions within the namespace as part of the testing process.
  - NAME - A string literal that specifies the name of the test suite. This is used for reporting test results and can be any string.

## Warning Modifiers

- `suppress_warning` - The statement that follows may emit a warning, and the warning shall be suppressed.
- `deprecated(MESSAGE)` - The declaration that follows is deprecated and should not be used. The type checker will emit a warning whenever the declaration is used.
  - MESSAGE - A string literal that specifies the deprecation message to include in the warning. This can be any string, but it is recommended to include information about why the declaration is deprecated and what should be used instead.

# Applying Modifiers

The main advantage of this syntax is to make parsing modifiers easier.

They are parsed before the statement they are applied to.
```cpp
std::optional<std::shared_ptr<Stmt>> Parser::statement() {

    modifiers();  // Parse modifiers and store them in a temporary list.

    // Consume semicolons to separate statements
    while (match({Tok::Semicolon}))
        ;

    if (match({Tok::KwLet, Tok::KwStatic})) {
        return variable_statement();
    }
    else if (match({Tok::KwFunc})) {
        return func_statement();
    }
    else if (match({Tok::KwNamespace})) {
        return namespace_statement();
    }
    //...
```

We don't need to complicate the type checker by having to check for modifiers in every visit method for every statement type.
If the parser is capable of applying the modifiers to the statement nodes in the AST, then the type checker does not need to worry about checking them separately.

The parser's job is to produce an AST that accurately represents the source code.
For the AST to support the application of modifiers, many AST nodes will need new fields for the data associated with modifiers.
For example, for the `linkage` modifier, `Stmt::Func` and `Stmt::Static` nodes will need a new field for the linkage type specified by the modifier.
```cpp
class Stmt::Func : public Stmt {
  enum class LinkageType { Internal, External } linkage; // New field for linkage type.
  //...
};
```

There are two main approaches to applying modifiers in the parser:
- Statement-centric: The statements check what modifiers can be applied to them and apply them in their parsing functions.
- Modifier-centric: The modifiers check what statements they can be applied to and apply themselves in a separate function after the statement is parsed.
- Hybrid: Some modifiers are applied in a statement-centric way, while others are applied in a modifier-centric way.

In the following approaches, we make an important assumption:
- We do not check for the absense of modifiers.

This assumption implies that all statements have a "default" behavior when no modifiers are applied, and that modifiers only modify the behavior of statements without being required for the statement to be valid.

It is still possible for the absense of a modifier to result in an error.
For example, a method can only be marked as `override` if the base class method is marked as `virtual`.
But this check is done in the type checker and is done by examining the metadata of the AST node and not by checking for the absense of the `virtual` modifier.

## Statement-Centric Approach

In a statement-centric approach, the parser applies modifiers directly within the parsing function for the statement to which they are applied.
For example, when parsing a function declaration, the parser can check for any modifiers that were parsed before the function declaration and apply them to the function node in the AST.
```cpp
std::shared_ptr<Stmt> Parser::func_statement(std::vector<Modifier>& modifiers) {
    auto func_node = std::make_shared<Stmt::Func>();

    // (normal parsing for function declaration)

    // Apply modifiers to the function node.
    for (const auto& modifier : modifiers) {
        if (modifier.name == "linkage") {
            func_node->linkage = parse_linkage(modifier.args);
        }
        // ... handle other modifiers
    }

    return func_node;
}
```

Note that the parsing function signature had to be changed to accept the list of modifiers as an argument, which may require changes to the way the parser is structured.
We cannot use a global temporary list of modifiers in this approach, as it would not work well with nested statements and would make the parser less modular.

Advantages of the statement-centric approach:
- Statements handle their own modifiers; no guessing what kind of statement is being modified.
- It allows for more context when applying modifiers, as the statement being modified is readily available.
- It is easier to implement since it fits well with the existing structure of the compiler.

Disadvantages of the statement-centric approach:
- Parsing functions may become more complex, as they need to handle the application of modifiers in addition to their normal parsing logic.
- It can be more difficult to extend, as adding new modifiers may require changes to multiple parsing functions for different statement types.
- The parser needs to be updated to pass the list of modifiers to the appropriate parsing functions, which may require significant changes to the parser's structure.
- It can lead to more complex and less modular code, as the logic for applying modifiers is intertwined with the logic for parsing statements.

## Modifier-Centric Approach

In a modifier-centric approach, the parser applies modifiers in a separate function that is called after the statement is parsed.
For example, after parsing a function declaration, the parser can call a separate function to apply any modifiers that were parsed before the function declaration to the function node in the AST.
```cpp
std::optional<std::shared_ptr<Stmt>> Parser::statement() {

    auto parsed_modifiers = modifiers();  // Parse modifiers and store them in a temporary list.

    // Consume semicolons to separate statements
    while (match({Tok::Semicolon}))
        ;

    if (match({Tok::KwLet, Tok::KwStatic})) {
        return variable_statement();
    }
    else if (match({Tok::KwFunc})) {
        return func_statement();
    }
    else if (match({Tok::KwNamespace})) {
        return namespace_statement();
    }
    //...


    // After parsing the statement, apply any modifiers that were parsed before it.
    apply_modifiers(parsed_modifiers, stmt);

    return stmt;
}
```

Advantages of the modifier-centric approach:
- All the logic for applying modifiers is contained within a single function, making it more modular and easier to maintain.
- It can be easier to extend, as adding new modifiers may only require changes to the modifier application function, without needing to modify existing parsing functions for different statement types.
- There is no need to pass the list of modifiers to the parsing functions, which can simplify the parser's structure and reduce the amount of changes needed to implement modifiers.

Disadvantages of the modifier-centric approach:
- Less context is available when applying modifiers, as the statement is boxed as a generic statement node and may require downcasting to determine its specific type.

## Hybrid Approach

In a hybrid approach, most modifiers are applied in a statement-centric way, but a few modifiers that require less context or are more general can be applied in a modifier-centric way.
For example, the `abstract` modifier can be applied in a statement-centric way since it can only be applied to class and method declarations:
```cpp
std::shared_ptr<Stmt> Parser::func_statement(std::vector<Modifier>& modifiers) {
    auto func_node = std::make_shared<Stmt::Func>();

    // (normal parsing for function declaration)

    // Apply abstract modifier in a statement-centric way.
    if (modifiers.contains("abstract")) {
        func_node->is_abstract = true;
        modifiers.erase("abstract"); // Remove the modifier from the list after applying it.
    }

    return func_node;
}
```

And the `suppress_warning` modifier can be applied in a modifier-centric way since it can be applied to any statement:
```cpp
std::shared_ptr<Stmt> Parser::statement() {

    auto parsed_modifiers = modifiers();  // Parse modifiers and store them in a temporary list.

    // Consume semicolons to separate statements
    while (match({Tok::Semicolon}))
        ;

    std::shared_ptr<Stmt> stmt;

    if (match({Tok::KwLet, Tok::KwStatic})) {
        stmt = variable_statement(parsed_modifiers);
    }
    else if (match({Tok::KwFunc})) {
        stmt = func_statement(parsed_modifiers);
    }
    else if (match({Tok::KwNamespace})) {
        stmt = namespace_statement(parsed_modifiers);
    }
    //...

    // Apply suppress_warning modifier in a modifier-centric way.
    if (parsed_modifiers.contains("suppress_warning")) {
        stmt->suppress_warning = true;
        parsed_modifiers.erase("suppress_warning"); // Remove the modifier from the list after applying it.
    }

    return stmt;
}
```

Note that we now have to erase modifiers from the list after applying them.
Because modifiers are handled in two different places, we need to somehow convey which modifiers have already been applied and which ones still need to be applied.
This allows us to catch errors such as applying the same modifier twice or applying an invalid modifier to a statement.

Advantages of the hybrid approach:
- It allows for more context when applying modifiers that require it, while still keeping the logic for more general modifiers separate and modular.
- It can be easier to extend, as adding new modifiers may only require changes to either the statement-centric parsing functions or the modifier-centric application function, depending on the nature of the modifier, without needing to modify both.

Disadvantages of the hybrid approach:
- It can be more complex to implement, as it requires handling modifiers in two different places and ensuring that they are applied correctly without conflicts.
- The parser needs to be updated to pass the list of modifiers to the appropriate parsing functions, which may require significant changes to the parser's structure.
- It may be less consistent, as some modifiers are applied in a statement-centric way while others are applied in a modifier-centric way, which may be confusing for users and developers.

### General and Special Modification Method

One way to implement the hybrid approach is to add two methods to the Stmt class:
```cpp

class Stmt {
  bool apply_general_modifier(const Modifier& modifier) {
    // Check for general modifiers that can be applied to any statement.
    if (modifier.name == "suppress_warning") {
      this->suppress_warning = true;
      return true; // Return true if the modifier was applied.
    }
    // ... handle other general modifiers

    return false; // Return false if the modifier was not applied.
  }

  virtual bool apply_special_modifier(const Modifier& modifier) {
    // This method can be overridden by specific statement types to handle special modifiers that require more context.
    return false; // Return false if the modifier was not applied.
  }

public:

  bool apply_modifier(const Modifier& modifier) {
    return apply_special_modifier(modifier) || apply_general_modifier(modifier);
  }
};
```

We can describe the modifiers that apply to any statement as "general modifiers", and the modifiers that require more context and are applied in a statement-centric way as "special modifiers".

All the logic for handling general modifiers is contained within the `apply_general_modifier` method.
It is not `virtual`; it is the same for all statement types, and it can only apply modifiers that are valid for any statement.

If a statement type has a special modifier that requires more context, it can override the `apply_special_modifier` method to handle that modifier.

Note that a modifier is not strictly "general" or "special"; it is just a matter of how we choose to implement the logic for applying it.

For example, consider a modifier that applies to declarations.
- You could say the modifier applies to *any declaration*, and implement it as a *general modifier* that is handled in the `apply_general_modifier` method, which checks if the statement is a declaration and applies the modifier if it is.
- You could say the modifier applies to *only declarations*, and implement it as a *special modifier* that is handled in the parsing functions for declaration statements.

The effect is the same in either case.
What we decide for the modifier depends on which implementation we think is cleaner and easier to maintain for that specific modifier.

This isn't a drawback of the hybrid approach.
In fact, it is a strength: we can flexibly choose how to implement the logic for applying each modifier in a way that makes the most sense for that modifier, without being constrained by a single approach for all modifiers.
