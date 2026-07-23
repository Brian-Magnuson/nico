# Basic Methods

This document discusses how basic methods will be implemented in the Nico programming language.
The word "basic" in this context refers to the implementation complexity of these methods in the compiler, not their functionality (though they are also simple in that regard).
Mainly, we shall focus on struct methods as structs do not support all OOP features and are designed to be simple and efficient.

> [!WARNING]
> This document is a work in progress and is subject to change.

## Introduction

The Nico programming language features a series of **complex types**, which are aggregate types containing named fields.
We organize them into three kinds:
- **Objects**: These are complex types that are not named and only support fields. They provide the most basic functionality and are the simplest to implement.
- **Structs**: These are named complex types that support fields and methods. They do not support advanced inheritance or polymorphism features, but the addition of methods enables encapsulation and better organization of code.
- **Classes**: These are named complex types that support fields, methods, access specifiers, and advanced OOP features such as inheritance and polymorphism. They are the most complex to implement but provide the most functionality.

This system of complex types gives the user many options for organizing their code and data.
Objects provide simple data structures without the need of struct/class definitions.
Structs provide more functionality while their simplicity keeps them efficient and easy to use.
Classes provide the most functionality and flexibility, but at the cost of complexity and potential performance overhead.

We bring special attention to structs, where we begin to introduce methods into our language.

A **method** is a kind of function that is associated with a complex type.
Methods are defined within the context of a complex type and can access its fields and other methods.
They are called on instances of the complex type and can be used to manipulate the data contained within that instance.

From an OOP perspective, where fields define the *state* of an object (what makes up its identity), methods define the *behavior* of that object (what it can do).

Our goal is to implement methods in a way that is simple and will not introduce unnecessary overhead in structs, while still providing the user with a powerful tool for organizing their code and data.


## Functions

First, we will observe regular functions.
```
func add(a: i32, b: i32) -> i32 {
    return a + b
}
```

A function is introduced with the `func` keyword, followed by its name, a list of parameters, and a return type.
The parameters are the inputs to the function.

You can define a function as part of a struct.
This is allowed because structs in Nico behave like namespaces where you can group declarations together:
```
struct MyStruct {
    func add(a: i32, b: i32) -> i32 {
        return a + b
    }
}
```

This does *not* make `add` a method of `MyStruct`. 
It won't have access to the fields of `MyStruct` and it won't be called on instances of `MyStruct`.
This function, in this context, is said to be **shared**. This is analogous to static methods in other languages like Java and C++.


## Function purity

A function is said to be **pure** if it meets the following criteria:
- Its output is determined solely by its input parameters.
- It does not have any side effects, such as modifying global state or performing I/O operations

The above `add` function is an example of a pure function, as it only depends on its input parameters and does not modify any external state.

Here is an example of an impure function:
```
static var global_counter = 0
func increment_counter() {
    global_counter += 1
}
```

This function modifies a global variable, which is a side effect, and therefore it is considered impure.

Here is another example of an impure function:
```
static var global_counter = 0
func get_counter() -> i32 {
    return global_counter
}
```

This function reads a global variable, which means its output is not solely determined by its input parameters.
Other functions can modify this global variable, which can result in this function returning different values at different times.

It is useful for programmers to know how their functions behave and whether they are pure or impure, as this can affect how they can be used and optimized.
For example, suppose we have a sorting function.
Its purpose is to sort a list of numbers in ascending order.
We can implement this function in multiple ways:
- We can implement it as a pure function that takes a list of numbers as input and returns a new sorted list as output.
- We can implement it as an impure function that takes a list of numbers as input and modifies that list in place to sort it.

The first approach is simple and easy to reason about, but it may be less efficient as it requires creating a new list.
The second approach is more efficient as it modifies the list in place, but it can be harder to reason about as it has side effects and can lead to unexpected behavior if not used carefully.

The concept of purity is important for when we implement methods, as methods are inherently tied to the state of the instance upon which they are called.


## Methods

We want methods to be visually distinct from shared functions, so we will introduce a new keyword `method` to define methods.

> [!CAUTION]
> The following example does not reflect the final design.
```
struct MyStruct {
    field a: i32

    method add(b: i32) -> i32 {
        return self.a + b
    }
}
```

The fields of a method are accessed through a special identifier `self`.
Other languages provide a similar feature, some using the keyword `this` instead of `self`.

You would then call the method on an instance of `MyStruct` like so:
```
let my_struct = new MyStruct { a: 5 }
let result = my_struct.add(3)  // result is 8
```

In my opinion, this design is weak.
The method signature does not indicate where `self` comes from or what this function does with it.
This is especially important since we care about immutability in our language.
```
let my_struct = new MyStruct { a: 5 }
my_struct.add(3)  // If this modifies my_struct, then it's not allowed since my_struct is immutable.
```

C++ lets you specify whether a method is `const` or not, which indicates whether it modifies the instance or not.
```cpp
struct MyStruct {
    int a;

    int add(int b) const {  // This method does not modify the instance.
        return a + b;
    }
};
```

Rust has a similar feature, except that `self` is written as a *parameter* to the method, which makes it more explicit.
```rust
struct MyStruct {
    a: i32,
}
impl MyStruct {
    fn add(&self, b: i32) -> i32 {  // This method does not modify the instance.
        self.a + b
    }
}
```

The design choice of having `self` as a parameter is somewhat elegant.
With parameters, you can already specify whether the parameter is mutable or not, so we can use the same syntax for `self` as well.

There are some drawbacks to this approach, however:
- You can't specify just any type for `self`. It must be the type of the struct that the method is defined on.
- This parameter always has to be first in the parameter list.
- You can't pass the instance directly as an argument to the method. You have to use the `.` operator to call the method on the instance.

Drawbacks aside, I find that this design offers the cleanest and clearest syntax for methods.


## The receiver parameter

Here, we will refine our method design to use the `self` parameter, and examine the `self` parameter in more detail.

```
method add(self: MyStruct, b: i32) -> i32 {
    return self.a + b
}
```

The first parameter of a method in Nico is called the **receiver parameter**.
The receiver parameter is a special parameter that represents the instance of the struct that the method is called on.
It is required to be the first parameter of a method definition and must use an allowed type for the struct that the method is defined on.

We name this parameter `self`, but it can be named anything.
We can even allow the user to omit the name of the receiver parameter, in which case it will default to `self`.
```
method add(this: MyStruct, b: i32) -> i32 => this.a + b

method add(:MyStruct, b: i32) -> i32 => self.a + b
```

This is the only time you can omit a name for a parameter in Nico.
When doing so, the colon is required to indicate that the type is being specified without a name.

When calling a method, you do not provide an argument for the receiver parameter.
Attempting to do so is an error.
Instead, the instance of the struct that the method is called on is automatically passed as the argument for the receiver parameter.
```
let my_struct = new MyStruct { a: 5 }
let result = my_struct.add(3)  // result is 8
MyStruct.add(my_struct, 3)  // Error: add has no overload that takes a MyStruct as the first argument
```

The receiver parameter is only allowed a certain set of types.
For a struct `T`, the receiver parameter is currently allowed to be one of the following types:
- `T`: The receiver parameter is a copy of the instance.
- `@T`: The receiver parameter is a raw, immutable pointer to the instance.
- `var@T`: The receiver parameter is a raw, mutable pointer to the instance.

The way the method is called will not change.
It is always called using the dot operator:
```
my_struct.add(3)
```

What changes is *how* the instance is passed to the method.
As we have described, depending on the type of the receiver parameter, the instance may be passed as a copy, or a pointer.
The user has the freedom to choose which type of receiver parameter they want to use.

Passing by copy ensures the original instance is not modified and the provided instance can be used freely within the method.
However, it may be less efficient if the struct is large and expensive to copy.
On the other hand, passing by pointer allows the method to modify the original instance and can be more efficient, but it requires the user to be careful about mutability and ownership.


## Accessor method problem

This section discusses a special pattern with C++ accessor methods, whether or not Nico also has this pattern, and whether or not it will be a problem in the future.

Users of C++ will know that containers such as `std::vector` with accessor methods like `at` and `operator[]` have two overloads: one that uses const and one that does not.
Developers of custom data structures in C++ often follow this pattern as well.
In fact, Nico's special `Dictionary` class has this pattern as well:
```cpp
V& operator[](K key);

const V& operator[](K key) const;
```

Both are needed for their respective use cases.

If we want to be able to modify data, we need to use the non-const version of the accessor method.
```cpp
Dictionary modifiable_dict;
modifiable_dict["key"] = "value";  // Calls the non-const version of operator[]
```

And if we can't modify data, we need to use the const version of the accessor method.
```cpp
const Dictionary const_dict;
auto value = const_dict["key"];  // Calls the const version of operator[]
```

Question: Does this problem exist in Nico?

First, there is no plan to support C++-style references.
So instead, we will see if this problem exists with raw pointers.

Nico does not allow you to create a mutable pointer to an immutable instance, so the following code is not allowed:
```
let num = 5
let ptr = var@num  // Error: cannot create a mutable pointer to an immutable instance
```

You also cannot convert an immutable pointer to a mutable pointer, so the following code is also not allowed:
```
let num = 5
let ptr = @num  // Immutable pointer to num
let mutable_ptr: var@i32 = ptr  // Error: cannot convert an immutable pointer to a mutable pointer
```

The same rules apply inside of structs:
```
struct MyStruct:
    field data: Data

    method get_data(self: @MyStruct) -> var@Data:
        return var@(self.data)  // Error: cannot create a mutable pointer to an immutable instance
```

Here, `data` is an immutable field of `MyStruct`, so we cannot create a mutable pointer to it.

Now, let's consider the case where `data` is declared with `var`:
```
struct MyStruct:
    field var data: Data

    method get_data(self: @MyStruct) -> var@Data:
        return var@(self.data)  // Error: cannot create a mutable pointer since self is not mutable.
```

According to the rules of Nico assignability, since `data` is declared with `var`, it inherits the mutability of the enclosing instance, which is the dereferenced `self` pointer. 
The assignability of the dereferenced `self` pointer depends on whether the pointer was a `var@` or a `@` pointer.
Since the receiver parameter is declared as `@MyStruct`, it is an immutable pointer, so the dereferenced instance is also immutable.
This means that `data` is also immutable, and we cannot create a mutable pointer to it.

A possible workaround would be to declare `data` with `mut`, which allows it to be mutable regardless of the mutability of the enclosing instance:
```
struct MyStruct:
    field mut data: Data
```
However, this is not ideal since it leaves our data unprotected and allows it to be modified even when the enclosing instance is immutable.

Storing `data` as a `var@` has the same problem, since anyone with a pointer to the instance can read `data` and dereference it to get a mutable pointer to it, even if the enclosing instance is immutable.
```
struct MyStruct:
    field data: var@Data

    method get_data(self: @MyStruct) -> var@Data:
        return self.data  // OK, interior mutability allows this.
```

...

## Problems with raw pointers

As explained in the previous section, one of the allowed types for the receiver parameter is a raw pointer to the instance.

Pointers are useful for passing large structs to methods without the overhead of copying them.
However, *raw* pointers are difficult to work with.
The language requires that raw pointers be dereferenced only in unsafe contexts:
```
method add(self: @MyStruct, b: i32) -> i32 {
    unsafe {
        return self.a + b
    }
}
```

Having the user constantly write `unsafe` blocks is cumbersome and error-prone.
The more `unsafe` blocks you have, the less meaningful they become.

The alternative would be to use references, a safer form of pointer that are guaranteed to be valid and not dangling through the management of ownership and lifetimes.
However, references are still in development and are complicated to implement.
It would be nice to have a "middle ground" between raw pointers and references.

There are a few special properties about the receiver parameter that make this middle ground possible:
- The receiver parameter is a parameter; it is rarely necessary for the user to change the values of parameters inside their function.
- The receiver parameter is always passed by the caller; it is never created inside the method.
- It is highly unusual for a method to deallocate the memory of the instance upon which it is called.
- If the method is called on a pointer, then the pointer is dereferenced at the call site; once inside the method, the instance is guaranteed to be valid and not dangling.

With these properties in mind, we can consider a special kind of raw pointer whose address value cannot be changed and whose memory cannot be deallocated.
This special pointer would be used exclusively for the receiver parameter in methods.
These restrictions are highly impractical outside methods, but methods offer special conditions that make them more useful.
