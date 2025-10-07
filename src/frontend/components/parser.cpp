#include "nico/frontend/components/parser.h"

#include "nico/frontend/utils/nodes.h"
#include "nico/shared/logger.h"
#include "nico/shared/utils.h"

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

// MARK: Expressions

std::optional<std::shared_ptr<Expr>> Parser::block() {
    Tok closing_token_type;

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
        return std::nullopt;
    }
    auto opening_tok = advance();

    std::vector<std::shared_ptr<Stmt>> statements;
    while (!match({closing_token_type})) {
        auto stmt = statement();
        if (!stmt)
            return std::nullopt;
        statements.push_back(*stmt);
    }

    return std::make_shared<Expr::Block>(opening_tok, statements);
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
        then_branch = block();
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
        return std::nullopt;
    }

    if (!then_branch)
        return std::nullopt;

    // Handle the optional 'else' branch.
    std::optional<std::shared_ptr<Expr>> else_branch = std::nullopt;
    if (match({Tok::KwElse})) {
        if (peek()->tok_type == Tok::Indent ||
            peek()->tok_type == Tok::LBrace) {
            else_branch = block();
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

std::optional<std::shared_ptr<Expr>> Parser::primary() {
    if (match({Tok::Int, Tok::Float, Tok::Bool, Tok::Str})) {
        return std::make_shared<Expr::Literal>(previous());
    }
    if (match({Tok::Identifier})) {
        return std::make_shared<Expr::NameRef>(previous());
    }
    if (match({Tok::KwBlock})) {
        return block();
    }
    if (match({Tok::KwIf})) {
        return conditional();
    }
    if (match({Tok::LParen})) {
        // Grouping or tuple expression.
        auto lparen = previous();
        std::vector<std::shared_ptr<Expr>> elements;
        if (match({Tok::RParen})) {
            // Empty tuple
            return std::make_shared<Expr::Tuple>(lparen, elements);
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
            // This error should already be caught in the lexer.
            panic("Parser::primary: Missing ')' while parsing grouping.");
        }

        if (elements.size() == 1 && !comma_matched) {
            // Just a parenthesized expression
            return elements[0];
        }
        else {
            return std::make_shared<Expr::Tuple>(lparen, elements);
        }
    }

    if (repl_mode) {
        repl_request = Request::Input;
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
    if (match({Tok::Dot})) {
        do {
            auto op = previous();
            if (match({Tok::Int, Tok::Identifier})) {
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
        } while (match({Tok::Dot}));
    }
    return left;
}

std::optional<std::shared_ptr<Expr>> Parser::unary() {
    if (match({Tok::Minus, Tok::KwNot})) {
        auto token = previous();
        auto right = unary();
        if (!right)
            return std::nullopt;
        return std::make_shared<Expr::Unary>(token, *right);
    }
    return postfix();
}

std::optional<std::shared_ptr<Expr>> Parser::factor() {
    auto left = unary();
    if (!left)
        return std::nullopt;
    while (match({Tok::Star, Tok::Slash, Tok::Percent})) {
        auto op = previous();
        auto right = unary();
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
    return term();
}

std::optional<std::shared_ptr<Expr>> Parser::equality() {
    return comparison();
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
    return left;
}

std::optional<std::shared_ptr<Expr>> Parser::expression() {
    return assignment();
}

// MARK: Statements

std::optional<std::shared_ptr<Stmt>> Parser::let_statement() {
    // Check for `var`
    bool has_var = match({Tok::KwVar});

    // Get identifier
    if (!match({Tok::Identifier})) {
        Logger::inst().log_error(
            Err::NotAnIdentifier,
            peek()->location,
            "Expected identifier in let statement."
        );
        return std::nullopt;
    }
    auto identifier = previous();

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
    }

    // If expr and annotation are both nullopt, we have an error.
    if (!expr && !anno) {
        Logger::inst().log_error(
            Err::LetWithoutTypeOrValue,
            peek()->location,
            "Let statement must have a type annotation or value."
        );
        return std::nullopt;
    }

    return std::make_shared<Stmt::Let>(identifier, expr, has_var, anno);
}

std::optional<std::shared_ptr<Stmt>> Parser::print_statement() {
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

    return std::make_shared<Stmt::Print>(expressions);
}

std::optional<std::shared_ptr<Stmt>> Parser::yield_statement() {
    auto expr = expression();
    if (!expr)
        return std::nullopt;

    return std::make_shared<Stmt::Yield>(*expr);
}

std::optional<std::shared_ptr<Stmt>> Parser::expression_statement() {
    auto expr = expression();
    if (!expr)
        return std::nullopt;
    if (repl_mode) {
        // In REPL mode, print the result of the expression
        return std::make_shared<Stmt::Print>(std::vector{*expr});
    }

    return std::make_shared<Stmt::Expression>(*expr);
}

std::optional<std::shared_ptr<Stmt>> Parser::statement() {
    // Consume semicolons to separate statements
    while (match({Tok::Semicolon}))
        ;

    if (match({Tok::KwLet})) {
        return let_statement();
    }
    else if (match({Tok::Eof})) {
        return std::make_shared<Stmt::Eof>();
    }
    else if (match({Tok::KwPrintout})) {
        return print_statement();
    }
    else if (match({Tok::KwPass})) {
        return std::make_shared<Stmt::Pass>();
    }
    else if (match({Tok::KwYield})) {
        return yield_statement();
    }
    return expression_statement();
}

// MARK: Annotations

std::optional<std::shared_ptr<Annotation>> Parser::annotation() {
    if (match({Tok::Identifier})) {
        auto token = previous();
        return std::make_shared<Annotation::NameRef>(Name(token));
    }
    if (match({Tok::LParen})) {
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
            // This error should already be caught in the lexer.
            panic("Parser::annotation: Missing ')' while parsing tuple.");
        }

        return std::make_shared<Annotation::Tuple>(elements);
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
        else if (repl_mode && repl_request != Request::None) {
            context->status = Status::Pause(repl_request);
            return;
        }
        else {
            synchronize();
        }
    }

    if (Logger::inst().get_errors().empty()) {
        context->status = Status::Ok();
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
