# Command Line Interface

This document describes the potential command line interface (CLI) for the project. The CLI will provide users with a way to interact with the application through terminal commands, allowing for automation and scripting capabilities.

> [!WARNING]
> This document is a work in progress and is subject to change.

> [!WARNING]
> This document is for ideation and design purposes. It is not meant to be a reference for users.

## Introduction

Virtually every compiler and build system has a command line interface. 
The CLI is a critical component of the user experience, as it allows users to interact with the application in a flexible and powerful way.

Designing a CLI for Nico can be tricky as it is essentially a *3-in-1* compiler. It is an AOT compiler, a JIT compiler, and a REPL. 
Furthermore, we may want to expand this in the future to include a debugger, build system, and other tools. 
To reference our coding principles: Keep it simple, but leave things open for future extension. 
Sometimes, the simplest solution can make things much harder to extend later.
This document will explore the design of the CLI, including the commands, options, and arguments that will be available to users.

Note that, although we will cover how to invoke the REPL, we will not cover the design of the REPL itself in this document.
The REPL is a separate component that will have its own design.

## CLIs for Existing Compilers

First, we will look at the CLIs for existing compilers to see what works well and what doesn't.

### GCC (C/C++)

First, GCC is not a single compiler, but rather a collection of compilers for different languages (the name literally stands for "GNU Compiler Collection").
Here, we will use C files as an example, but the same principles apply to other languages.

Compiling a file is as simple as specifying the file name:
```bash
gcc hello.c
```
This usually produces an executable file called `a.out`. 

The user can specify the output file name with the `-o` option:
```bash
gcc hello.c -o hello
```
This produces an executable file called `hello`.

The above examples actually invoke the compiler and the linker.
This way, the user doesn't have to deal with linking multiple object files and can go straight to the executable.

Adding the `-c` option tells `gcc` to stop before the linking stage, producing only an object file:
```bash
gcc -c hello.c -o hello.o
```
This is useful when there are many source files that need to be compiled separately and then linked together.
Build tools like `make` can be used to automate this process.

Producing a library is actually a separate command, `ar`, which is used to create static libraries.
```bash
ar rcs libhello.a hello.o
```
Libraries of this form are prefixed with `lib` by convention.

The `-l` option is used to link against a library. If the library is not in a standard location, the `-L` option can be used to specify the library search path:
```bash
gcc main.c -L. -lhello -o main
```
Oddly, the `lib` prefix is omitted when specifying the library name with `-l`.

We give special attention to `gcc` because it is such a widely used compiler and has a well-known CLI.
Even though options like `-c`, `-o`, and `-l` are not the most intuitive to the uninitiated, they are well-established and widely used.
Although we may want to have more clearer options for our users, we should also consider the fact that many users will already be familiar with these options and may expect them to work in a similar way.

For example, many CLIs offer both a short and long form of options, such as `-o` and `--output`.
These will do the same thing, but the long form is more descriptive and easier to remember.

That said, we should be careful not to give *every* option a long form, as this can make the CLI more complex and harder to use.
Explicit is still better than implicit.


### Javac (Java)

The Java programming language works a little differently than C and C++.
Instead of being compiled directly to machine code, Java is compiled to an intermediate form called bytecode, which is then executed by the Java Virtual Machine (JVM).

To compile a Java file, the `javac` command is used:
```bash
javac Program.java
```
This produces a file called `Program.class`, which contains the bytecode for the program.

To output the compiled bytecode to a directory, the `-d` option can be used:
```bash
javac -d out Program.java
```

The `-cp` option can be used to specify the classpath, which is a list of directories and JAR files that contain classes that the program depends on:
```bash
javac -cp lib/* Program.java
```

Multiple compiled Java bytecode files can be combined into a Java Archive or JAR file using the `jar` command:
```bash
jar --create --file Program.jar Program.class
```
The `--create` option tells the `jar` command to create a new JAR file, and the `--file` option specifies the name of the JAR file to create.

To run the JVM with a compiled Java bytecode file or JAR file, the `java` command is used:
```bash
java Program
java -jar Program.jar
```

Since there is no executable, command line arguments are passed to the `java` command after the class name or JAR file name:
```bash
java Program arg1 arg2 arg3
```

Another aspect that makes Java different from C and C++ is that the JVM is more-or-less a Java bytecode interpreter.
Instead of compiling to machine code and producing an executable file, the JVM interprets the bytecode.

We could take some inspiration here given that Nico compiles to an intermediate representation.
Our JIT is like the JVM in the sense that the code is executed immediately instead of producing an executable file.
We could give our users the option to produce `.ll` files, which store the intermediate representation.

However, we also want to provide an AOT compiler, same as with C and C++.
This may involve having multiple *sub-commands*, which we use to distinguish between the different modes of operation.


### Python

The Python language, like Java, also compiles to an intermediate representation.
However, most users won't work with compiled Python bytecode files.
Rather, the Python interpreter is used to execute Python scripts straight from the source files.

```bash
python script.py
```

Python also lets you specify command line arguments to the script, which are passed to the script as a list of strings:
```bash
python script.py arg1 arg2 arg3
```

This simplicity is one of the reasons why Python is so popular, as it allows users to quickly run scripts without having to worry about compilation or linking.
You also do not need to specify multiple source files, as Python will automatically import any modules that are needed.

Another aspect of Python is that it has a REPL, which allows users to interactively execute Python code in a terminal session.
You can invoke the REPL by simply running the `python` command without any arguments:
```bash
python
```

Similar to Python, we want Nico be simple to use, allowing users to either specify a single script to run or to enter the REPL for interactive use.
This is part of the reason why we do not allow users to define a `main` function. Rather, one is defined implicitly based on the designated "start file" of the program.


## Rust

The last language we'll look at is Rust, which is a systems programming language that is designed to be safe, concurrent, and fast.
Rust can be thought of as a modern alternative to C and C++, with a focus on safety and performance.

To compile a Rust file, the `rustc` command is used:
```bash
rustc main.rs
```
You don't need to specify multiple files as Rust will automatically compile any modules that are needed.

To specify the output file name, the `-o` option can be used:
```bash
rustc main.rs -o main
```

The above commands create executables. To create a library instead, the `--crate-type` option can be used:
```bash
rustc main.rs --crate-type lib
```

Most Rust projects are built using the `cargo` command, which is a build system and package manager for Rust.
To build a Rust project, the `cargo build` command is used:
```bash
cargo build
```

To build and run a Rust project, the `cargo run` command is used:
```bash
cargo run
```

It is worth noting that Rust, like C and C++, requires users to define a `main` function when creating an executable.
In Nico, users do not define a `main` function, as one is defined implicitly based on the designated "start file" of the program.
As such, it may not be obvious which file is the "start file" of the program, especially if there are multiple source files.
However, Rust shows that it is possible to specify a single file for an AOT compiler, while also allowing for multiple source files to be compiled together.
We could take inspiration from this and have users write only the start file for the compiler, while also allowing for multiple source files to be compiled together.
