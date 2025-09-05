#ifndef NICO_GLOBAL_CHECKER_H
#define NICO_GLOBAL_CHECKER_H

#include <memory>
#include <vector>

#include "../nodes/ast_node.h"
#include "symbol_tree.h"

class GlobalChecker {
    std::shared_ptr<SymbolTree> symbol_tree;

public:
    /**
     * @brief Constructs a new GlobalChecker.
     *
     * @param symbol_tree The symbol tree to use for type checking.
     */
    GlobalChecker(std::shared_ptr<SymbolTree> symbol_tree)
        : symbol_tree(symbol_tree) {}

    /**
     * @brief Type checks the given AST at the global level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param ast The list of statements to type check.
     */
    void check(std::vector<std::shared_ptr<Stmt>>& ast) { return; }
};

#endif // NICO_GLOBAL_CHECKER_H
