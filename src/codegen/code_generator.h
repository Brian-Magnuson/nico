#ifndef NICO_CODE_GENERATOR_H
#define NICO_CODE_GENERATOR_H

#include "../parser/stmt.h"

/**
 * @brief A class to perform LLVM code generation.
 *
 * This class assumes that the AST has been type-checked.
 * It does not perform type-checking, it does not check for memory safety, and it does not check for undefined behavior.
 */
class CodeGenerator {
public:
    CodeGenerator() = default;
};

#endif // CODE_GENERATOR_H
