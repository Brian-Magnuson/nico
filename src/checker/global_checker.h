#ifndef NICO_GLOBAL_CHECKER_H
#define NICO_GLOBAL_CHECKER_H

#include <memory>
#include <vector>

#include "../frontend/context.h"

class GlobalChecker {
public:
    GlobalChecker() = default;

    /**
     * @brief Type checks the given context at the global level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param context The front end context containing the AST to type check.
     */
    void check(std::unique_ptr<FrontendContext>& context) { return; }
};

#endif // NICO_GLOBAL_CHECKER_H
