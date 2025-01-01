#ifndef NICO_TOKEN_H
#define NICO_TOKEN_H

#include <string>

#include "compiler/code_file.h"

/**
 * @brief Enum class for a token type.
 */
enum class Tok {
    // Base tokens

    Null,
    Eof,
    Unknown,

    // Ignored tokens

    SlashSlash,
    StarSlash,
    SlashStar,
    Backslash,
    SingleQuote,
    DoubleQuote,
    TripleQuote,

    // Whitespace

    Indent,
    Dedent,

    // Symbols

    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    LeftSquare,
    RightSquare,

    Comma,
    Semicolon,

    Plus,
    PlusEq,
    Minus,
    MinusEq,
    Star,
    StarEq,
    Slash,
    SlashEq,
    Percent,
    PercentEq,
    Caret,
    CaretEq,
    Amp,
    AmpEq,
    Bar,
    BarEq,
    Bang,

    BangEq,
    EqEq,
    Gt,
    GtEq,
    Lt,
    LtEq,

    Eq,
    Dot,
    Arrow,
    Colon,

    // Keywords

    KwAnd,
    KwOr,
    KwNot,
    KwIf,
    KwElse,
    KwElif,
    KwLoop,
    KwWhile,
    KwBreak,
    KwContinue,
    KwReturn,
    KwYield,

    KwLet,
    KwVar,
    KwConst,
    KwGlobal,
    KwFunc,
    KwStruct,
    KwClass,
    KwEnum,

    KwAs,
    KwIs,
    KwAlloc,
    KwDealloc,
};

/**
 * @brief Struct for the location of a token within a code file.
 */
struct Location {
    // The file where the token is located.
    std::shared_ptr<CodeFile> file;
    // The start index of the token.
    size_t start;
    // The length of the token.
    size_t length;
};

class Token {
public:
    const Tok tok_type;
    const std::string lexeme;
};

#endif // NICO_TOKEN_H
