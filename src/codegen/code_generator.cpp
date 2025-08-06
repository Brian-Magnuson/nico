#include "code_generator.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>

#include "../logger/logger.h"

bool CodeGenerator::generate(const std::vector<std::shared_ptr<Stmt>>& stmts, bool require_verification) {
    // Normally, we would traverse the AST and generate LLVM IR here.
    // For now, we will generate a simple main function that prints "Hello, World!".

    // Create the main function type: int main()
    llvm::FunctionType* mainFuncType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context), false
    );
    llvm::Function* mainFunc = llvm::Function::Create(
        mainFuncType, llvm::Function::ExternalLinkage, "main", ir_module.get()
    );

    // Create a basic block for the main function
    llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(*context, "entry", mainFunc);
    builder->SetInsertPoint(entryBlock);

    // Create a string constant "Hello, World!"
    llvm::Value* helloWorldStr = builder->CreateGlobalStringPtr("Hello, World!");

    // Get the puts function
    llvm::FunctionType* putsType = llvm::FunctionType::get(
        builder->getInt32Ty(), builder->getPtrTy(), false
    );
    llvm::Function* putsFunc = llvm::Function::Create(
        putsType, llvm::Function::ExternalLinkage, "puts", ir_module.get()
    );

    // Call puts with the string constant
    builder->CreateCall(putsFunc, helloWorldStr);

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
