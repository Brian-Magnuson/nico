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

