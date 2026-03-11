#include "test_jit.h"

#include <llvm/ExecutionEngine/Orc/LLJIT.h>

#include "nico/shared/utils.h"

namespace nico {

llvm::Error TestJit::add_static_library(const std::string& lib_path) {
    auto err =
        jit->linkStaticLibraryInto(jit->getMainJITDylib(), lib_path.c_str());
    if (err) {
        panic(
            "TestJit::add_static_library: Failed to add static library '" +
            lib_path + "' to JIT: " + llvm::toString(std::move(err))
        );
    }
    return llvm::Error::success();
}

} // namespace nico
