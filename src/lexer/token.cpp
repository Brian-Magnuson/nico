#include "token.h"

std::string_view Token::get_lexeme() const {
    return std::string_view(location.file->src_code).substr(location.start, location.length);
}
