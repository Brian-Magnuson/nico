#ifndef NICO_MIR_BUILDER_H
#define NICO_MIR_BUILDER_H

#include <any>
#include <memory>

#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/frontend_context.h"
#include "nico/shared/ir_module_context.h"

namespace nico {

class MIRBuilder {
public:
    /**
     * @brief Builds the MIR for the given front end context.
     *
     * @param context The front end context containing the AST to build MIR for.
     */
    static void build_mir(std::unique_ptr<FrontendContext>& context);
};

} // namespace nico

#endif // NICO_MIR_BUILDER_H
