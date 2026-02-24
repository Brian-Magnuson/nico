#include "nico/frontend/components/parser.h"

#include <cctype>
#include <cstdint>

#include "nico/shared/logger.h"
#include "nico/shared/utils.h"

namespace nico {

bool Parser::is_at_end() const {
    return current >= tokens.size();
}

const std::shared_ptr<Token>& Parser::peek() const {
    if (is_at_end()) {
        return tokens.back();
    }
    return tokens.at(current);
}

const std::shared_ptr<Token>& Parser::previous() const {
    if (current == 0) {
        panic("Parser::previous: No previous token.");
    }
    return tokens.at(current - 1);
}

const std::shared_ptr<Token>& Parser::advance() {
    if (!is_at_end()) {
        current++;
    }
    return tokens.at(current - 1);
}

bool Parser::match(const std::vector<Tok>& types) {
    for (const auto& type : types) {
        if (peek()->tok_type == type) {
            advance();
            return true;
        }
    }
    return false;
}

void Parser::synchronize() {
    advance();

    while (!is_at_end()) {
        switch (peek()->tok_type) {
        case Tok::Eof:
        case Tok::KwLet:
            return;
        default:
            break;
        }

        advance();
    }
}

std::shared_ptr<Token>
Parser::binary_op_from_compound_op(const std::shared_ptr<Token>& compound_op) {
    Tok binary_op_type;
    switch (compound_op->tok_type) {
    case Tok::PlusEq:
        binary_op_type = Tok::Plus;
        break;
    case Tok::MinusEq:
        binary_op_type = Tok::Minus;
        break;
    case Tok::StarEq:
        binary_op_type = Tok::Star;
        break;
    case Tok::SlashEq:
        binary_op_type = Tok::Slash;
        break;
    case Tok::PercentEq:
        binary_op_type = Tok::Percent;
        break;
    default:
        panic(
            "Parser::binary_op_from_compound_op: Unknown compound assignment "
            "operator."
        );
    }
    // Create a new token for the binary operator, adjusting length to exclude
    // '='
    auto binary_op_loc = compound_op->location;
    binary_op_loc.length -= 1;
    return std::make_shared<Token>(binary_op_type, binary_op_loc);
}

// MARK: Expressions

std::optional<std::shared_ptr<Expr>> Parser::block(Expr::Block::Kind kind) {
    Tok closing_token_type;
    bool is_unsafe = previous()->tok_type == Tok::KwUnsafe;

    if (peek()->tok_type == Tok::Indent) {
        closing_token_type = Tok::Dedent;
    }
    else if (peek()->tok_type == Tok::LBrace) {
        closing_token_type = Tok::RBrace;
    }
    else {
        Logger::inst().log_error(
            Err::NotABlock,
            peek()->location,
            "Expected '{' or an indent to start a block expression."
        );
        if (peek()->tok_type == Tok::Colon) {
            Logger::inst().log_note("Indentation is possibly ignored here.");
        }
        return std::nullopt;
    }
    auto opening_tok = advance();

    std::vector<std::shared_ptr<Stmt::IExecAllowed>> statements;
    bool defer_error = false;
    while (!match({closing_token_type})) {
        auto stmt = statement();
        if (!stmt)
            return std::nullopt;
        auto exec_allowed_stmt =
            std::dynamic_pointer_cast<Stmt::IExecAllowed>(*stmt);
        if (!exec_allowed_stmt) {
            Logger::inst().log_error(
                Err::NonExecAllowedStmt,
                stmt.value()->location,
                "Block expression does not allow this kind of statement."
            );
            Logger::inst().log_note(
                "Only execution-space statements are allowed in block "
                "expressions. Declarations must be made outside of block "
                "expressions."
            );
            defer_error = true;
            continue;
        }
        statements.push_back(exec_allowed_stmt);
    }

    if (defer_error)
        return std::nullopt;

    return std::make_shared<Expr::Block>(
        opening_tok,
        std::move(statements),
        kind,
        is_unsafe
    );
}

std::optional<std::shared_ptr<Expr>> Parser::conditional() {
    auto if_kw = previous();
    bool implicit_else = false;

    // Handle the condition.
    auto condition = expression();
    if (!condition)
        return std::nullopt;

    // Handle the 'then' branch.
    std::optional<std::shared_ptr<Expr>> then_branch;
    if (peek()->tok_type == Tok::Indent || peek()->tok_type == Tok::LBrace) {
        then_branch = block(Expr::Block::Kind::Plain);
    }
    else if (match({Tok::KwThen})) {
        then_branch = expression();
    }
    else {
        Logger::inst().log_error(
            Err::ConditionalWithoutThenOrBlock,
            peek()->location,
            "Conditional expression requires `then` keyword or a block."
        );
        if (peek()->tok_type == Tok::Colon) {
            Logger::inst().log_note("Indentation is possibly ignored here.");
        }
        return std::nullopt;
    }

    if (!then_branch)
        return std::nullopt;

    // Handle the optional 'else' branch.
    std::optional<std::shared_ptr<Expr>> else_branch = std::nullopt;
    if (match({Tok::KwElse})) {
        if (peek()->tok_type == Tok::Indent ||
            peek()->tok_type == Tok::LBrace) {
            else_branch = block(Expr::Block::Kind::Plain);
        }
        else {
            else_branch = expression();
        }
        if (!else_branch)
            return std::nullopt;
    }
    else {
        // If there is no `else` keyword, we inject a unit value.
        else_branch = std::make_shared<Expr::Unit>(if_kw);
        implicit_else = true;
    }

    return std::make_shared<Expr::Conditional>(
        if_kw,
        *condition,
        *then_branch,
        *else_branch,
        implicit_else
    );
}

std::optional<std::shared_ptr<Expr>> Parser::loop() {
    auto loop_kw = previous();
    std::optional<std::shared_ptr<Expr>> condition;
    std::optional<std::shared_ptr<Expr>> expr_body;
    bool loops_once = false;

    if (loop_kw->tok_type == Tok::KwLoop) {
        // Loop-loops always run at least once.
        loops_once = true;
        if (peek()->tok_type == Tok::Indent ||
            peek()->tok_type == Tok::LBrace) {
            expr_body = block(Expr::Block::Kind::Loop);
        }
        else {
            expr_body = expression();
        }
        if (!expr_body)
            return std::nullopt;
    }
    else if (loop_kw->tok_type == Tok::KwWhile) {
        // Parse the condition.
        condition = expression();
        if (!condition)
            return std::nullopt;
        if (peek()->tok_type == Tok::Indent ||
            peek()->tok_type == Tok::LBrace) {
            expr_body = block(Expr::Block::Kind::Loop);
        }
        else if (match({Tok::KwDo})) {
            expr_body = expression();
        }
        else {
            Logger::inst().log_error(
                Err::WhileLoopWithoutDoOrBlock,
                peek()->location,
                "While loop requires `do` keyword or a block."
            );
            if (peek()->tok_type == Tok::Colon) {
                Logger::inst().log_note(
                    "Indentation is possibly ignored here."
                );
            }
            return std::nullopt;
        }
        if (!expr_body)
            return std::nullopt;

        // If the condition is literally `true`...
        if (auto literal =
                std::dynamic_pointer_cast<Expr::Literal>(*condition)) {
            if (literal->token->lexeme == "true") {
                // Treat this like a loop-loop
                loops_once = true;
                condition = std::nullopt;
            }
        }
    }
    else if (loop_kw->tok_type == Tok::KwDo) {
        // Do-while loops always run at least once.
        loops_once = true;
        if (peek()->tok_type == Tok::Indent ||
            peek()->tok_type == Tok::LBrace) {
            expr_body = block(Expr::Block::Kind::Loop);
        }
        else {
            expr_body = expression();
        }
        if (!expr_body)
            return std::nullopt;

        // Check for the `while` keyword.
        if (!match({Tok::KwWhile})) {
            Logger::inst().log_error(
                Err::DoWhileLoopWithoutWhile,
                peek()->location,
                "`do` must be followed by `while`."
            );
            return std::nullopt;
        }
        // Parse the condition.
        condition = expression();
        if (!condition)
            return std::nullopt;
        // If the condition is literally `true`...
        if (auto literal =
                std::dynamic_pointer_cast<Expr::Literal>(*condition)) {
            if (literal->token->lexeme == "true") {
                // Treat this like a loop-loop
                condition = std::nullopt;
            }
        }
    }
    else {
        panic("Parser::loop: Unexpected loop keyword.");
    }

    // The body must be a block.
    // If it is not a block, we wrap it in one.
    std::shared_ptr<Expr::Block> body =
        std::dynamic_pointer_cast<Expr::Block>(expr_body.value());
    if (!body) {
        body = std::make_shared<Expr::Block>(
            loop_kw,
            std::vector<std::shared_ptr<Stmt::IExecAllowed>>{
                std::make_shared<Stmt::Expression>(expr_body.value())
            },
            Expr::Block::Kind::Loop
        );
    }

    return std::make_shared<Expr::Loop>(loop_kw, body, condition, loops_once);
}

std::optional<std::shared_ptr<Expr>> Parser::allocation() {
    auto alloc_kw = previous();

    // Check for 'for' keyword (dynamic array allocation).
    if (match({Tok::KwFor})) {
        // `alloc for <amount_expr> of <type_annotation>`

        // Parse the amount expression.
        auto amount_expr = expression();
        if (!amount_expr)
            return std::nullopt;
        // Check for 'of' keyword.
        if (!match({Tok::KwOf})) {
            Logger::inst().log_error(
                Err::AllocForWithoutOf,
                peek()->location,
                "Expected `of` keyword after amount expression after `alloc "
                "for`."
            );
            return std::nullopt;
        }
        // Parse the type annotation.
        auto type_annotation = annotation();
        if (!type_annotation)
            return std::nullopt;
        return std::make_shared<Expr::Alloc>(
            alloc_kw,
            type_annotation,
            std::nullopt,
            amount_expr
        );
    }
    else if (match({Tok::KwWith})) {
        // `alloc with <init_expr>`

        // Parse the initialization expression.
        auto init_expr = expression();
        if (!init_expr)
            return std::nullopt;
        return std::make_shared<Expr::Alloc>(alloc_kw, std::nullopt, init_expr);
    }
    else {
        // `alloc <type_annotation> [with <init_expr>]`

        // Parse the type annotation.
        auto type_annotation = annotation();
        if (!type_annotation)
            return std::nullopt;
        std::optional<std::shared_ptr<Expr>> init_expr = std::nullopt;
        if (match({Tok::KwWith})) {
            // Parse the initialization expression.
            init_expr = expression();
            if (!init_expr)
                return std::nullopt;
        }
        return std::make_shared<Expr::Alloc>(
            alloc_kw,
            type_annotation,
            init_expr
        );
    }
}

std::optional<size_t> Parser::array_size() {
    if (peek()->tok_type != Tok::IntDefault) {
        Logger::inst().log_error(
            Err::NaturalNumberWithoutIntDefaultToken,
            peek()->location,
            "Expected a non-negative integer without a sign or type suffix."
        );
        return std::nullopt;
    }
    std::string numeric_string;
    auto lexeme = peek()->lexeme;
    for (size_t i = 0; i < lexeme.size(); i++) {
        if (lexeme[i] == '_') {
            continue;
        }
        else if (!std::isdigit(lexeme[i])) {
            Logger::inst().log_error(
                Err::AlphaCharInArraySize,
                peek()->location,
                "Array size contains non-digit characters."
            );
            Logger::inst().log_note(
                "Only base-10 digits (0-9) and underscores are allowed in this "
                "number."
            );
            return std::nullopt;
        }
        numeric_string += lexeme[i];
    }
    advance();
    auto token = previous();
    auto [any_val, ec] = parse_number<size_t>(numeric_string, 10);

    if (ec == std::errc::result_out_of_range) {
        Logger::inst().log_error(
            Err::ArraySizeTooLarge,
            previous()->location,
            "Array size is too large."
        );
        return std::nullopt;
    }
    else if (ec != std::errc()) {
        panic("Parser::array_size: Number in unexpected format.");
        return std::nullopt;
    }
    token->literal = any_val;
    token->tok_type = Tok::ArraySize;
    return std::any_cast<size_t>(any_val);
}

std::optional<std::shared_ptr<Name>> Parser::name() {
    std::shared_ptr<Token> identifier = previous();
    if (identifier->tok_type != Tok::Identifier) {
        panic(
            "Parser::name: Attempted to parse a name, but previous token is "
            "not an identifier."
        );
    }
    auto name = std::make_shared<Name>(identifier);

    while (match({Tok::ColonColon})) {
        if (!match({Tok::Identifier})) {
            Logger::inst().log_error(
                Err::NotAnIdentifier,
                peek()->location,
                "Expected an identifier after `::`."
            );
            return std::nullopt;
        }
        name = std::make_shared<Name>(name, previous());
    }
    return name;
}

std::optional<std::shared_ptr<Expr>> Parser::number_literal() {
    std::string numeric_string;
    if (current > 0 && previous()->tok_type == Tok::Negative) {
        if (tokens::is_unsigned_integer(peek()->tok_type)) {
            Logger::inst().log_error(
                Err::NegativeOnUnsignedLiteral,
                previous()->location,
                "Cannot use unary `-` on unsigned integer literal."
            );
            return std::nullopt;
        }
        numeric_string += "-";
    }
    auto lexeme = peek()->lexeme;
    int base = 10;
    int start_index = 0;
    if (lexeme.starts_with("0b")) {
        base = 2;
        start_index = 2;
    }
    else if (lexeme.starts_with("0x")) {
        base = 16;
        start_index = 2;
    }
    else if (lexeme.starts_with("0o")) {
        base = 8;
        start_index = 2;
    }
    for (size_t i = start_index; i < lexeme.size(); i++) {
        if (lexeme[i] != '_') {
            numeric_string += lexeme[i];
        }
    }

    advance();
    auto token = previous();
    std::pair<std::any, std::errc> parse_result;

    switch (token->tok_type) {
    case Tok::Int8:
        parse_result = parse_number<int8_t>(numeric_string, base);
        break;
    case Tok::Int16:
        parse_result = parse_number<int16_t>(numeric_string, base);
        break;
    case Tok::Int32:
        parse_result = parse_number<int32_t>(numeric_string, base);
        break;
    case Tok::Int64:
        parse_result = parse_number<int64_t>(numeric_string, base);
        break;
    case Tok::UInt8:
        parse_result = parse_number<uint8_t>(numeric_string, base);
        break;
    case Tok::UInt16:
        parse_result = parse_number<uint16_t>(numeric_string, base);
        break;
    case Tok::UInt32:
        parse_result = parse_number<uint32_t>(numeric_string, base);
        break;
    case Tok::UInt64:
        parse_result = parse_number<uint64_t>(numeric_string, base);
        break;
    case Tok::Float32:
        parse_result = parse_number<float>(numeric_string);
        break;
    case Tok::Float64:
        parse_result = parse_number<double>(numeric_string);
        break;
    case Tok::IntDefault:
        token->tok_type = Tok::Int32;
        parse_result = parse_number<int32_t>(numeric_string, base);
        break;
    case Tok::FloatDefault:
        token->tok_type = Tok::Float64;
        parse_result = parse_number<double>(numeric_string);
        break;
    default:
        panic("Parser::number_literal: Unexpected token type.");
        return std::nullopt;
    }

    auto [any_val, ec] = parse_result;
    if (ec == std::errc::result_out_of_range) {
        Logger::inst().log_error(
            Err::NumberOutOfRange,
            previous()->location,
            "Numeric literal is out of range for its type."
        );
        return std::nullopt;
    }
    else if (ec != std::errc()) {
        panic("Parser::number_literal: Number in unexpected format.");
        return std::nullopt;
    }
    token->literal = any_val;
    return std::make_shared<Expr::Literal>(token);
}

std::optional<std::shared_ptr<Expr>> Parser::primary() {
    if (tokens::is_number(peek()->tok_type)) {
        return number_literal();
    }
    if (match({Tok::Bool, Tok::Nullptr, Tok::Str})) {
        return std::make_shared<Expr::Literal>(previous());
    }
    if (match({Tok::Identifier})) {
        auto name_opt = name();
        if (!name_opt)
            return std::nullopt;
        return std::make_shared<Expr::NameRef>(*name_opt);
    }
    if (match({Tok::KwBlock, Tok::KwUnsafe})) {
        return block(Expr::Block::Kind::Plain);
    }
    if (match({Tok::KwIf})) {
        return conditional();
    }
    if (match({Tok::KwLoop, Tok::KwWhile, Tok::KwDo})) {
        return loop();
    }
    if (match({Tok::KwSizeof})) {
        auto token = previous();
        auto anno = annotation();
        if (!anno)
            return std::nullopt;
        return std::make_shared<Expr::SizeOf>(token, *anno);
    }
    if (match({Tok::KwAlloc})) {
        return allocation();
    }
    if (match({Tok::LParen})) {
        // Grouping or tuple expression.
        auto lparen = previous();
        std::vector<std::shared_ptr<Expr>> elements;
        if (match({Tok::RParen})) {
            // Empty tuple
            return std::make_shared<Expr::Tuple>(lparen, std::move(elements));
        }
        bool comma_matched = false;
        do {
            if (peek()->tok_type == Tok::RParen) {
                // We allow trailing commas.
                break;
            }
            auto expr = expression();
            if (!expr)
                return std::nullopt;
            elements.push_back(*expr);
            comma_matched = match({Tok::Comma});
        } while (comma_matched);

        if (!match({Tok::RParen})) {
            Logger::inst().log_error(
                Err::UnexpectedToken,
                peek()->location,
                "Expected `)` after expression grouping."
            );
            return std::nullopt;
        }

        if (elements.size() == 1 && !comma_matched) {
            // Just a parenthesized expression
            return elements[0];
        }
        else {
            return std::make_shared<Expr::Tuple>(lparen, std::move(elements));
        }
    }
    if (match({Tok::LSquare})) {
        // Array literal
        auto lsquare = previous();
        std::vector<std::shared_ptr<Expr>> elements;
        if (match({Tok::RSquare})) {
            // Empty array
            return std::make_shared<Expr::Array>(lsquare, std::move(elements));
        }
        bool comma_matched = false;
        do {
            if (peek()->tok_type == Tok::RSquare) {
                // We allow trailing commas.
                break;
            }
            auto expr = expression();
            if (!expr)
                return std::nullopt;
            elements.push_back(*expr);
            comma_matched = match({Tok::Comma});
        } while (comma_matched);

        if (!match({Tok::RSquare})) {
            Logger::inst().log_error(
                Err::UnexpectedToken,
                peek()->location,
                "Expected `]` after array literal."
            );
            return std::nullopt;
        }

        return std::make_shared<Expr::Array>(lsquare, std::move(elements));
    }

    if (repl_mode && peek()->tok_type == Tok::Eof) {
        incomplete_statement = true;
    }
    else {
        Logger::inst().log_error(
            Err::NotAnExpression,
            peek()->location,
            "Expected expression."
        );
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<Expr>> Parser::postfix() {
    auto left = primary();
    if (!left)
        return std::nullopt;
    while (match({Tok::Dot, Tok::LParen, Tok::LSquare})) {
        if (previous()->tok_type == Tok::Dot) {
            auto op = previous();
            if (match({Tok::TupleIndex, Tok::Identifier})) {
                left = std::make_shared<Expr::Access>(*left, op, previous());
            }
            else {
                Logger::inst().log_error(
                    Err::UnexpectedTokenAfterDot,
                    peek()->location,
                    "Expected identifier or integer after `.`."
                );
                return std::nullopt;
            }
        }
        else if (previous()->tok_type == Tok::LSquare) {
            auto op = previous();
            auto index_expr = expression();
            if (!index_expr)
                return std::nullopt;
            if (!match({Tok::RSquare})) {
                Logger::inst().log_error(
                    Err::UnexpectedToken,
                    peek()->location,
                    "Expected `]` after array subscript."
                );
                return std::nullopt;
            }
            left = std::make_shared<Expr::Subscript>(*left, op, *index_expr);
        }
        else if (previous()->tok_type == Tok::LParen) {
            auto l_paren = previous();
            std::vector<std::shared_ptr<Expr>> pos_args;
            Dictionary<std::string, std::shared_ptr<Expr>> named_args;
            bool has_named_args = false;
            do {
                if (peek()->tok_type == Tok::RParen) {
                    // Allow trailing commas.
                    break;
                }
                if (peek()->tok_type == Tok::Identifier &&
                    tokens.at(current + 1)->tok_type == Tok::Colon) {
                    // This token definitely exists because there is a
                    // guaranteed `)` from the lexer.
                    has_named_args = true;
                    // Definitely a named argument.
                    auto name_token = advance(); // Consume identifier
                    advance();                   // Consume ':'
                    auto expr = expression();
                    if (!expr)
                        return std::nullopt;
                    named_args.insert(std::string(name_token->lexeme), *expr);
                }
                else {
                    // Not a named argument, just a normal positional argument.
                    auto expr = expression();
                    if (!expr)
                        return std::nullopt;
                    pos_args.push_back(*expr);
                    if (has_named_args) {
                        Logger::inst().log_error(
                            Err::PosArgumentAfterNamedArgument,
                            expr->get()->location,
                            "Positional arguments cannot follow named "
                            "arguments."
                        );
                        return std::nullopt;
                    }
                }
            } while (match({Tok::Comma}));
            if (!match({Tok::RParen})) {
                Logger::inst().log_error(
                    Err::UnexpectedToken,
                    peek()->location,
                    "Expected `)` after arguments in function call."
                );
                return std::nullopt;
            }
            left = std::make_shared<Expr::Call>(
                *left,
                l_paren,
                std::move(pos_args),
                std::move(named_args)
            );
        }
    }
    return left;
}

std::optional<std::shared_ptr<Expr>> Parser::unary() {
    if (match({Tok::Minus})) {
        auto token = previous();
        token->tok_type = Tok::Negative;
        auto right = unary();
        if (!right)
            return std::nullopt;
        else if (tokens::is_signed_number(previous()->tok_type))
            // number_literal already handles the negation.
            return right;

        return std::make_shared<Expr::Unary>(token, *right);
    }
    if (match({Tok::KwNot, Tok::Bang})) {
        auto token = previous();
        auto right = unary();
        if (!right)
            return std::nullopt;
        return std::make_shared<Expr::Unary>(token, *right);
    }
    if (match({Tok::Caret})) {
        auto token = previous();
        auto right = unary();
        if (!right)
            return std::nullopt;
        return std::make_shared<Expr::Deref>(token, *right);
    }
    bool has_var = match({Tok::KwVar});
    if (match({Tok::At, Tok::Amp})) {
        auto token = previous();
        auto right = unary();
        if (!right)
            return std::nullopt;
        return std::make_shared<Expr::Address>(token, *right, has_var);
    }
    else if (has_var) {
        Logger::inst().log_error(
            Err::UnexpectedVarInExpression,
            peek()->location,
            "`var` must be followed by address-of operator `@` or `&`."
        );
        return std::nullopt;
    }
    return postfix();
}

std::optional<std::shared_ptr<Expr>> Parser::cast() {
    auto left = unary();
    if (!left)
        return std::nullopt;
    while (match({Tok::KwAs})) {
        auto as_token = previous();
        auto anno = annotation();
        if (!anno)
            return std::nullopt;
        left = std::make_shared<Expr::Cast>(*left, as_token, *anno);
    }
    return left;
}

std::optional<std::shared_ptr<Expr>> Parser::factor() {
    auto left = cast();
    if (!left)
        return std::nullopt;
    while (match({Tok::Star, Tok::Slash, Tok::Percent})) {
        auto op = previous();
        auto right = cast();
        if (!right)
            return std::nullopt;
        left = std::make_shared<Expr::Binary>(*left, op, *right);
    }
    return left;
}

std::optional<std::shared_ptr<Expr>> Parser::term() {
    auto left = factor();
    if (!left)
        return std::nullopt;
    while (match({Tok::Plus, Tok::Minus})) {
        auto op = previous();
        auto right = factor();
        if (!right)
            return std::nullopt;
        left = std::make_shared<Expr::Binary>(*left, op, *right);
    }
    return left;
}

std::optional<std::shared_ptr<Expr>> Parser::comparison() {
    auto left = term();
    if (!left)
        return std::nullopt;
    while (match({Tok::Lt, Tok::Gt, Tok::LtEq, Tok::GtEq})) {
        auto op = previous();
        auto right = term();
        if (!right)
            return std::nullopt;
        left = std::make_shared<Expr::Binary>(*left, op, *right);
    }
    return left;
}

std::optional<std::shared_ptr<Expr>> Parser::equality() {
    auto left = comparison();
    if (!left)
        return std::nullopt;
    while (match({Tok::EqEq, Tok::BangEq})) {
        auto op = previous();
        auto right = comparison();
        if (!right)
            return std::nullopt;
        left = std::make_shared<Expr::Binary>(*left, op, *right);
    }
    return left;
}

std::optional<std::shared_ptr<Expr>> Parser::logical_and() {
    auto left = equality();
    if (!left)
        return std::nullopt;
    while (match({Tok::KwAnd})) {
        auto op = previous();
        auto right = equality();
        if (!right)
            return std::nullopt;
        left = std::make_shared<Expr::Logical>(*left, op, *right);
    }
    return left;
}

std::optional<std::shared_ptr<Expr>> Parser::logical_or() {
    auto left = logical_and();
    if (!left)
        return std::nullopt;
    while (match({Tok::KwOr})) {
        auto op = previous();
        auto right = logical_and();
        if (!right)
            return std::nullopt;
        left = std::make_shared<Expr::Logical>(*left, op, *right);
    }
    return left;
}

std::optional<std::shared_ptr<Expr>> Parser::assignment() {
    auto left = logical_or();
    if (!left)
        return std::nullopt;
    if (match({Tok::Eq})) {
        auto op = previous();
        auto right = assignment();
        if (!right)
            return std::nullopt;
        return std::make_shared<Expr::Assign>(*left, op, *right);
    }
    else if (match(
                 {Tok::PlusEq,
                  Tok::MinusEq,
                  Tok::StarEq,
                  Tok::SlashEq,
                  Tok::PercentEq}
             )) {
        auto op = previous();
        auto right = assignment();
        if (!right)
            return std::nullopt;

        auto binary_op_token = binary_op_from_compound_op(op);
        auto binary_expr =
            std::make_shared<Expr::Binary>(*left, binary_op_token, *right);
        return std::make_shared<Expr::Assign>(
            *left,
            std::make_shared<Token>(Tok::Eq, op->location),
            binary_expr
        );
    }
    return left;
}

std::optional<std::shared_ptr<Expr>> Parser::expression() {
    return assignment();
}

// MARK: Statements

std::optional<std::shared_ptr<Stmt>> Parser::variable_statement() {
    auto start_token = previous();
    // Check for `var`
    bool has_var = match({Tok::KwVar});

    // Get identifier
    if (!match({Tok::Identifier})) {
        Logger::inst().log_error(
            Err::NotAnIdentifier,
            peek()->location,
            "Expected identifier in declaration."
        );
        return std::nullopt;
    }
    auto identifier = previous();

    if (match({Tok::ColonColon})) {
        Logger::inst().log_error(
            Err::DeclarationIdentWithColonColon,
            previous()->location,
            "Declaration identifier cannot contain `::`."
        );
        return std::nullopt;
    }

    // Check for type annotation
    std::optional<std::shared_ptr<Annotation>> anno = std::nullopt;
    if (match({Tok::Colon})) {
        anno = annotation();
        if (!anno) {
            // At this point, an error has already been logged.
            return std::nullopt;
        }
    }

    // Get expression
    std::optional<std::shared_ptr<Expr>> expr = std::nullopt;
    if (match({Tok::Eq})) {
        expr = expression();
        if (!expr) {
            // At this point, an error has already been logged.
            return std::nullopt;
        }

        if (start_token->tok_type == Tok::KwStatic &&
            !expr.value()->is_constant()) {

            Logger::inst().log_error(
                Err::NonCompileTimeExpr,
                previous()->location,
                "Static variable initializer is not a compile-time constant."
            );
            Logger::inst().log_note(
                "Static variables must be initialized with compile-time "
                "constant "
                "expressions."
            );
            return std::nullopt;
        }
    }

    // If expr and annotation are both nullopt, we have an error.
    if (!expr && !anno) {
        Logger::inst().log_error(
            Err::VariableWithoutTypeOrValue,
            peek()->location,
            "Variable declaration must have a type annotation or value."
        );
        return std::nullopt;
    }
    // If we don't have `var`, we require an initializer.
    if (!has_var && !expr) {
        Logger::inst().log_error(
            Err::ImmutableWithoutInitializer,
            peek()->location,
            "Immutable variable declaration must have an initializer."
        );
        return std::nullopt;
    }

    if (start_token->tok_type == Tok::KwLet) {
        return std::make_shared<Stmt::Let>(
            start_token,
            identifier,
            expr,
            has_var,
            anno
        );
    }
    else if (start_token->tok_type == Tok::KwStatic) {
        return std::make_shared<Stmt::Static>(
            start_token,
            identifier,
            expr,
            has_var,
            anno
        );
    }
    else {
        panic("Parser::variable_statement: Unexpected starting token.");
        return std::nullopt;
    }
}

std::optional<std::shared_ptr<Stmt>> Parser::func_statement() {
    auto start_token = previous();
    // Identifier
    if (!match({Tok::Identifier})) {
        Logger::inst().log_error(
            Err::NotAnIdentifier,
            peek()->location,
            "Expected identifier in declaration."
        );
        return std::nullopt;
    }
    auto identifier = previous();

    if (match({Tok::ColonColon})) {
        Logger::inst().log_error(
            Err::DeclarationIdentWithColonColon,
            previous()->location,
            "Declaration identifier cannot contain `::`."
        );
        return std::nullopt;
    }

    // Open parenthesis
    if (!match({Tok::LParen})) {
        Logger::inst().log_error(
            Err::FuncWithoutOpeningParen,
            peek()->location,
            "Expected `(` after function name."
        );
        return std::nullopt;
    }

    // Parameters
    std::vector<Stmt::Func::Param> parameters;
    do {
        // Closing parenthesis?
        if (peek()->tok_type == Tok::RParen) {
            // We allow trailing commas.
            break;
        }
        // Has var?
        bool has_var = match({Tok::KwVar});
        // Parameter name
        if (!match({Tok::Identifier})) {
            Logger::inst().log_error(
                Err::NotAnIdentifier,
                peek()->location,
                "Expected identifier in function parameter."
            );
            return std::nullopt;
        }
        auto param_name = previous();
        // Annotation (always required)
        if (!match({Tok::Colon})) {
            Logger::inst().log_error(
                Err::NotAType,
                peek()->location,
                "Expected type annotation in function parameter."
            );
            return std::nullopt;
        }
        auto param_type = annotation();
        if (!param_type) {
            // At this point, an error has already been logged.
            return std::nullopt;
        }
        // Optional default value
        std::optional<std::shared_ptr<Expr>> default_value = std::nullopt;
        if (match({Tok::Eq})) {
            default_value = expression();
            if (!default_value) {
                // At this point, an error has already been logged.
                return std::nullopt;
            }
        }
        parameters.push_back(
            Stmt::Func::Param(has_var, param_name, *param_type, default_value)
        );
    } while (match({Tok::Comma}));

    // Closing parenthesis
    if (!match({Tok::RParen})) {
        Logger::inst().log_error(
            Err::UnexpectedToken,
            peek()->location,
            "Expected `)` after parsing parameters."
        );
        return std::nullopt;
    }

    // Return type (optional)
    std::optional<std::shared_ptr<Annotation>> return_type = std::nullopt;
    if (match({Tok::Arrow})) {
        return_type = annotation();
        if (!return_type) {
            // At this point, an error has already been logged.
            return std::nullopt;
        }
    }

    std::shared_ptr<Expr::Block> body_expr;
    // Function body
    if (match({Tok::DoubleArrow})) {
        // Single-expression function.
        // For simplicity, wrap it in a block.
        body_expr = std::make_shared<Expr::Block>(
            previous(),
            std::vector<std::shared_ptr<Stmt::IExecAllowed>>{std::make_shared<
                Stmt::Yield>(
                std::make_shared<Token>(Tok::KwReturn, previous()->location),
                *expression()
            )},
            Expr::Block::Kind::Function,
            false
        );
    }
    else if (peek()->tok_type == Tok::Indent ||
             peek()->tok_type == Tok::LBrace) {
        // Block function
        auto block_expr = block(Expr::Block::Kind::Function);
        if (!block_expr) {
            // At this point, an error has already been logged.
            return std::nullopt;
        }
        body_expr = std::dynamic_pointer_cast<Expr::Block>(*block_expr);
    }
    else {
        Logger::inst().log_error(
            Err::FuncWithoutArrowOrBlock,
            peek()->location,
            "Expected `=>` or a block for function body."
        );
        if (peek()->tok_type == Tok::Colon) {
            Logger::inst().log_note("Indentation is possibly ignored here.");
        }
        return std::nullopt;
    }

    // Put it all together
    return std::make_shared<Stmt::Func>(
        start_token,
        identifier,
        return_type,
        std::move(parameters),
        body_expr
    );
}

std::optional<std::shared_ptr<Stmt>> Parser::namespace_statement() {
    auto start_token = previous();
    // Identifier
    if (!match({Tok::Identifier})) {
        Logger::inst().log_error(
            Err::NotAnIdentifier,
            peek()->location,
            "Expected identifier in namespace declaration."
        );
        return std::nullopt;
    }
    auto identifier = previous();

    if (match({Tok::ColonColon})) {
        Logger::inst().log_error(
            Err::DeclarationIdentWithColonColon,
            previous()->location,
            "Declaration identifier cannot contain `::`."
        );
        return std::nullopt;
    }

    Tok closing_token_type;
    bool is_file_spanning = false;
    if (match({Tok::Indent})) {
        closing_token_type = Tok::Dedent;
    }
    else if (match({Tok::LBrace})) {
        closing_token_type = Tok::RBrace;
    }
    else {
        Logger::inst().log_error(
            Err::NamespaceWithoutBlock,
            peek()->location,
            "Expected indented block or `{` after namespace declaration."
        );
        if (peek()->tok_type == Tok::Colon) {
            Logger::inst().log_note("Indentation is possibly ignored here.");
        }
        return std::nullopt;
    }

    // Body
    std::vector<std::shared_ptr<Stmt::IDeclAllowed>> body_stmts;
    bool defer_error = false;
    while (!match({closing_token_type})) {
        auto stmt = statement();
        if (!stmt) {
            // At this point, an error has already been logged.
            return std::nullopt;
        }
        auto decl_allowed_stmt =
            std::dynamic_pointer_cast<Stmt::IDeclAllowed>(*stmt);
        if (!decl_allowed_stmt) {
            Logger::inst().log_error(
                Err::NonDeclAllowedStmt,
                stmt.value()->location,
                "Namespace does not allow this kind of statement."
            );
            Logger::inst().log_note(
                "Only declaration-space statements are allowed directly inside "
                "a namespace. Execution-space statements must be in a local "
                "scope or at the top level."
            );
            if (PTR_INSTANCEOF(stmt.value(), Stmt::Let)) {
                Logger::inst().log_note(
                    "Variables declared with `let` are execution-space "
                    "statements. Consider using `static` instead of `let`."
                );
            }
            defer_error = true;
            continue;
        }
        body_stmts.push_back(decl_allowed_stmt);
    }

    if (defer_error) {
        return std::nullopt;
    }

    return std::make_shared<Stmt::Namespace>(
        start_token,
        identifier,
        is_file_spanning,
        std::move(body_stmts)
    );
}

std::optional<std::shared_ptr<Stmt>> Parser::print_statement() {
    auto print_token = previous();
    std::vector<std::shared_ptr<Expr>> expressions;
    auto expr = expression();
    if (!expr)
        return std::nullopt;
    expressions.push_back(*expr);

    while (match({Tok::Comma})) {
        expr = expression();
        if (!expr)
            return std::nullopt;
        expressions.push_back(*expr);
    }

    return std::make_shared<Stmt::Print>(print_token, std::move(expressions));
}

std::optional<std::shared_ptr<Stmt>> Parser::yield_statement() {
    auto yield_token = previous();
    auto expr = expression();
    if (!expr) {
        return std::nullopt;
    }

    return std::make_shared<Stmt::Yield>(yield_token, *expr);
}

std::optional<std::shared_ptr<Stmt>> Parser::expression_statement() {
    auto expr = expression();
    if (!expr)
        return std::nullopt;

    if (repl_mode && peek()->tok_type == Tok::Eof) {
        // If this is the last statement in REPL mode, we print the result.
        return std::make_shared<Stmt::Print>(std::vector{*expr});
    }

    return std::make_shared<Stmt::Expression>(*expr);
}

std::optional<std::shared_ptr<Stmt>> Parser::statement() {
    // Consume semicolons to separate statements
    while (match({Tok::Semicolon}))
        ;

    if (match({Tok::KwLet, Tok::KwStatic})) {
        return variable_statement();
    }
    else if (match({Tok::KwFunc})) {
        return func_statement();
    }
    else if (match({Tok::KwNamespace})) {
        return namespace_statement();
    }
    else if (match({Tok::Eof})) {
        return std::make_shared<Stmt::Eof>(previous());
    }
    else if (match({Tok::KwPrintout})) {
        return print_statement();
    }
    else if (match({Tok::KwPass})) {
        return std::make_shared<Stmt::Pass>(previous());
    }
    else if (match({Tok::KwYield, Tok::KwBreak, Tok::KwReturn})) {
        return yield_statement();
    }
    else if (match({Tok::KwContinue})) {
        return std::make_shared<Stmt::Continue>(previous());
    }
    else if (match({Tok::KwDealloc})) {
        auto token = previous();
        auto expr = expression();
        if (!expr)
            return std::nullopt;
        return std::make_shared<Stmt::Dealloc>(token, *expr);
    }
    return expression_statement();
}

// MARK: Annotations

std::optional<std::shared_ptr<Annotation>> Parser::annotation() {
    // First, check for annotations that can have 'var': pointers and
    // references.
    bool has_var = false;
    if (match({Tok::KwVar}))
        has_var = true;

    if (match({Tok::At})) {
        auto at_token = previous();
        // Pointer annotation
        auto inner_anno = annotation();
        if (!inner_anno)
            return std::nullopt;
        return std::make_shared<Annotation::Pointer>(
            *inner_anno,
            at_token,
            has_var
        );
    }
    if (match({Tok::Amp})) {
        auto amp_token = previous();
        // Reference annotation
        auto inner_anno = annotation();
        if (!inner_anno)
            return std::nullopt;
        return std::make_shared<Annotation::Reference>(
            *inner_anno,
            amp_token,
            has_var
        );
    }

    if (has_var) {
        Logger::inst().log_error(
            Err::UnexpectedVarInAnnotation,
            previous()->location,
            "`var` is not allowed here. Use only with pointers or references."
        );
        return std::nullopt;
    }
    // Now check for other annotation types.

    if (match({Tok::KwTypeof})) {
        auto typeof_token = previous();
        if (!match({Tok::LParen})) {
            Logger::inst().log_error(
                Err::TypeofWithoutOpeningParen,
                peek()->location,
                "Expected `(` after `typeof`."
            );
            return std::nullopt;
        }
        auto expr = expression();
        if (!match({Tok::RParen})) {
            Logger::inst().log_error(
                Err::UnexpectedToken,
                peek()->location,
                "Expected `)` after expression in "
                "typeof annotation."
            );
            return std::nullopt;
        }
        if (!expr) {
            // At this point, an error has already been logged.
            return std::nullopt;
        }

        return std::make_shared<Annotation::TypeOf>(typeof_token, *expr);
    }
    if (match({Tok::Identifier})) {
        auto token = previous();
        return std::make_shared<Annotation::NameRef>(
            std::make_shared<Name>(token)
        );
    }
    if (match({Tok::Nullptr})) {
        return std::make_shared<Annotation::Nullptr>(previous());
    }
    if (match({Tok::LParen})) {
        auto lparen_token = previous();
        // Tuple annotation
        std::vector<std::shared_ptr<Annotation>> elements;
        do {
            if (peek()->tok_type == Tok::RParen) {
                // We allow trailing commas.
                break;
            }
            auto anno = annotation();
            if (!anno)
                return std::nullopt;
            elements.push_back(*anno);
        } while (match({Tok::Comma}));

        if (!match({Tok::RParen})) {
            Logger::inst().log_error(
                Err::UnexpectedToken,
                peek()->location,
                "Expected `)` after expression in "
                "tuple annotation."
            );
            return std::nullopt;
        }

        return std::make_shared<Annotation::Tuple>(
            lparen_token,
            std::move(elements)
        );
    }
    if (match({Tok::LSquare})) {
        auto lsquare_token = previous();
        if (match({Tok::RSquare})) {
            return std::make_shared<Annotation::Array>(lsquare_token);
        }
        auto element_anno = annotation();
        if (!element_anno)
            return std::nullopt;
        if (!match({Tok::Semicolon})) {
            Logger::inst().log_error(
                Err::UnexpectedToken,
                peek()->location,
                "Expected `;` after element type in array annotation."
            );
            return std::nullopt;
        }
        std::optional<size_t> arr_size = std::nullopt;
        if (match({Tok::Question})) {
            // Unsized array; do nothing.
        }
        else {
            arr_size = array_size();
            if (!arr_size)
                return std::nullopt;
        }
        if (!match({Tok::RSquare})) {
            Logger::inst().log_error(
                Err::UnexpectedToken,
                peek()->location,
                "Expected `]` after size in array annotation."
            );
            return std::nullopt;
        }
        return std::make_shared<Annotation::Array>(
            lsquare_token,
            *element_anno,
            arr_size
        );
    }
    Logger::inst()
        .log_error(Err::NotAType, peek()->location, "Not a valid type.");
    return std::nullopt;
}

// MARK: Interface

void Parser::run_parse(std::unique_ptr<FrontendContext>& context) {
    size_t start_size = context->stmts.size();

    while (!is_at_end()) {
        auto stmt = statement();
        if (stmt) {
            context->stmts.push_back(*stmt);
        }
        else if (repl_mode && incomplete_statement) {
            context->status = Status::Pause(Request::Input);
            // Roll back any statements added during this parse.
            context->stmts.resize(start_size);
            return;
        }
        else {
            synchronize();
        }
    }

    if (Logger::inst().get_errors().empty()) {
        context->status = Status::Ok();
    }
    else if (repl_mode) {
        context->status = Status::Pause(Request::Discard);
        // Roll back any statements added during this parse.
        context->stmts.resize(start_size);
    }
    else {
        context->status = Status::Error();
        // Roll back any statements added during this parse.
        context->stmts.resize(start_size);
    }
}

void Parser::parse(std::unique_ptr<FrontendContext>& context, bool repl_mode) {
    if (IS_VARIANT(context->status, Status::Error)) {
        panic("Parser::parse: Context is already in an error state.");
    }

    Parser parser(std::move(context->scanned_tokens), repl_mode);
    context->scanned_tokens = {};
    parser.run_parse(context);
}

} // namespace nico
