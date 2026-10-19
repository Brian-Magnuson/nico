# Building LLVM for Nico

This guide will walk you through the process of installing LLVM for use with the Nico compiler.

## Introduction

The Nico compiler relies heavily on the LLVM framework for code generation and optimization.
To properly build and run Nico, you need to have LLVM installed on your system.

Unfortunately, installing LLVM can be a complex process depending on your operating system.

Installing LLVM on Windows is especially challenging for a few reasons:

- No MinGW‑compatible prebuilt binaries: LLVM does not distribute Windows binaries built with MinGW, which makes it difficult to integrate with a MinGW‑based toolchain without building LLVM from source.
- vcpkg is a possible cross‑platform option, but with major caveats
  - Adopting vcpkg would require committing to it as the dependency manager across all platforms.
  - LLVM packages in vcpkg are not always up‑to‑date with the latest releases.
  - Installing LLVM through vcpkg may trigger a full source build, which is slow and resource‑intensive.
- MSVC is difficult to use for building LLVM
  - MSVC tends to take a long time and a lot of resources to build LLVM, which can be a problem for developers with limited hardware resources.
  - MSVC also uses a proprietary toolchain and licensing model, which may not be compatible with all developers' needs or preferences.
- MSYS2 provides LLVM, but only the latest versions
  - MSYS2 is Windows‑only; even if it works on Windows, other platforms will require a different, equivalent solution.
  - MSYS2 typically ships only the newest LLVM release, which may not match the version required by the project.
  - LLVM major versions are generally not ABI‑compatible, so relying on “latest available” would force frequent updates to the project’s LLVM integration.
  - Package managers on other systems may not provide matching versions, making consistent cross‑platform support difficult.

To keep things simple, we adopt the following approach:

- We select a specific LLVM version that is known to work with Nico and is commonly available across platforms.
  - If this version is available in your system's package manager, you can install it directly.
    - The package is usually named with the major version number. E.g., for LLVM 20, the package may be called `llvm-20` or `llvm20`.
  - Otherwise, you can build it from source using the instructions provided below.

## Disclaimers

- This guide is for installing LLVM version 20. Nico may be using a different version, so please check the `CMakeLists.txt` file in the root of the Nico repository to confirm the required LLVM version. The steps are expected to be similar for other versions, so make sure to adjust the version numbers accordingly.
- LLVM has its own guide for building from source, which is comprehensive and detailed. This guide is meant to provide a simplified overview of the process, but you may need to refer to the official LLVM documentation for more complex scenarios or troubleshooting.
- Building LLVM from source can be resource-intensive and time-consuming. Keep this in mind as some settings can be adjusted to optimize for speed or resource usage.

## Prerequisites

The prerequisites for building LLVM are listed in the official LLVM documentation, which you can read here: [LLVM Documentation](https://llvm.org/docs/GettingStarted.html#requirements).

If you are using Windows, building LLVM from source should also work for MSYS2, which is available for installation here: [MSYS2](https://www.msys2.org/).
We recommend using MSYS2 for building LLVM on Windows, as it provides a Unix-like environment and package management system that simplifies the installation of dependencies.

The key software prerequisites are listed below:

- Git
- CMake - version 3.20 or later
- python - version 3.8 or later
- GNU Make (or any other build tool supported by CMake)
- A C++ compiler that supports C++17 (though C++20 is recommended since our project allows C++20 features). We recommend using Clang since we use it for building Nico.

Though it is not required for building LLVM, Windows users may be interested in installing `compiler-rt` as it is used later when configuring your local environment for building Nico. You can install it using the MSYS2 package manager.

When using MSYS2 on windows, you may want to run commands from the MSYS2 shell instead of the Windows command prompt or PowerShell, as it provides a Unix-like environment that is more compatible with the build process.
If you want, you can update your Path environment variable to include the MSYS2 binaries, which will allow you to run MSYS2 commands from any terminal.
For example, if you are using the UCRT64 environment, you can add the following to your Path:
```
C:\msys64\ucrt64\bin
```

## Building LLVM

Choose a location on your system to clone the LLVM repository.

Start by cloning the LLVM project repository from GitHub.
```bash
git clone --depth 1 --branch llvmorg-20.1.8 https://github.com/llvm/llvm-project.git llvm-project-20
```
We rename the directory to `llvm-project-20` to indicate the version of LLVM we are using. You can use a different name if you prefer.

If on Windows, you may want to disable automatic line ending conversion:
```powershell
git clone --config core.autocrlf=false --depth 1 --branch llvmorg-20.1.8 https://github.com/llvm/llvm-project.git llvm-project-20
```

Enter the newly created directory:
```bash
cd llvm-project-20
```

You may want to filter out user branches for when you want to use `git fetch` or `git pull` later:
```bash
git config --add remote.origin.fetch '^refs/heads/users/*'
git config --add remote.origin.fetch '^refs/heads/revert-*'
```

Configure the project with CMake.

Here is an example command that uses "Unix Makefiles" as the generator for Unix-like systems:
```bash
cmake -S llvm -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DLLVM_INCLUDE_BENCHMARKS=OFF
```

If you are using MSYS2 with the UCRT64 environment on Windows, you can use the following command to configure the project:
```powershell
cmake -S llvm -B build -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DLLVM_DEFAULT_TARGET_TRIPLE=x86_64-w64-windows-gnu -DLLVM_INCLUDE_BENCHMARKS=OFF
```

We have to specify the default target triple because, when building LLVM, it has a tendency to select `x86_64-pc-msys` as the default target triple, which is not compatible with the MinGW toolchain. 
By explicitly setting the target triple to `x86_64-w64-windows-gnu`, we ensure that LLVM is built with the correct target for our MinGW environment.

We also disable benchmarks because they rely on third-party code that tends to produce build errors with later versions of Clang, and they are not necessary for our purposes.

For the build target, you can choose `check-llvm` to build LLVM and run tests, or `all` to simply build LLVM without running tests.
Build tools usually offer an option to specify the number of parallel jobs to speed up the build process.
LLVM can take a long time to build, so it is recommended to increase the number of parallel jobs to utilize multiple CPU cores.

Here is an example command to build LLVM using CMake with 8 parallel jobs:
```bash
cmake --build build --target all --parallel 8
```

Wait patiently for the build process to complete.

## Configuring Nico to Use the Built LLVM

Once LLVM is built, you may need to configure the Nico project to use the built LLVM instead of any system-installed version.
To do this, we use CMake presets, which allow us to define a set of configuration options that can be easily reused.

You can see the available presets in `CMakePresets.json` in the root of the Nico project.
Some of the presets may require additional configuration. 
In these cases, you can create a `CMakeUserPresets.json` file in the root of the project to override or extend the existing presets.
`CMakeUserPresets.json` is ignored by Git, so you can safely add your own presets without affecting other developers.

Create a file in the root of the project named `CMakeUserPresets.json` with the following content:
```json
{
  "version": 8,
  "configurePresets": [
    {
      "name": "local"
    }
  ],
  "buildPresets": [
    {
      "name": "local",
      "configurePreset": "local"
    }
  ]
}
```

Here, we add a new configure preset named `local` to represent your local configuration, though you can name it whatever you like.

You may be interested in adding a `displayName` field to both presets to make them easier to identify in your IDE:
```
"displayName": "Local Configuration"
```

You may also be interested in adding an `inherits` field to the build preset to inherit from an existing preset, such as `windows-msys2-debug`:
```
"inherits": "windows-msys2-debug"
```

For your build preset, you may be interested in specifying the number of parallel jobs to use when building, which can be done by adding a `jobs` field:
```
"jobs": 8
```
The number of parallel jobs can be adjusted based on your system's capabilities. 
It is generally limited by the number of CPU cores available, so you may want to set it to a value that is appropriate for your system.

For your configure preset, you can set various CMake cache variables to configure the build process.
For example, if you want to use a different compiler, you can set the `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER` variables:
```
"cacheVariables": {
  "CMAKE_C_COMPILER": "/path/to/clang",
  "CMAKE_CXX_COMPILER": "/path/to/clang++"
}
```

The `LLVM_DIR` variable should point to the directory containing the `LLVMConfig.cmake` file, which is typically located in the `lib/cmake/llvm` subdirectory of your LLVM build directory.
```
"cacheVariables": {
  "LLVM_DIR": "/path/to/llvm-project-20/build/lib/cmake/llvm"
}
```

When using MSYS2 on Windows, the JIT tends to have issues finding certain built-in symbols, which can lead to runtime errors when running Nico.
These issues can generally be resolved by indicating the location of the `compiler-rt` library, which provides the necessary built-in symbols for the JIT.
If `compiler-rt` is not installed, you should be able to install it using the MSYS2 package manager.

The exact library we want is named something like `libclang_rt.builtins-<arch>.a` where `<arch>` is the architecture of your system, such as `x86_64`.

Specify the location of the `compiler-rt` library in the `CMakeUserPresets.json` file by adding the `NICO_CXX_COMPILER_RT_LIB_PATH` variable under the `cacheVariables` field:
```
"NICO_CXX_COMPILER_RT_LIB_PATH": "C:/msys64/ucrt64/lib/clang/22/lib/windows/libclang_rt.builtins-x86_64.a"
```
This is just an example and your path may be different.
Also note that this path uses Clang version 22, which is different from the version of LLVM we are using (20).
This is because your version of Clang may be different from the version of LLVM you are using, which is generally fine as long as the library is compatible with your system and the version of LLVM you are using.

You can use the MSYS2 shell to locate the library by running a command like:
```bash
find /ucrt64/lib/clang -name 'libclang_rt.builtins-*.a'
```

Here is an example of a `CMakeUserPresets.json` file that includes the `LLVM_DIR` and `NICO_CXX_COMPILER_RT_LIB_PATH` variables:
```json
{
  "version": 8,
  "configurePresets": [
    {
      "name": "local",
      "inherits": "windows-msys2-debug",
      "displayName": "Local Configuration",
      "cacheVariables": {
        "LLVM_DIR": "C:/llvm-project-20/build/lib/cmake/llvm",
        "NICO_CXX_COMPILER_RT_LIB_PATH": "C:/msys64/ucrt64/lib/clang/22/lib/windows/libclang_rt.builtins-x86_64.a"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "local",
      "inherits": "windows-msys2-debug",
      "configurePreset": "local",
      "displayName": "Local Build",
      "jobs": 16
    }
  ]
}
```

Your IDE may provide a way to select the CMake preset to use when configuring the project.
Otherwise, you can use the command line to configure the project with the `local` preset:
```bash
cmake --preset local
```

You can then build the project using the `local` build preset:
```bash
cmake --build --preset local
```
