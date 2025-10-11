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

#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/block.h"
#include "nico/frontend/utils/frontend_context.h"
#include "nico/shared/ir_module_context.h"
#include "nico/shared/token.h"

namespace nico {

/**
 * @brief A class to perform LLVM code generation.
 *
 * This class assumes that the AST has been type-checked.
 * It does not perform type-checking, it does not check for memory safety, and
 * it does not check for undefined behavior.
 */
class CodeGenerator : public Stmt::Visitor, public Expr::Visitor {
    // A static counter for generating unique names in REPL mode.
    static int repl_counter;

    // A flag to indicate whether IR should be printed just before verification.
    const bool ir_printing_enabled = false;
    // A flag to indicate whether panic is recoverable.
    // Can be set to true when testing panics.
    const bool panic_recoverable = false;
    // A flag to indicate whether we are generating code in REPL mode.
    const bool repl_mode = false;

    // The LLVM module and context used for code generation.
    IrModuleContext mod_ctx;
    // The IR builder used to generate the IR; always set the insertion point
    // before using it.
    std::unique_ptr<llvm::IRBuilder<>> builder;
    // A linked list of blocks for tracking control flow.
    std::shared_ptr<Block> block_list = nullptr;

    CodeGenerator(
        std::string_view module_name,
        bool ir_printing_enabled,
        bool panic_recoverable,
        bool repl_mode
    );

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
     * @brief Generates the LLVM IR for the script function.
     *
     * All of the AST statements in the context are processed here.
     *
     * In our programming language, code is executed from the top level instead
     * of an explicit "main" function. Internally, this code is put into a
     * special function called the script function, which can be called to "run"
     * the code.
     *
     * @param context The front end context containing the AST to generate IR.
     * @param script_fn_name The name of the script function to generate.
     * Defaults to "$script".
     */
    void generate_script_func(
        const std::unique_ptr<FrontendContext>& context,
        std::string_view script_fn_name = "$script"
    );

    /**
     * @brief Generates the LLVM IR for the main function.
     *
     * The main function is a special function that serves as the entry point
     * for the executable. Internally, it calls the script function to execute
     * the top-level code.
     *
     * The main function always has the type `i32 (i32, ptr)`.
     * It may be named "main" or something else.
     *
     * @param script_fn_name The name of the script function to call from main.
     * Should be the same as the name used in the call to generate_script.
     * Defaults to "$script".
     * @param main_fn_name The name of the main function. Defaults to "main".
     */
    void generate_main_func(
        std::string_view script_fn_name = "$script",
        std::string_view main_fn_name = "main"
    );

public:
    /**
     * @brief Generates the LLVM IR for an executable module from the given
     * front end context.
     *
     * Use only for AOT and JIT compilation modes. For REPL mode, use
     * `generate_repl_ir`.
     *
     * Once code generation is complete, the generated module and context
     * will be moved into the provided front end context. If code generation
     * fails, this function will panic. Ensure code is correct before calling
     * this function.
     *
     * If ir_printing_enabled is true, the generated IR will be printed to the
     * console just before verification. Useful for debugging.
     *
     * If panic_recoverable is true, the generated code will include mechanisms
     * to recover from panics using setjmp and longjmp. Useful for testing.
     *
     * If require_verification is true, the generated IR will be verified for
     * correctness. If verification fails, this function will panic.
     *
     * @param context The front end context containing the AST to generate IR
     * for.
     * @param ir_printing_enabled Whether to print the generated IR before
     * verification. Defaults to false.
     * @param panic_recoverable Whether to make panics recoverable using
     * setjmp and longjmp. Defaults to false.
     * @param module_name The name of the generated module. Defaults to "main".
     * @param require_verification Whether to verify the generated IR.
     * Defaults to true.
     */
    static void generate_exe_ir(
        std::unique_ptr<FrontendContext>& context,
        bool ir_printing_enabled = false,
        bool panic_recoverable = false,
        std::string_view module_name = "main",
        bool require_verification = true
    );

    /**
     * @brief Generates the LLVM IR for a REPL submission from the given
     * front end context.
     *
     * Use only for REPL mode. For other compilation modes, use
     * `generate_exe_ir`.
     *
     * Once code generation is complete, the generated module and context
     * will be moved into the provided front end context. If code generation
     * fails, this function will panic. Ensure code is correct before calling
     * this function.
     *
     * If ir_printing_enabled is true, the generated IR will be printed to the
     * console just before verification. Useful for debugging.
     *
     * @param context The front end context containing the AST to generate IR
     * for.
     * @param ir_printing_enabled Whether to print the generated IR before
     * verification. Defaults to false.
     * @param require_verification Whether to verify the generated IR.
     * Defaults to true.
     */
    static void generate_repl_ir(
        std::unique_ptr<FrontendContext>& context,
        bool ir_printing_enabled = false,
        bool require_verification = true
    );
};

} // namespace nico

#endif // NICO_CODE_GENERATOR_H
