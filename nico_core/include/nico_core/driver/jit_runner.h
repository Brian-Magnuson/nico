#ifndef NICO_CORE_JIT_RUNNER_H
#define NICO_CORE_JIT_RUNNER_H

#include <string_view>

namespace nico {

void compile_and_run(std::string_view file_name);

} // namespace nico

#endif // NICO_CORE_JIT_RUNNER_H
