#include "nico/frontend/utils/nodes.h"

namespace nico {

bool Stmt::apply_modifier(const Modifier& modifier) {
    // General modifiers are handled here
    // But we don't have any right now, so just return false.
    // In the future, if we add general modifiers, we can check for them here.

    return false;
}

} // namespace nico
