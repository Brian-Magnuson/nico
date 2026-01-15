#include "nico/frontend/components/mir_builder.h"

namespace nico {

std::any MIRBuilder::visit(Stmt::Expression* stmt) {
    // Implementation for visiting Expression statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Let* stmt) {
    // Implementation for visiting Let statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Func* stmt) {
    // Implementation for visiting Func statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Print* stmt) {
    // Implementation for visiting Print statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Dealloc* stmt) {
    // Implementation for visiting Dealloc statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Pass* stmt) {
    // Implementation for visiting Pass statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Yield* stmt) {
    // Implementation for visiting Yield statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Continue* stmt) {
    // Implementation for visiting Continue statements goes here.
    return {};
}

std::any MIRBuilder::visit(Stmt::Eof* stmt) {
    // Implementation for visiting Eof statements goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Assign* expr, bool as_lvalue) {
    // Implementation for visiting Assign expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Logical* expr, bool as_lvalue) {
    // Implementation for visiting Logical expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Binary* expr, bool as_lvalue) {
    // Implementation for visiting Binary expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Unary* expr, bool as_lvalue) {
    // Implementation for visiting Unary expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Address* expr, bool as_lvalue) {
    // Implementation for visiting Address expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Deref* expr, bool as_lvalue) {
    // Implementation for visiting Deref expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Cast* expr, bool as_lvalue) {
    // Implementation for visiting Cast expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Access* expr, bool as_lvalue) {
    // Implementation for visiting Access expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Subscript* expr, bool as_lvalue) {
    // Implementation for visiting Subscript expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Call* expr, bool as_lvalue) {
    // Implementation for visiting Call expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::SizeOf* expr, bool as_lvalue) {
    // Implementation for visiting SizeOf expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Alloc* expr, bool as_lvalue) {
    // Implementation for visiting Alloc expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::NameRef* expr, bool as_lvalue) {
    // Implementation for visiting NameRef expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Literal* expr, bool as_lvalue) {
    // Implementation for visiting Literal expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Tuple* expr, bool as_lvalue) {
    // Implementation for visiting Tuple expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Array* expr, bool as_lvalue) {
    // Implementation for visiting Array expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Block* expr, bool as_lvalue) {
    // Implementation for visiting Block expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Conditional* expr, bool as_lvalue) {
    // Implementation for visiting Conditional expressions goes here.
    return {};
}

std::any MIRBuilder::visit(Expr::Loop* expr, bool as_lvalue) {
    // Implementation for visiting Loop expressions goes here.
    return {};
}

void MIRBuilder::run_build() {
    // Implementation of MIR building goes here.
}

void MIRBuilder::build_mir(std::unique_ptr<FrontendContext>& context) {
    MIRBuilder builder(context->mir_module, context->symbol_tree);
    builder.run_build();
}

} // namespace nico
