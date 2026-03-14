#ifndef NICO_TEST_JIT_H
#define NICO_TEST_JIT_H

#include "nico/backend/jit.h"

#include <llvm/Support/Error.h>

#include <string>

namespace nico {

/**
 * @brief A JIT class specialized for testing purposes. This class extends the
 * SimpleJIT implementation and adds functionality for loading static libraries,
 * which can be useful for testing interactions between JIT-compiled code and
 * external libraries.
 */
class TestJit : public SimpleJIT {
public:
    virtual ~TestJit() = default;

    /**
     * @brief Adds a static library to the JIT.
     *
     * Currently used only for testing purposes to test how external libraries
     * interact with JIT-compiled code.
     *
     * @param lib_path The path to the static library to be added. Paths are
     * relative to CWD.
     * @return An Error indicating success or failure of the operation.
     */
    llvm::Error add_static_library(const std::string& lib_path);
};

} // namespace nico

#endif // NICO_TEST_JIT_H
