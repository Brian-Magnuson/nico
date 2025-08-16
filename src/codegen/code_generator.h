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

#include "../parser/ast.h"
#include "block.h"

/**
 * @brief A class to perform LLVM code generation.
 *
 * This class assumes that the AST has been type-checked.
 * It does not perform type-checking, it does not check for memory safety, and it does not check for undefined behavior.
 */
class CodeGenerator : public Stmt::Visitor, public Expr::Visitor {

    // /**
    //  * @brief A block entry for a list of basic blocks.
    //  */
    // struct Block {
    //     // Whether this block is an exit block.
    //     // Return statements use these blocks to exit a function.
    //     bool is_exit;
    //     // The llvm block pointer.
    //     llvm::BasicBlock* block;
    //     // A pointer to the previous block.
    //     std::shared_ptr<Block> prev;
    //     // A pointer to the previous exit block.
    //     std::shared_ptr<Block> prev_exit;

    //     Block(std::shared_ptr<Block> prev, llvm::BasicBlock* block, bool is_exit) {
    //         this->block = block;
    //         this->is_exit = is_exit;
    //         this->prev = prev;

    //         // If the previous block exists and is an exit block...
    //         if (this->prev && this->prev->is_exit) {
    //             // Assign the prev exit block pointer accordingly.
    //             this->prev_exit = this->prev;
    //         }
    //         // If the previous block exists, but is not an exit...
    //         else if (this->prev) {
    //             // Assign the prev exit block pointer to the previous exit block.
    //             this->prev_exit = this->prev->prev_exit;
    //         }
    //         // If prev is null (this is the first block in the list)
    //         else {
    //             this->prev_exit = nullptr;
    //         }
    //     }
    // };

    // The LLVM context.
    std::unique_ptr<llvm::LLVMContext> context;
    // The LLVM Module that will be generated.
    std::unique_ptr<llvm::Module> ir_module;
    // The IR builder used to generate the IR; always set the insertion point before using it.
    std::unique_ptr<llvm::IRBuilder<>> builder;
    // A linked list of blocks for tracking control flow.
    std::shared_ptr<Block> block_list = nullptr;

    std::any visit(Stmt::Expression* stmt) override;
    std::any visit(Stmt::Let* stmt) override;
    std::any visit(Stmt::Eof* stmt) override;
    std::any visit(Stmt::Print* stmt) override;

    std::any visit(Expr::Assign* expr, bool as_lvalue) override;
    std::any visit(Expr::Binary* expr, bool as_lvalue) override;
    std::any visit(Expr::Unary* expr, bool as_lvalue) override;
    std::any visit(Expr::Identifier* expr, bool as_lvalue) override;
    std::any visit(Expr::Literal* expr, bool as_lvalue) override;

public:
    /**
     * @brief The output of the code generator, containing the generated LLVM module and context.
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

        Output(std::unique_ptr<llvm::Module> mod, std::unique_ptr<llvm::LLVMContext> ctx)
            : module(std::move(mod)), context(std::move(ctx)) {}
    };

    CodeGenerator() {
        context = std::make_unique<llvm::LLVMContext>();
        ir_module = std::make_unique<llvm::Module>("main", *context);
        builder = std::make_unique<llvm::IRBuilder<>>(*context);
    }

    /**
     * @brief Generate the LLVM IR for the given AST statements.
     *
     * This method traverses the AST and generates the corresponding LLVM IR.
     * If `require_verification` is true, it will verify the generated IR for
     * correctness and return false if verification fails.
     *
     * @param stmts The statements to generate IR for.
     * @param require_verification Whether to verify the generated IR. Defaults to true.
     * @return true if the IR was generated successfully, false otherwise.
     */
    bool generate(
        const std::vector<std::shared_ptr<Stmt>>& stmts,
        bool require_verification = true
    );

    /**
     * @brief Eject the generated LLVM module and context.
     *
     * This method moves the generated LLVM module and context into a
     * CodeGenerator::Output object and returns it.
     * After calling this method, the CodeGenerator object is in an invalid
     * state and should not be used.
     *
     * @return An Output object containing the generated LLVM module and context.
     */
    Output eject() {
        return Output(std::move(ir_module), std::move(context));
    }
};

#endif // CODE_GENERATOR_H
