#ifndef NICO_TOKEN_H
#define NICO_TOKEN_H

#include <any>
#include <string>
#include <string_view>
#include <utility>

#include "nico/shared/code_file.h"

namespace nico {

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
    Minus,
    Star,
    Slash,
    Percent,
    Negative,
    Bar,
    Bang,

    _CompoundOperatorsStart,
    PlusEq,
    MinusEq,
    StarEq,
    SlashEq,
    PercentEq,
    BarEq,
    _CompoundOperatorsEnd,

    BangEq,
    EqEq,

    _ComparisonsStart,
    Gt,
    GtEq,
    Lt,
    LtEq,
    _ComparisonsEnd,

    Eq,
    Dot,
    Arrow,
    DoubleArrow,
    Colon,
    ColonColon,
    At,
    Amp,
    Caret,

    Identifier,
    TupleIndex,

    // Literals

    _LiteralsStart,
    _NumbersStart,
    _SignedNumbersStart,
    FloatDefault,
    Float32,
    Float64,
    _IntegersStart,
    _SignedIntegersStart,
    IntDefault,
    Int8,
    Int16,
    Int32,
    Int64,
    _SignedIntegersEnd,
    _SignedNumbersEnd,
    _UnsignedIntegersStart,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    _UnsignedIntegersEnd,
    _IntegersEnd,
    _NumbersEnd,
    Bool,
    Str,
    Nullptr,
    _LiteralsEnd,

    // Keywords

    KwAnd,
    KwOr,
    KwNot,
    KwBlock,
    KwUnsafe,
    KwIf,
    KwThen,
    KwElse,
    KwElif,
    KwLoop,
    KwWhile,
    KwDo,
    KwBreak,
    KwContinue,
    KwReturn,
    KwYield,
    KwPass,

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
    KwSizeof,
    KwTypeof,
    KwTransmute,
    KwAlloc,
    KwDealloc,

    KwPrintout // Temporary print keyword for development.
};

namespace tokens {

/**
 * @brief Checks if a token type is for a literal value.
 *
 * Examples of literal token types are Int32, Float64, Bool, Str, etc.
 *
 * @param tok The token type to check.
 * @return True if the token type is a literal, false otherwise.
 */
inline bool is_literal(Tok tok) {
    return tok > Tok::_LiteralsStart && tok < Tok::_LiteralsEnd;
}

/**
 * @brief Checks if a token type is for a number literal.
 *
 * Examples of number token types are Int32, Float64, etc.
 *
 * @param tok The token type to check.
 * @return True if the token type is a number, false otherwise.
 */
inline bool is_number(Tok tok) {
    return tok > Tok::_NumbersStart && tok < Tok::_NumbersEnd;
}

/**
 * @brief Checks if a token type is for a signed number.
 *
 * Signed numbers include all floating-point numbers and signed integers.
 *
 * Useful for determining if a negative sign can be applied to a numeric
 * literal.
 *
 * @param tok The token type to check.
 * @return True if the token type is a signed number, false otherwise.
 */
inline bool is_signed_number(Tok tok) {
    return tok > Tok::_SignedNumbersStart && tok < Tok::_SignedNumbersEnd;
}

/**
 * @brief Checks if a token type is for an unsigned integer.
 *
 * Useful for determining if a negative sign cannot be applied to a numeric
 * literal.
 *
 * @param tok The token type to check.
 * @return True if the token type is an unsigned integer, false otherwise.
 */
inline bool is_unsigned_integer(Tok tok) {
    return tok > Tok::_UnsignedIntegersStart && tok < Tok::_UnsignedIntegersEnd;
}

/**
 * @brief Checks if a token type is a compound operator.
 *
 * Examples of compound operator token types are PlusEq, MinusEq, StarEq, etc.
 *
 * This function tests if the token type is within the range of defined compound
 * operator token types, making it more efficient than testing each type
 * individually.
 *
 * @param tok The token type to check.
 * @return True if the token type is a compound operator, false otherwise.
 */
inline bool is_compound_operator(Tok tok) {
    return tok > Tok::_CompoundOperatorsStart &&
           tok < Tok::_CompoundOperatorsEnd;
}

/**
 * @brief Checks if a token type is a comparison operator.
 *
 * Examples of comparison operator token types are Gt, GtEq, Lt, LtEq, etc.
 *
 * This function tests if the token type is within the range of defined
 * comparison operator token types, making it more efficient than testing each
 * type individually.
 *
 * @param tok The token type to check.
 * @return True if the token type is a comparison operator, false otherwise.
 */
inline bool is_comparison_operator(Tok tok) {
    return tok > Tok::_ComparisonsStart && tok < Tok::_ComparisonsEnd;
}

} // namespace tokens

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
     *
     * @param file A pointer to the code file.
     * @param start The start index of the token.
     * @param length The length of the token.
     */
    Location(
        std::shared_ptr<CodeFile> file, size_t start, size_t length, size_t line
    )
        : file(file), start(start), length(length), line(line) {}

    /**
     * @brief Convert the location to a 3-tuple of (file path, line number,
     * column number).
     * @return A 3-tuple containing the file path, line number, and column
     * number.
     */
    std::tuple<std::string, size_t, size_t> to_tuple() const {
        size_t line_start = file->src_code.rfind('\n', start);
        if (line_start == std::string::npos) {
            line_start = 0;
        }
        return {file->path_string, line, start - line_start + 1};
    }

    /**
     * @brief Convert the location to a string in the format
     * "file_path:line_number:column_number".
     * @return A string representation of the location.
     */
    std::string to_string() const {
        auto [file_path, line_num, col_num] = to_tuple();
        return file_path + ":" + std::to_string(line_num) + ":" +
               std::to_string(col_num);
    }
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
    // The literal value of this token, if any; primarily used for string
    // literals
    std::any literal;

    /**
     * @brief Constructs a new Token object.
     *
     * @param tok_type The type of the token.
     * @param location The location of the token.
     * @param literal The literal value of the token, if any. Default is an
     * empty std::any value.
     */
    Token(Tok tok_type, const Location& location, std::any literal = std::any())
        : tok_type(tok_type),
          location(location),
          lexeme(
              std::string_view(location.file->src_code)
                  .substr(location.start, location.length)
          ),
          literal(literal) {}
};

} // namespace nico

#endif // NICO_TOKEN_H
