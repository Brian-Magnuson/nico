#include "token.h"

Token::Token(Tok tok_type, const Location& location)
    : tok_type(tok_type),
      location(location),
      lexeme(
          std::string_view(location.file->src_code).substr(location.start, location.length)
      ) {}
