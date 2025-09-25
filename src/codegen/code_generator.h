#ifndef NICO_CODE_GENERATOR_H
#define NICO_CODE_GENERATOR_H

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "../frontend/context.h"
#include "../lexer/token.h"
#include "../nodes/ast_node.h"
#include "../parser/ast.h"
#include "block.h"

/**
 * @brief A class to perform LLVM code generation.
 *
 * This class assumes that the AST has been type-checked.
 * It does not perform type-checking, it does not check for memory safety, and
 * it does not check for undefined behavior.
 */
class CodeGenerator : public Stmt::Visitor, public Expr::Visitor {
    // The LLVM context.
    std::unique_ptr<llvm::LLVMContext> llvm_context;
    // The LLVM Module that will be generated.
    std::unique_ptr<llvm::Module> ir_module;
    // The IR builder used to generate the IR; always set the insertion point
    // before using it.
    std::unique_ptr<llvm::IRBuilder<>> builder;
    // A linked list of blocks for tracking control flow.
    std::shared_ptr<Block> block_list = nullptr;
    // A flag to indicate whether panic is recoverable.
    // Can be set to true when testing panics.
    bool panic_recoverable = false;
    // A flag to indicate whether IR should be printed just before verification.
    bool ir_printing_enabled = false;

    std::any visit(Stmt::Expression* stmt) override;
    std::any visit(Stmt::Let* stmt) override;
    std::any visit(Stmt::Print* stmt) override;
    std::any visit(Stmt::Pass* stmt) override;
    std::any visit(Stmt::Yield* stmt) override;
    std::any visit(Stmt::Eof* stmt) override;

    std::any visit(Expr::Assign* expr, bool as_lvalue) override;
    std::any visit(Expr::Logical* expr, bool as_lvalue) override;
    std::any visit(Expr::Binary* expr, bool as_lvalue) override;
    std::any visit(Expr::Unary* expr, bool as_lvalue) override;
    std::any visit(Expr::Deref* expr, bool as_lvalue) override;
    std::any visit(Expr::Access* expr, bool as_lvalue) override;
    std::any visit(Expr::NameRef* expr, bool as_lvalue) override;
    std::any visit(Expr::Literal* expr, bool as_lvalue) override;
    std::any visit(Expr::Tuple* expr, bool as_lvalue) override;
    std::any visit(Expr::Block* expr, bool as_lvalue) override;
    std::any visit(Expr::Conditional* expr, bool as_lvalue) override;

    /**
     * @brief Adds C standard library functions to the module that are useful
     * for code generation.
     *
     * Includes the following functions:
     * `printf`,
     * `abort`,
     * `exit`,
     * `malloc`,
     * `free`
     *
     * If panic recoverable is enabled, the following are also included:
     * `setjmp`,
     * `longjmp`
     */
    void add_c_functions();

    /**
     * @brief Adds a runtime check for division by zero.
     *
     * This check generates code to compare the divisor against zero. If the
     * divisor is zero, the program will abort with an error message.
     *
     * @param divisor The divisor value to check.
     * @param location The location in the source code where the division
     * occurs, used for the panic message.
     */
    void add_div_zero_check(llvm::Value* divisor, const Location* location);

    /**
     * @brief Adds a panic call to the generated code.
     *
     * During a panic, the program will print the error message and immediately
     * terminate.
     *
     * The implementation of this may vary.
     *
     * @param message The error message to pass to the `panic` function.
     * @param location The location in the source code where the panic occurs,
     * used for the panic message.
     */
    void add_panic(std::string_view message, const Location* location);

public:
    /**
     * @brief Constructs a code generator with a new LLVM context and module.
     */
    CodeGenerator() { reset(); }

    // Move constructors and move assignment are disabled due to LLVM context
    // and module complexities.
    CodeGenerator(CodeGenerator&&) = delete;
    CodeGenerator(const CodeGenerator&) = delete;
    CodeGenerator& operator=(CodeGenerator&&) = delete;
    CodeGenerator& operator=(const CodeGenerator&) = delete;

    /**
     * @brief Generate the LLVM IR for the given frontend context.
     *
     * This method traverses the AST and generates the corresponding LLVM IR.
     * A `script` function will be generated, which can be called to "run" the
     * code.
     *
     * This method does not verify the generated IR; use `verify_ir` to do so.
     * If the goal is to generate a complete executable module, use
     * `generate_executable_ir` instead.
     *
     * @param context The front end context containing the AST to generate IR
     * for.
     */
    void generate_script(const std::unique_ptr<Context>& context);

    /**
     * @brief Generates the LLVM IR for the main function.
     *
     * The main function is used to make the module executable.
     * It calls a function named `$script`, which begins executing code
     * generated from top-level statements.
     *
     * This method does not verify the generated IR; use `verify_ir` to do so.
     * If the goal is to generate a complete executable module, use
     * `generate_executable_ir` instead.
     */
    void generate_main();

    /**
     * @brief Verify the generated LLVM IR for correctness.
     *
     * This method uses LLVM's built-in verification to check the generated IR
     * for correctness.
     *
     * @return True if the IR is valid, false otherwise.
     */
    bool verify_ir();

    /**
     * @brief Generates the LLVM IR for the script and main functions.
     *
     * This method has the same effect as calling `generate_script`,
     * `generate_main`, and `verify_ir` sequentially.
     *
     * If verification fails, an error message will be logged and false will
     be
     * returned.
     * Verification may be skipped by setting `require_verification` to
     false.
     * In such case, this function always returns true.
     *
     * The resulting module will contain a `main` function, allowing it to be
     * executed as a standalone program.
     *
     * @param context The front end context containing the AST to generate IR
     * for.
     * @param require_verification Whether to verify the generated IR. Defaults
     * to true.
     */
    void generate_executable_ir(
        const std::unique_ptr<Context>& context,
        bool require_verification = true
    );

    // /**
    //  * @brief Eject the generated LLVM module and context.
    //  *
    //  * This method moves the generated LLVM module and context into a
    //  * CodeGenerator::Output object and returns it.
    //  * After calling this method, the CodeGenerator object is in an invalid
    //  * state and should not be used.
    //  *
    //  * @return An Output object containing the generated LLVM module and
    //  * context.
    //  */
    // Output eject() { return Output(std::move(ir_module), std::move(context));
    // }

    /**
     * @brief Moves the generated LLVM module and context into the provided
     * front end context.
     *
     * Once the module and llvm context are moved into the front end context,
     * the code generator will be reset, allowing it to be reused.
     *
     * @param context The front end context to add the module and context to.
     */
    void add_module_to_context(std::unique_ptr<Context>& context);

    /**
     * @brief Reset the code generator to its initial state.
     *
     * This method creates a new LLVMContext object and Module object.
     *
     * Effectively equivalent to constructing a new CodeGenerator object.
     */
    void reset();

    /**
     * @brief Sets whether the code generator should use panic recovery.
     *
     * If setting panic recoverable, make sure to call this function before any
     * code is generated.
     *
     * Normally, panics cause the program to terminate. But this makes the
     * program difficult to test. So we provide a means to make a panic
     * "recoverable".
     *
     * When panic recoverable is true, the program will generate instructions to
     * call setjmp and longjmp. These behave similar to throw and catch in C++.
     *
     * @param value True to enable panic recovery, false to disable it.
     */
    void set_panic_recoverable(bool value) { panic_recoverable = value; }

    /**
     * @brief Sets whether the code generator should print the generated IR just
     * before verification.
     *
     * This is useful for debugging/testing purposes.
     * Make sure to call this function before any code is generated.
     *
     * @param value True to enable IR printing, false to disable it.
     */
    void set_ir_printing_enabled(bool value) { ir_printing_enabled = value; }
};

#endif // CODE_GENERATOR_H
