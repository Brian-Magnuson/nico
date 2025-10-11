#ifndef NICO_EMITTER_H
#define NICO_EMITTER_H

#include <memory>
#include <string_view>

#include <llvm/IR/Module.h>

namespace nico {

/**
 * @brief A class to emit the IR module to an object file.
 *
 * The object file can be linked to create an executable.
 */
class Emitter {
public:
    /**
     * @brief Emit the IR module to an object file.
     *
     * @param ir_module The IR module containing the IR to emit.
     * @param target_destination A string specifying the target destination for
     * the object file. E.g. "./bin/output.o". Paths are relative to CWD.
     */
    void emit(
        const std::unique_ptr<llvm::Module>& ir_module,
        std::string_view target_destination = "output.o"
    );
};

} // namespace nico

#endif // NICO_EMITTER_H
