# Error Recovery

In a compiler, error handling is crucial for providing meaningful feedback to developers and allowing the compilation process to continue even when errors are encountered. Error recovery is perhaps most difficult in the parsing phase, where the parser must be able to determine what tokens must be skipped to allow the parser to continue processing the remaining input. 

This document explains the concept of error recovery, giving special attention to the parsing phase, and describes the error recovery strategy used in the compiler.

> [!WARNING]
> This document is a work in progress and is subject to change

## Introduction

