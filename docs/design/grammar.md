# Grammar

The following provides the grammar for the Nico programming language. The grammar is written in a variant of Extended Backus-Naur Form (EBNF) and is intended to be read by humans.

> [!WARNING]
> This document is a work in progress and is subject to change.

## Key

Terminals are written in quotes or in all capital letters. Non-terminals are written in lowercase.
```
"terminal"
TERMINAL
non_terminal
```

A rule consists of a non-terminal followed by a `::=` symbol and a sequence of terminals and non-terminals. The rule ends with a semicolon `;`.

Here is an example rule:
```
rule ::= "terminal" non_terminal ;
```

A rule may have multiple alternatives separated by a vertical bar `|`. Parentheses may be used to group alternatives. Here is an example rule with multiple alternatives:
```
rule_1 ::= "1" | "2" | "3" ;
rule_2 ::= ("1" | "2") "0" ;
```

A question mark, `?`, indicates that the preceding element is optional. An asterisk, `*`, indicates that the preceding element may be repeated zero or more times. A plus sign, `+`, indicates that the preceding element may be repeated one or more times.
```
rule ::= "a"? "b"* "c"+ ;
```

# Lexical grammar

The lexical grammar is used by the lexer to tokenize the input source code. It consists of rules for individual tokens such as identifiers, strings, and numbers.

The lexer also scans for comments, but these are not included in this document.

The lexer also scans keywords, but for simplicity, these are written as literals in the syntax grammar.

## Single characters

```
ALPHA ::= "A"..."Z" | "a"..."z" | "_" ;
DIGIT ::= "0"..."9" ;
HEX_DIGIT ::= DIGIT | "A"..."F" | "a"..."f" ;
OCT_DIGIT ::= "0"..."7" ;
```

## Identifiers

```
IDENTIFIER ::= ALPHA (ALPHA | DIGIT)* ;
```

## Strings and string-related tokens

```
ESC_CHAR ::= "\\" ("0" | "n" | "r" | "t" | "b" | "f" | "\"" | "'" | "\\") ;

CHARACTER ::= "'" (ESC_CHAR | <any char except "'">) "'" ;
STRING ::= "\"" (ESC_CHAR | <any char except "\"">)* "\"" ;
```

## Numbers

```
INTEGER ::= DIGIT+ 
        | "0x" HEX_DIGIT+ 
        | "0b" ("0" | "1")+ 
        | "0o" OCT_DIGIT+ ;

FLOAT ::= DIGIT+ "." DIGIT+ (("E" | "e") ("+" | "-")? DIGIT+)? ;
```

## Indentation

The rules for indentation are special. They cannot be formally defined in EBNF, but they are described here.

```
INDENT ::= ":" (CR)? LF <greater left spacing than previous line, except between grouping tokens> ;
DEDENT ::= (CR)? LF <less or equal left spacing to previous indent, except between grouping tokens> ;
```

# Syntax grammar

The syntax grammar is used by the parser to parse the tokens produced by the lexer. It consists of rules for the structure of the language such as expressions, statements, and declarations.

## Program

```
program ::= statement* EOF ;

statement ::= struct_decl
        | func_decl
        | let_decl
        | return_stmt
        | yield_stmt
        | break_stmt
        | continue_stmt
        | expression
```

## Declarations

### Body
```
body ::= (INDENT statement* DEDENT) | (L_BRACE statement* R_BRACE) ;
```

### Let declaration

```
let_decl ::= "let" ("var")? IDENTIFIER (":" type)? ("=" expression)? ;

type ::= ("&" | "var&" | "*" | "var*")? IDENTIFIER ;
```

### Function declaration

```
func_decl ::= "func" IDENTIFIER "(" parameters? ")" ("->" type)? block ;
) ;

parameters ::= IDENTIFIER ("," IDENTIFIER)* ;
```

### Struct declaration

```
struct_decl ::= ("struct" | "class") IDENTIFIER body ;
```

## Expressions

```
expression ::= block_expr
        | if_expr
        | loop_expr
        | while_expr
        | assignment

block ::= (INDENT statement* DEDENT) | (L_BRACE statement* R_BRACE) ;

block_expr ::= "block" block ;
if_expr ::= "if" expression ( "then" expression | block ) ( "else" ( expression | block ))? ;
loop_expr ::= "loop" block ;
while_expr ::= "while" expression block ;

assignment ::= logical_or ("=" assignment)? ;
logical_or ::= logical_and ("or" logical_and)* ;
logical_and ::= equality ("and" equality)* ;
equality ::= comparison (("==" | "!=") comparison)* ;
comparison ::= term (("<" | ">" | "<=" | ">=") term)* ;
term ::= factor (("+" | "-") factor)* ;
factor ::= unary (("*" | "/" | "%") unary)* ;
unary ::= postfix | ("+" | "-" | "not") unary ;
postfix ::= primary ("." IDENTIFIER | "(" arguments? ")" | "[" expression "]")* ;

primary ::= IDENTIFIER | literal | grouping | tuple | array;
grouping ::= "(" expression ")" ;
tuple ::= "(" ( expression "," expression_list? )? ")" ;
array ::= "[" expression_list? "]" ;

arguments ::= expression ("," expression)* ;
expression_list ::= expression ("," expression)* ;
literal ::= INTEGER | FLOAT | STRING | CHARACTER | "true" | "false";
```


