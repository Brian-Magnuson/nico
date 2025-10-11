#ifndef NICO_GLOBAL_CHECKER_H
#define NICO_GLOBAL_CHECKER_H

#include <memory>
#include <vector>

#include "nico/frontend/utils/frontend_context.h"

namespace nico {

class GlobalChecker {
    GlobalChecker() = default;

    /**
     * @brief Type checks the given context at the global level.
     *
     * This function will modify the AST to add type information to the nodes.
     *
     * @param context The front end context containing the AST to type check.
     */
    void run_check(std::unique_ptr<FrontendContext>& context);

public:
    static void check(std::unique_ptr<FrontendContext>& context);
};

} // namespace nico

#endif // NICO_GLOBAL_CHECKER_H
