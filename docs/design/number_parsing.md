# Number Parsing

This document explains how number parsing will be implemented in the Nico programming language.
Number parsing is one of the most basic functionalities of any programming language, and it is essential for handling numerical data effectively.

> [!WARNING]
> This document is a work in progress and may be subject to change.

## Introduction

Number parsing, despite the name, is a process that begins in the lexer phase of the compiler. The lexer is responsible for recognizing number literals in the source code and converting them into tokens that can be processed by the parser.
At a minimum, the lexer must check to make sure the number is in the expected format (e.g., correct prefixes, use of exponents and decimal points, etc.). 

At some point during compilation, the string representation of the number must be converted into its actual numeric value (e.g., "37" must be converted to the integer value 37). This is necessary for the program to generate the correct LLVM IR code and ensure the number is within the valid range for its type.

This is easy to do for any positive integer that fits in 64 bits, which is the largest width we plan to support for integer types. However, as we will see in this document, number parsing becomes more complicated when we consider negative integers.

In this document, we will explain the number formats we intend to support, the challenges associated with parsing numbers, and our proposed solutions to these challenges.

## Supported Number Formats

We will begin by describing the number formats that Nico will support. We'll start with basic number formats, then expand on these to include more complex formats.

Please note that we will *not* consider negative signs as part of the number token. We will explain why later in this document.

### Integer Formats

First, we will support decimal integers, which are sequences of digits from 0 to 9. For example:
```
0          // Int
123        // Int
456789     // Int
A          // Not a number; scanned as an identifier instead.
```

We allow underscores in the middle or end of the number for readability:
```
1_000      // Int
12_345_678 // Int
1_______0  // Int
_1         // Not a number; scanned as an identifier instead.
```

We allow numbers to have prefixes to indicate different bases. Numbers without prefixes are always interpreted as decimal. The supported prefixes are:
- `0b` or `0B` for binary (base 2)
- `0o` or `0O` for octal (base 8)
- `0x` or `0X` for hexadecimal (base 16)

For hexadecimal numbers, we allow letters A-F and a-f to represent values 10-15.

Additionally, any number that has a base prefix is disqualified from becoming a floating-point number.

```
0b1010        // Int (binary)
0o755         // Int (octal)
0x1A3F        // Int (hexadecimal)
0xDEAD_BEEF   // Int (hexadecimal with underscores)
0XFF          // Int (hexadecimal)
0xBad_Cafe    // Int (hexadecimal with different letter cases)

0b2           // Not a number; will report an error.
0o8           // Not a number; will report an error.

017           // Int (decimal; leading zero does not imply octal)
```

These are all the integer formats we plan to support.

### Floating-Point Formats

For floating-point numbers, we will support the basic decimal format, which consists of a decimal point and decimal digits on both sides of the decimal point.
```
0.0          // Float
12.34        // Float
0.123456     // Float
.1           // Not a number; scanned as a dot and size instead.
1.           // Not a number; will report an error.
```

Like integers, we allow underscores in the middle or end of the number for readability:
```
1_000.000_1   // Float
12_345.67_89  // Float
_0.1_         // Not a number; scanned as an ident, dot, and size instead.
```

We also support scientific notation, which consists of an exponent part following the decimal part. The exponent part starts with 'e' or 'E', followed by an optional '+' or '-' sign, and then a sequence of decimal digits.
```
1.23e4       // Float (1.23 * 10^4)
0.1E-2       // Float (0.1 * 10^-2)
3.14e+10     // Float (3.14 * 10^10)
```

These are all the floating-point formats we plan to support.

### Special Floating-Point Values

We will also support special floating-point values: positive infinity, negative infinity, and NaN (Not a Number). These values will be represented as follows:
```
inf        // Float (positive infinity)
-inf       // Float (negative infinity)
nan        // Float (Not a Number)
```

### Additional Restrictions

Finally, we will impose some additional restrictions on number formats:

- A number cannot be immediately followed by an alphabetic character. 
  - This is mainly to prevent cases where the user expects a letter to be part of the number.
  - For example, `123abc` would normally be interpreted as a number followed by an identifier, but we can't be sure if the user meant to write a hexadecimal number and forgot the `0x` prefix.

- A floating-point number cannot be immediately followed by another dot. 
  - This error would eventually be caught by the type checker as the dot operator cannot be applied to a float, but we can catch it earlier during lexing.
  - For example, `1.2.3` is invalid because the second dot cannot be part of the floating-point number.

- A floating-point number cannot have a base prefix. This is to avoid confusion and maintain consistency in number formats.
  - For example `0xab.cd` is invalid because it is unclear how this would be interpreted.

- A floating-point number cannot have multiple exponent parts.
  - For example, `1.2e3e4` is invalid because it contains two exponent parts.

## Type Suffixes

When creating an integer of a specific size, some programming languages use a pattern like this:
```
let x: i8 = 42
```

However, this means that the type of the number literal `42` is context-dependent. In this case, the type of `42` is inferred to be `i8` because of the type annotation on `x`. This is difficult to achieve because the type checker would have to examine multiple nodes in the AST to determine the type of a single number literal.

It is best if the type of a number literal can be determined solely by examining the number literal itself. To achieve this, we will use type suffixes that can be appended to number literals to indicate their types explicitly.

A type suffix is simply the intended type of the number literal written immediately after the number, without any spaces. The supported type suffixes are as follows:
- For integers: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`
- For floating-point numbers: `f32`, `f64`

```
42i8        // 8-bit signed integer
42_i8       // Same as above with an underscore for readability
255u8       // 8-bit unsigned integer
3.14f32     // 32-bit floating-point number
```

Integers that do not have a type suffix will default to `i32`.
Floating-point numbers that do not have a type suffix will default to `f64`.

Additionally, we may support special type suffixes for convenience:
- `l` or `L` for `i64`
- `u` or `U` for `u32`
- `ul` or `UL` for `u64`
- `f` or `F` for `f64`

## Challenges in Number Parsing

While parsing positive integers and floating-point numbers is relatively straightforward, handling negative integers presents several challenges.

Negative numbers are generally written like this: `-42`. However, the negative sign (`-`) is not considered part of the number token itself. Instead, it is treated as a separate unary operator that negates the value of the number.

This is because the lexer cannot distinguish between unary minus and binary minus (subtraction) at the time of scanning.

Consider the following example:
```
a - 5
```
In this case, the `-` symbol is a binary operator that subtracts 5 from the value of `a`. If the lexer were to treat `-5` as a single number token, it would not be able to correctly parse expressions like this.

Because of this, the actual number token will not include any negative signs. That is, the number token is always a positive number.

This is usually not a problem as almost every negative number can be represented as the negation of its positive counterpart. For example, `-42` can be represented as `-(42)`.

However, for each signed integer, there is one exception: its minimum value. For example, an `i8` can represent values from -128 to 127. The minimum value, -128, cannot be represented as the negation of a positive `i8`, because there is no positive `i8` value of 128.

So if the user tries to write `-128_i8`, the lexer will scan `128_i8` as a positive number token. If we do not parse this correctly, `-128_i8` may be interpreted as out-of-range despite being a perfectly valid value for an `i8`.

Because of this edge case, number parsing cannot be handled by the lexer alone. The parser and possibly the type checker must also be involved to ensure that negative numbers are handled correctly.

## Solution 1: Two-Parse Strategy

The two-parse strategy works like this:

1. During scanning...
   1. The lexer scans integers and parses them into `uint64_t` values using `std::from_chars`, regardless of the suffix.
   2. The literal value is then converted into a base-36 string using `std::to_chars` and stored in the token (base 36 is the largest base supported by `std::to_chars`).
   3. The token is assigned a type according to its suffix (or default type if no suffix is present).
2. During parsing...
   1. When the parser creates a unary minus node, it checks if the inner expression is a number literal.
   2. If it is, the parser prepends the base-36 string with a negative sign (`-`) to form the full string representation of the negative number, and then returns a literal node instead of a unary minus node.
3. During type checking...
   1. When the type checker visits a number literal node, it parses the base-36 string again using `std::from_chars`, this time taking into account the type of the number (including signedness).
   2. If the number is out of range for its type, the type checker reports an error.
   3. Otherwise, the type checker replaces the base-36 string with the actual numeric value in the token.

The number is parsed twice: once during scanning and again during type checking. The second parse is where the "true" value of the number is determined, taking into account its type and signedness.

Performing the initial parse has a few benefits:
- It allows the lexer to catch format errors early, such as invalid digits for a given base.
- It "cleans" the number, converting it to a standard representation that is more compact than the original lexeme.
- It allows the parser to attach a negative sign to the number without worrying about format errors.

However, this strategy has some downsides:
- It requires parsing the number twice, which may have performance implications for very large numbers of literals.
- Although base-36 number strings are more compact than the original lexemes, they may still be larger than the original numeric value, leading to increased memory usage.

## Solution 2: uint64_t with Sign Flag

We can store any number in the range of supported types as a `uint64_t`. The `uint64_t` does not have signedness. To explain why this is an issue, we will use a smaller example with `uint16_t`.

Suppose the user writes `-32768_i16`. The lexer scans `32768_i16` and stores the value `32768` as a `uint16_t`. 
Internally, this is represented as `0b1000 0000 0000 0000`.
If the parser tries to apply the unary minus operator by "negating" this value, then, using 2's complement arithmetic, the result would be `0b1000 0000 0000 0000`, which is still `32768` when interpreted as a `uint16_t`.

This means that `32768_i16` (without a negative sign) and `-32768_i16` would both be represented by the same internal value, even though the latter is valid and the former is out of range for `i16`. Without the negative sign, we lose the ability to distinguish between these two cases.

This solution proposes storing an additional sign flag alongside the `uint64_t` value in the token. The sign flag indicates whether the number has a negative sign applied to it.

Tokens can only store one literal value in an std::any, so we instead use an `std::pair<uint64_t, bool>`, where the `uint64_t` is the absolute value of the number, and the `bool` is the sign flag (true for negative, false for positive).

1. During scanning...
   1. The lexer scans integers and parses them into `uint64_t` values using `std::from_chars`, regardless of the suffix.
   2. The token stores an `std::pair<uint64_t, bool>`, where the `uint64_t` is the parsed value, and the `bool` is initialized to false (indicating a positive number).
   3. The token is assigned a type according to its suffix (or default type if no suffix is present).
2. During parsing...
   1. When the parser creates a unary minus node, it checks if the inner expression is a number literal.
   2. If it is, the parser sets the sign flag in the token to true (indicating a negative number) and returns the number literal node instead of a unary minus node.
3. During type checking...
   1. When the type checker visits a number literal node, it retrieves the `std::pair<uint64_t, bool>` from the token.
   2. It checks the sign flag to determine if the number is negative.
   3. It then checks if the absolute value is within the valid range for its type, taking into account the sign.
   4. If the number is out of range for its type, the type checker reports an error.
   5. Otherwise, the type checker computes the final numeric value (applying the sign if necessary) and stores it in the token.

This solution avoids the need to parse the number twice. It also means that for every integer literal, at least 9 extra bytes are used (8 bytes for the `uint64_t` and 1 byte for the `bool`), which may increase memory usage. Ironically, FP numbers will be more memory-efficient than integers in this scheme, as they can be stored directly as `float` or `double` values without any additional flags.

Compared to the two-parse strategy, this solution is only more space efficient for integer literals larger than $\text{ZZZZZZZZZ}_{36}$ or 101559956668415 in decimal.

## Solution 3: Sized Integer Types with Sign Flag

This solution is an optimization of Solution 2. Instead of storing all integer literals as `uint64_t`, we can store them in their smallest possible sized type based on their suffix (or default type if no suffix is present).

For `i8` and `u8`, we can use `uint8_t`.
For `i16` and `u16`, we can use `uint16_t`.
For `i32` and `u32`, we can use `uint32_t`.
For `i64` and `u64`, we can use `uint64_t`.

This solution introduces slightly more complexity in the lexer, as it must determine the appropriate sized type for each integer literal. However, it reduces memory usage for smaller integer literals.

## Solution 4: Single-Byte Metadata

This solution recognizes that, in addition to the lexeme, 3 things are needed to parse a number correctly:
1. The type suffix (i8, i16, i32, i64, u8, u16, u32, u64)
2. The base (2, 8, 10, or 16)
3. The sign (positive or negative)

The first item is handled by the token type. The second and third items can be encoded into 8-bit signed integer of metadata stored in the token's literal slot.
- If the 8-bit integer is positive, the sign is positive; if negative, the sign is negative.
- The absolute value of the 8-bit integer encodes the base:
  - 2 for binary
  - 8 for octal
  - 10 for decimal
  - 16 for hexadecimal

In this solution, the lexer does not parse the number at all. It only checks if it is in a valid format and stores the base in the `std::any` literal slot as an `int8_t`.
The lexeme is still accessible through the token's `lexeme` field, which is stored as an `std::string_view`, just like any other token.

1. During scanning...
   1. The lexer scans integers and checks if they are in a valid format.
   2. The lexer stores an `int8_t` in the token's literal slot, encoding the base and sign as described above.
   3. The token is assigned a type according to its suffix (or default type if no suffix is present).
2. During parsing...
   1. When the parser creates a unary minus node, it checks if the inner expression is a number literal.
   2. If it is, the parser updates the `int8_t` in the token's literal slot to indicate a negative sign and returns the number literal node instead of a unary minus node.
3. During type checking...
   1. When the type checker visits a number literal node, it retrieves the `int8_t` from the token's literal slot.
   2. It checks the sign and base encoded in the `int8_t`.
   3. It then parses the lexeme using `std::from_chars`, taking into account the base and sign.
   4. If the number is out of range for its type, the type checker reports an error.
   5. Otherwise, the type checker stores the actual numeric value in the token, replacing the `int8_t` metadata.

This solution has even lower memory usage than Solution 3. After type checking, each token will only use as many bytes as the actual numeric value requires (e.g., 1 byte for `i8`, 2 bytes for `i16`, etc.), plus the additional memory that comes with all tokens (e.g., lexeme storage, token type, etc.).

Additionally, formatting checks can still be caught early during scanning. Range checking is deferred to the type checker, where it has been done in previous solutions.

# Solution 5: Integer Minimum Requires Negative Token

This solution is designed to treat the minimum value of each signed integer type as a special case that requires the presence of a unary minus token.

For example, for `i8`, `-128` is the only negative value that has no positive counterpart.
Therefore, when the parser sees `128`, it will only accept it as a valid `i8` literal if it is preceded by a unary minus token. Otherwise, it will be treated as an out-of-range error.

Tokens do not indicate if they are a unary or binary operator, so we introduce a new token type just for this case: `Tok::Negative`. During parsing, the parser will change the `-` token's type from `Tok::Minus` to `Tok::Negative` when it creates a unary minus node.

1. During scanning...
   1. The lexer checks if the number is signed.
      1. If it is, the lexer parses the number and tests if it is in the range of [0, max_value_of_type + 1] ([0, 128] for `i8`).
      2. If it is not, the lexer rejects the number as out of range.
   2. The token is assigned a type according to its suffix (or default type if no suffix is present).
2. During parsing...
   1. When the parser creates a unary minus node, it changes the `-` token's type from `Tok::Minus` to `Tok::Negative`.
   2. When the parser sees a literal number with the value equal to the max value of its type + 1 (e.g., 128 for `i8`), it checks if the preceding token is of type `Tok::Negative`.
      1. If `Tok::Negative` is not present, the parser reports an out-of-range error.

The type checker does not need to do any additional work for number literals, as all range checking has already been handled by the lexer and parser.

The parser does not "apply" the negative sign to the number literal. It only checks if it is there. This works because of a trick with 2's complement arithmetic: negating the minimum value of a signed integer type results in the same bit pattern.
```
"128" (rejected) -> error: out of range for i8
"127" (lit 127) -> 127
"1" (lit 1) -> 1
"-1" (unary - (lit 1)) -> -1
"-127" (unary - (lit 127)) -> -127
"-128" (unary - (lit -128)) -> -128
```

As shown above, the lexer will store `128` as `-128`, but applying the negative sign will still yield the correct result.

An important part of this solution is that, when checking for `Tok::Negative`, the parser checks the previous token only. This means that the parser will accept `-128_i8` and `- 128_i8` but reject `-(128_i8)` because the token before the `128_i8` literal is `Tok::RParen`, not a `Tok::Negative`.

This solution has a few advantages:
- The memory usage is minimal, as no additional data needs to be stored in the token.
- The number is only parsed once, during lexing.
- Format errors can be caught early during lexing.
- The type checker does not need to do any additional work for number literals.
