#ifndef NICO_TOKEN_H
#define NICO_TOKEN_H

#include <any>
#include <string>
#include <string_view>

#include "../common/code_file.h"

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

    LParen,
    RParen,
    LBrace,
    RBrace,
    LSquare,
    RSquare,

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
    ColonColon,

    Identifier,

    // Literals

    Int,
    Float,
    Bool,
    Str,

    // Keywords

    KwAnd,
    KwOr,
    KwNot,
    KwBlock,
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

    KwPrint, // Temporary print keyword for development.
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
    // The line number of the token.
    size_t line;

    /**
     * @brief Constructs a new Location object.
     * @param file A pointer to the code file.
     * @param start The start index of the token.
     * @param length The length of the token.
     */
    Location(std::shared_ptr<CodeFile> file, size_t start, size_t length, size_t line)
        : file(file), start(start), length(length), line(line) {}
};

/**
 * @brief A token scanned from the source code.
 */
class Token {
public:
    // The type of this token.
    Tok tok_type;
    // The location of this token.
    const Location location;
    // A string view of the lexeme of this token.
    const std::string_view lexeme;

    /**
     * @brief Constructs a new Token object.
     * @param tok_type The type of the token.
     * @param location The location of the token.
     */
    Token(Tok tok_type, const Location& location)
        : tok_type(tok_type),
          location(location),
          lexeme(
              std::string_view(location.file->src_code).substr(location.start, location.length)
          ) {}
};

#endif // NICO_TOKEN_H
