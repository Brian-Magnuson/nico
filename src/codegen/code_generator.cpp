#include "code_generator.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>

#include "../logger/logger.h"

bool CodeGenerator::generate(const std::vector<std::shared_ptr<Stmt>>& stmts, bool require_verification) {
    // Normally, we would traverse the AST and generate LLVM IR here.
    // For now, we will generate a simple script that prints "Hello, World!".

    llvm::FunctionType* script_fn_type = llvm::FunctionType::get(
        builder->getInt32Ty(), false
    );
    llvm::Function* script_fn = llvm::Function::Create(
        script_fn_type, llvm::Function::ExternalLinkage, "script", ir_module.get()
    );

    // Create a basic block for the script function
    llvm::BasicBlock* entry_block = llvm::BasicBlock::Create(*context, "entry", script_fn);
    llvm::BasicBlock* exit_block = llvm::BasicBlock::Create(*context, "exit", script_fn);

    // Start inserting instructions into the entry block.
    builder->SetInsertPoint(entry_block);

    // Allocate space for the return value.
    llvm::AllocaInst* ret_val = builder->CreateAlloca(builder->getInt32Ty(), nullptr, "$retval");

    // Append the exit block to the block list.
    block_list = std::make_shared<Block::Script>(block_list, ret_val, exit_block);

    // CODE STARTS HERE

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

    // CODE ENDS HERE

    // Assign to ret_val
    builder->CreateStore(builder->getInt32(0), ret_val);

    // Jump to exit block
    builder->CreateBr(exit_block);
    builder->SetInsertPoint(exit_block);

    // Return the value from ret_val
    builder->CreateRet(builder->CreateLoad(builder->getInt32Ty(), ret_val));

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

bool CodeGenerator::generate_main(bool require_verification) {
    // Generate the main function that calls $script
    llvm::FunctionType* main_fn_type = llvm::FunctionType::get(
        builder->getInt32Ty(),
        {builder->getInt32Ty(), builder->getPtrTy()},
        false
    );

    llvm::Function* main_fn = llvm::Function::Create(
        main_fn_type, llvm::Function::ExternalLinkage, "main", ir_module.get()
    );

    // Create a basic block for the main function
    llvm::BasicBlock* main_entry = llvm::BasicBlock::Create(*context, "entry", main_fn);
    builder->SetInsertPoint(main_entry);

    // Call the script function
    llvm::Function* script_fn = ir_module->getFunction("script");
    llvm::Value* script_ret = builder->CreateCall(script_fn);

    // We discard the args for now; later we can come up with a way to make them
    // accessible to the user.

    // Return the result
    builder->CreateRet(script_ret);

    ir_module->print(llvm::outs(), nullptr);

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
