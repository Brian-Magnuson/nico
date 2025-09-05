#ifndef NICO_GLOBAL_CHECKER_H
#define NICO_GLOBAL_CHECKER_H

#include <memory>
#include <vector>

#include "../parser/ast.h"

class GlobalChecker {
public:
    GlobalChecker() = default;

    /**
     * @brief Type checks the given AST at the global level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param ast The AST to type check.
     */
    void check(Ast& ast) { return; }
};

#endif // NICO_GLOBAL_CHECKER_H
