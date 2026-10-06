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
