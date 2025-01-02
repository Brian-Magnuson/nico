#include "token.h"

std::string Token::get_lexeme() const {
    return location.file->src_code.substr(location.start, location.length);
}
