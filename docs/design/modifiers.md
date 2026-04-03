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
The type checker will need to enforce that modifiers are only applied to valid declarations, and that the modifiers themselves are valid for the declaration they are applied to.

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
- `override` - The method that follows should override a virtual method in a base class. The type checker will error if there is no virtual method with the same name and signature in a base class.
- `final` - The method that follows cannot be overridden by derived classes. The type checker will error if a derived class attempts to override a method marked as `final`.
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

This syntax is meant to make parsing modifiers easier.
However, the benefits of this syntax are lost if modifiers are too difficult to type check.

Consider this example:
```
#[abstract]
static var my_var: i32 = 0;
```

The modifier syntax implies that any modifier can be applied to any statement.
And yet, it is clear that `abstract` cannot be applied to a variable declaration.
We need to ensure that each statement has a valid set of modifiers that can be applied to it.

We need to consider how modifiers are checked:
- Is the statement incorrect because it it has an invalid modifier?
- Or is the modifier incorrect because it is applied to the wrong statement?

This gives us two approaches to type checking modifiers:
- Statement-centric: Statements are checked for the modifiers they have applied to them.
- Modifier-centric: Modifiers are checked for the statements they are applied to.

We also need to consider *when* modifiers are checked:
- During parsing?
- During a separate modifier checking phase after parsing but before type checking?
- During type checking?
- During a separate modifier checking phase after type checking?
- Or some combination of the above?

While evaluating these approaches, we should consider the general complexity of the implementation.
Validating modifiers may not be an easy task.
But if this syntax is to be successful, it should not be any more difficult to validate than an alternative syntax (such as the aforementioned C#-style modifiers).

## Statement-Centric Approaches

In a statement-centric approach, we check statements for the modifiers they have applied to them.
This approach works well with the current implementation of the compiler since the type checker already performs type checking by visiting statements one-by-one.

Starting in the parser, each statement node will have an additional field for modifiers:
```cpp
class Stmt {
  const Location* location;

  Dictionary<std::string, Modifier> modifiers; // New field for modifiers.
};
```

When the parser encounters a modifiers, it temporarily stores the modifiers in a dictionary until it encounters the next statement, at which point it attaches the modifiers to the statement node and clears the temporary storage.
This way, the modifiers are attached to the statement node in the AST, and can be easily accessed during type checking.

Next, during type checking, when we visit a statement node, we can check the modifiers attached to it and ensure that they are valid for that statement.
```cpp
std::any TypeChecker::visit(Stmt::Static* stmt) {
  // (normal type checking for static variable declaration)

  // Check modifiers.
  for (const auto& [name, modifier] : stmt->modifiers) {
    if (name == "public") {
      // handle public modifier...
    }
    else if (name == "private") {
      // handle private modifier...
    }
    else if (name == "abstract") {
      // handle abstract modifier...
    }
    else {
      Logger::inst().log_error(
        Err::InvalidModifier,
        stmt->location,
        "Invalid modifier '" + name + "' applied to static variable declaration."
      );
    }
}
```

For modifiers that apply to any statement, like `suppress_warning`, we in the type-checking loop outside of the visit call:
```cpp
void TypeChecker::check(std::vector<std::shared_ptr<Stmt>>& statements) {
  for (const auto& stmt : statements) {
    // Check modifiers that apply to any statement.
    if (stmt->modifiers.contains("suppress_warning")) {
      // handle suppress_warning modifier...
    }

    // Type check the statement.
    visit(stmt);
  }
}
```

Benefits of statement-centric approaches:
- It is easier to implement since it fits well with the existing structure of the compiler.
- It allows for more context when checking modifiers, as the statement being checked is readily available.

Drawbacks of statement-centric approaches:
- It can lead to more complex and less modular code, as the logic for checking modifiers is intertwined with the logic for type checking statements.
- It can be more difficult to extend, as adding new modifiers may require changes to multiple visit methods for different statement types.

## Modifier-Centric Approaches

In a modifier-centric approach, we check modifiers for the statements they are applied to.
Each modifier will have its own logic for checking its validity and applying its effects.
This can be achieved through either simple conditional branches, class methods, or even a visitor pattern for modifiers.

At some point, we loop through each statement and check the modifiers applied to it. Then we run the modifier validation logic, providing the statement as context for the modifier to check itself against.
```cpp
void TypeChecker::check(std::vector<std::shared_ptr<Stmt>>& statements) {
  for (const auto& stmt : statements) {
    // Check modifiers.
    for (const auto& [name, modifier] : stmt->modifiers) {
      modifier.check_with(stmt);
    }

    // Type check the statement.
    visit(stmt);
  }
}
```

As mentioned, each modifier has its own logic for checking its validity and applying its effects.
```cpp
class Mod::Public : public Mod {
public:
  void check_with(const std::shared_ptr<Stmt>& stmt) override {
    // Check if the statement is a valid declaration that can be public.
    if (!stmt->is_declaration()) {
      Logger::inst().log_error(
        Err::InvalidModifier,
        stmt->location,
        "The 'public' modifier can only be applied to declarations."
      );
    }
    // Additional checks for specific declaration types can be added here.
  }
};
```

Class-based modifiers are useful if each modifier has a different structure.
However, modifiers all have the same structure:
- They all have a name.
- They all optionally have arguments, which are scanned as un-parsed tokens.
- They all have a statement to which they are applied.

If we don't want to implement each modifier as a class, we can also try a "lookup table" approach, where we have a mapping of modifier names to their checking logic:
```cpp
std::unordered_map<std::string, std::function<void(const std::shared_ptr<Stmt>&, std::vector<std::shared_ptr<Token>>)>> modifier_checkers = {
  {"public", check_public_modifier},
  {"private", check_private_modifier},
  {"abstract", check_abstract_modifier},
  // ... other modifiers
};
```

The key point is that the modifier-centric approach means each modifier has its own logic for checking its validity and applying its effects.

Benefits of modifier-centric approaches:
- It leads to more modular and maintainable code, as the logic for each modifier is contained within its own function or class.
- It can be easier to extend, as adding new modifiers may only require adding a new function or class for the modifier, without needing to modify existing visit methods for different statement types.

Drawbacks of modifier-centric approaches:
- It can be more difficult to implement, as it may require a new setup for handling modifiers, such as a lookup table or a visitor pattern.
- It may require more context to be passed to the modifier checking logic, as the statement being checked may not be readily available within the modifier's logic, depending on how it is structured.

