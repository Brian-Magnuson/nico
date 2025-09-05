#ifndef NICO_AST_H
#define NICO_AST_H

#include <memory>
#include <vector>

#include "../nodes/ast_node.h"
#include "../parser/symbol_tree.h"

/**
 * @brief An abstract syntax tree (AST) class to encapsulate both the AST nodes
 * and the symbol tree used for type checking.
 */
class Ast {
public:
    // The list of statements in the AST.
    std::vector<std::shared_ptr<Stmt>> stmts;
    // The symbol tree used for type checking.
    std::shared_ptr<SymbolTree> symbol_tree;

    Ast(std::vector<std::shared_ptr<Stmt>> stmts)
        : stmts(std::move(stmts)) {
        symbol_tree = std::make_shared<SymbolTree>();
    }
};

#endif // NICO_AST_H
