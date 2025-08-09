#include "code_generator.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>

#include "../logger/logger.h"

bool CodeGenerator::generate(const std::vector<std::shared_ptr<Stmt>>& stmts, bool require_verification) {
    // Normally, we would traverse the AST and generate LLVM IR here.
    // For now, we will generate a simple main function that prints "Hello, World!".

    // Create the main function type: int main()
    llvm::FunctionType* main_fn_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context), false
    );
    llvm::Function* main_fn = llvm::Function::Create(
        main_fn_type, llvm::Function::ExternalLinkage, "main", ir_module.get()
    );

    // Create a basic block for the main function
    llvm::BasicBlock* entry_block = llvm::BasicBlock::Create(*context, "entry", main_fn);
    llvm::BasicBlock* exit_block = llvm::BasicBlock::Create(*context, "exit", main_fn);

    // Append the exit block to the block list.
    block_list = std::make_shared<Block>(block_list, exit_block, true);
    // Start inserting instructions into the entry block.
    builder->SetInsertPoint(entry_block);

    // Create a string constant "Hello, World!"
    llvm::Value* hello_world_str = builder->CreateGlobalStringPtr("Hello, World!");

    // Get the puts function
    llvm::FunctionType* puts_fn_type = llvm::FunctionType::get(
        builder->getInt32Ty(), builder->getPtrTy(), false
    );
    llvm::Function* puts_fn = llvm::Function::Create(
        puts_fn_type, llvm::Function::ExternalLinkage, "puts", ir_module.get()
    );

    // Call puts with the string constant
    builder->CreateCall(puts_fn, hello_world_str);

    // Return 0 from main
    builder->CreateRet(builder->getInt32(0));

    // If verification is required, verify the generated IR
    if (require_verification && llvm::verifyModule(*ir_module, &llvm::errs())) {
        Logger::inst().log_error(
            Err::ModuleVerificationFailed,
            "Generated LLVM IR failed verification."
        );
        return false;
    }

    return true;
}
