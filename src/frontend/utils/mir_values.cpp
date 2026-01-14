#include "nico/frontend/utils/mir_values.h"

namespace nico {

std::unordered_map<std::string, size_t>
    MIRValue::Temporary::mir_temp_name_counters;

} // namespace nico
