#include "local_checker.h"

std::any LocalChecker::visit(Stmt::Expression* stmt) {
    return std::any();
}

std::any LocalChecker::visit(Stmt::Let* stmt) {
    return std::any();
}

std::any LocalChecker::visit(Stmt::Eof* stmt) {
    return std::any();
}

std::any LocalChecker::visit(Stmt::Print* stmt) {
    return std::any();
}

std::any LocalChecker::visit(Expr::Assign* expr, bool as_lvalue) {
    return std::any();
}

std::any LocalChecker::visit(Expr::Binary* expr, bool as_lvalue) {
    return std::any();
}

std::any LocalChecker::visit(Expr::Unary* expr, bool as_lvalue) {
    return std::any();
}

std::any LocalChecker::visit(Expr::Identifier* expr, bool as_lvalue) {
    return std::any();
}

std::any LocalChecker::visit(Expr::Literal* expr, bool as_lvalue) {
    return std::any();
}

bool LocalChecker::check(std::vector<std::shared_ptr<Stmt>> ast) {
    return true;
}
