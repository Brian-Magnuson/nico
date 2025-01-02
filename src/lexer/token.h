#ifndef NICO_TOKEN_H
#define NICO_TOKEN_H

#include <string>

#include "../compiler/code_file.h"

/**
 * @brief A token type.
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
 * @brief A location of a token within a code file.
 *
 * Includes a pointer to the code file containing the source code string.
 */
struct Location {
    // The file where the token is located.
    std::shared_ptr<CodeFile> file;
    // The start index of the token.
    size_t start;
    // The length of the token.
    size_t length;
};

/**
 * @brief A token scanned from the source code.
 */
class Token {
public:
    // The type of this token.
    const Tok tok_type;
    // The location of this token.
    const Location location;

    /**
     * @brief Gets the lexeme of the token.
     *
     * The lexeme is retrieved from the source code using the location of the token.
     *
     * @return A string representing the lexeme of the token.
     */
    std::string get_lexeme() const;
};

#endif // NICO_TOKEN_H
