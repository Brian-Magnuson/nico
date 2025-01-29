#ifndef NICO_GLOBAL_CHECKER_H
#define NICO_GLOBAL_CHECKER_H

#include <memory>
#include <vector>

#include "../parser/stmt.h"

class GlobalChecker {
public:
    GlobalChecker() = default;

    /**
     * @brief Type checks the given AST at the global level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param ast The list of statements to type check.
     */
    void check(std::vector<std::shared_ptr<Stmt>> ast) { return; }

    // TODO: Consider adding `reset` function.
};

#endif // NICO_GLOBAL_CHECKER_H
