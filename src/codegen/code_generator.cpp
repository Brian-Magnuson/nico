#include "code_generator.h"

#include <string_view>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>

#include "../common/utils.h"
#include "../logger/logger.h"

std::any CodeGenerator::visit(Stmt::Expression* stmt) {
    stmt->expression->accept(this, false);
    return std::any();
}

std::any CodeGenerator::visit(Stmt::Let* stmt) {
    auto llvm_type = stmt->field_entry.lock()->field.type->get_llvm_type(builder);
    llvm::Value* allocation = nullptr;

    if (PTR_INSTANCEOF(block_list, Block::Script)) {
        // If we are in a script, create a global variable instead of a local
        allocation = new llvm::GlobalVariable(
            *ir_module,
            llvm_type,
            false,
            llvm::GlobalValue::InternalLinkage,
            llvm::Constant::getNullValue(llvm_type), // We cannot assign non-constants here, so we use this instead
            stmt->identifier->lexeme
        );
    } else {
        allocation = builder->CreateAlloca(
            llvm_type,
            nullptr,
            stmt->identifier->lexeme
        );
    }

    stmt->field_entry.lock()->llvm_ptr = allocation;

    if (stmt->expression.has_value()) {
        // Here, storing a non-constant is okay.
        builder->CreateStore(
            std::any_cast<llvm::Value*>(stmt->expression.value()->accept(this, false)),
            allocation
        );
    }
    return std::any();
}

std::any CodeGenerator::visit(Stmt::Eof* stmt) {
    // Generate code for the end-of-file (EOF) statement
    return std::any();
}

std::any CodeGenerator::visit(Stmt::Print* stmt) {
    // Get printf
    llvm::Function* printf_fn = ir_module->getFunction("printf");
    if (!printf_fn) {
        llvm::FunctionType* printf_type = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(*context),
            {llvm::PointerType::get(*context, 0)},
            true // true = variadic
        );
        printf_fn = llvm::Function::Create(
            printf_type,
            llvm::Function::ExternalLinkage,
            "printf",
            *ir_module
        );
    }

    llvm::Value* format_str = nullptr;
    for (const auto& expr : stmt->expressions) {
        auto value = std::any_cast<llvm::Value*>(expr->accept(this, false));
        if (PTR_INSTANCEOF(expr->type, Type::Int)) {
            format_str = builder->CreateGlobalStringPtr("%d");
            builder->CreateCall(printf_fn, {format_str, value});
        } else if (PTR_INSTANCEOF(expr->type, Type::Float)) {
            format_str = builder->CreateGlobalStringPtr("%f");
            builder->CreateCall(printf_fn, {format_str, value});
        } else if (PTR_INSTANCEOF(expr->type, Type::Bool)) {
            format_str = builder->CreateGlobalStringPtr("%s");
            llvm::Value* bool_str = builder->CreateSelect(
                value,
                builder->CreateGlobalStringPtr("true"),
                builder->CreateGlobalStringPtr("false")
            );
            builder->CreateCall(printf_fn, {format_str, bool_str});
        } else if (PTR_INSTANCEOF(expr->type, Type::Str)) {
            format_str = builder->CreateGlobalStringPtr("%s");
            builder->CreateCall(printf_fn, {format_str, value});
        } else {
            panic("CodeGenerator::visit(Stmt::Print*): Cannot print expression of this type.");
        }
    }

    // Generate code for the print statement
    return std::any();
}

std::any CodeGenerator::visit(Expr::Assign* expr, bool as_lvalue) {
    // Generate code for the assignment expression
    return std::any();
}

std::any CodeGenerator::visit(Expr::Binary* expr, bool as_lvalue) {
    // Generate code for the binary expression
    return std::any();
}

std::any CodeGenerator::visit(Expr::Unary* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;
    auto value = std::any_cast<llvm::Value*>(expr->right->accept(this, false));
    if (expr->op->tok_type == Tok::Minus) {
        if (PTR_INSTANCEOF(expr->type, Type::Int)) {
            result = builder->CreateNeg(value);
        } else if (PTR_INSTANCEOF(expr->type, Type::Float)) {
            result = builder->CreateFNeg(value);
        } else {
            panic("CodeGenerator::visit(Expr::Unary*): Cannot negate value of this type.");
        }
    } else {
        panic("CodeGenerator::visit(Expr::Unary*): Unknown unary operator.");
    }

    return result;
}

std::any CodeGenerator::visit(Expr::NameRef* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;

    if (as_lvalue) {
        // We use the pointer to the variable (its alloca inst or global ptr)
        result = expr->field_entry.lock()->llvm_ptr;
    } else {
        // We load the value from the variable's memory location
        result = builder->CreateLoad(expr->type->get_llvm_type(builder), expr->field_entry.lock()->llvm_ptr);
    }

    return result;
}

std::any CodeGenerator::visit(Expr::Literal* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;

    switch (expr->token->tok_type) {
    case Tok::Int: {
        result = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(*context),
            std::any_cast<int32_t>(expr->token->literal)
        );
        break;
    }
    case Tok::Float:
        if (expr->token->lexeme == "inf") {
            result = llvm::ConstantFP::getInfinity(
                llvm::Type::getDoubleTy(*context)
            );
        } else if (expr->token->lexeme == "NaN") {
            result = llvm::ConstantFP::getNaN(
                llvm::Type::getDoubleTy(*context)
            );
        } else {
            result = llvm::ConstantFP::get(
                llvm::Type::getDoubleTy(*context),
                std::any_cast<double>(expr->token->literal)
            );
        }
        break;
    case Tok::Bool:
        result = llvm::ConstantInt::get(
            llvm::Type::getInt1Ty(*context),
            expr->token->lexeme == "true"
        );
        break;
    case Tok::Str:
        result = builder->CreateGlobalStringPtr(std::any_cast<std::string>(expr->token->literal));
        break;
    default:
        panic("CodeGenerator::visit(Expr::Literal*): Unknown literal type.");
    }

    // Generate code for the literal expression
    return result;
}

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

    for (auto& stmt : stmts) {
        stmt->accept(this);
    }

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

    // ir_module->print(llvm::outs(), nullptr);

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
