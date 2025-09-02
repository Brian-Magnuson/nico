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

#include "../lexer/token.h"
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
    std::unique_ptr<llvm::LLVMContext> context;
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

    std::any visit(Stmt::Expression* stmt) override;
    std::any visit(Stmt::Let* stmt) override;
    std::any visit(Stmt::Print* stmt) override;
    std::any visit(Stmt::Yield* stmt) override;
    std::any visit(Stmt::Eof* stmt) override;

    std::any visit(Expr::Assign* expr, bool as_lvalue) override;
    std::any visit(Expr::Binary* expr, bool as_lvalue) override;
    std::any visit(Expr::Unary* expr, bool as_lvalue) override;
    std::any visit(Expr::NameRef* expr, bool as_lvalue) override;
    std::any visit(Expr::Literal* expr, bool as_lvalue) override;
    std::any visit(Expr::Tuple* expr, bool as_lvalue) override;
    std::any visit(Expr::Block* expr, bool as_lvalue) override;

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
     * @brief The output of the code generator, containing the generated LLVM
     * module and context.
     *
     * This struct has no additional functions; it is simply a wrapper for both
     * the module and context.
     *
     * Objects of this type are NOT copy-assignable/copy-constructible.
     * Use `std::move` to transfer ownership.
     */
    struct Output {
        // The generated LLVM module.
        std::unique_ptr<llvm::Module> module;
        // The LLVM context used to generate the module.
        std::unique_ptr<llvm::LLVMContext> context;

        Output(
            std::unique_ptr<llvm::Module> mod,
            std::unique_ptr<llvm::LLVMContext> ctx
        )
            : module(std::move(mod)), context(std::move(ctx)) {}
    };

    /**
     * @brief Constructs a code generator with a new LLVM context and module.
     */
    CodeGenerator() { reset(); }

    /**
     * @brief Generate the LLVM IR for the given AST statements.
     *
     * This method traverses the AST and generates the corresponding LLVM IR.
     * If `require_verification` is true, it will verify the generated IR for
     * correctness and return false if verification fails.
     *
     * @param stmts The statements to generate IR for.
     * @param require_verification Whether to verify the generated IR. Defaults
     * to true.
     * @return true if the IR was generated successfully, false otherwise.
     */
    bool generate(
        const std::vector<std::shared_ptr<Stmt>>& stmts,
        bool require_verification = true
    );

    /**
     * @brief Generates the LLVM IR for the main function.
     *
     * The main function is used to make the module executable.
     * It calls a function named `$script`, which begins executing code
     * generated from top-level statements.
     *
     * If `require_verification` is true, it will verify the generated IR for
     * correctness and return false if verification fails.
     *
     * @param require_verification Whether to verify the generated IR. Defaults
     * to true.
     * @return true if the main function was generated successfully, false
     * otherwise.
     */
    bool generate_main(bool require_verification = true);

    /**
     * @brief Eject the generated LLVM module and context.
     *
     * This method moves the generated LLVM module and context into a
     * CodeGenerator::Output object and returns it.
     * After calling this method, the CodeGenerator object is in an invalid
     * state and should not be used.
     *
     * @return An Output object containing the generated LLVM module and
     * context.
     */
    Output eject() { return Output(std::move(ir_module), std::move(context)); }

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
};

#endif // CODE_GENERATOR_H
