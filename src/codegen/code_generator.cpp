#include "code_generator.h"

#include <string_view>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>

#include "../common/utils.h"
#include "../logger/logger.h"

void CodeGenerator::add_c_functions() {
    // stderr
    if (!ir_module->getGlobalVariable("stderr")) {
        llvm::PointerType* file_ptr_type = llvm::PointerType::get(*context, 0);
        new llvm::GlobalVariable(
            *ir_module,
            file_ptr_type,
            true, // isConstant
            llvm::GlobalValue::ExternalLinkage,
            nullptr,
            "stderr"
        );
    }
    // printf
    if (!ir_module->getFunction("printf")) {
        llvm::FunctionType* printf_type = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(*context),
            {llvm::PointerType::get(*context, 0)},
            true // true = variadic
        );
        llvm::Function::Create(
            printf_type,
            llvm::Function::ExternalLinkage,
            "printf",
            *ir_module
        );
    }
    // fprintf
    if (!ir_module->getFunction("fprintf")) {
        llvm::FunctionType* fprintf_type = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(*context),
            {llvm::PointerType::get(*context, 0), llvm::PointerType::get(*context, 0)},
            true // true = variadic
        );
        llvm::Function::Create(
            fprintf_type,
            llvm::Function::ExternalLinkage,
            "fprintf",
            *ir_module
        );
    }
    // abort
    if (!ir_module->getFunction("abort")) {
        llvm::FunctionType* abort_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context),
            {},
            false
        );
        llvm::Function::Create(
            abort_type,
            llvm::Function::ExternalLinkage,
            "abort",
            *ir_module
        );
    }
    // exit
    if (!ir_module->getFunction("exit")) {
        llvm::FunctionType* exit_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context),
            {llvm::Type::getInt32Ty(*context)},
            false
        );
        llvm::Function::Create(
            exit_type,
            llvm::Function::ExternalLinkage,
            "exit",
            *ir_module
        );
    }
    // malloc
    if (!ir_module->getFunction("malloc")) {
        llvm::FunctionType* malloc_type = llvm::FunctionType::get(
            llvm::PointerType::get(*context, 0),
            {llvm::Type::getIntNTy(*context, sizeof(size_t) * 8)},
            false
        );
        llvm::Function::Create(
            malloc_type,
            llvm::Function::ExternalLinkage,
            "malloc",
            *ir_module
        );
    }
    // free
    if (!ir_module->getFunction("free")) {
        llvm::FunctionType* free_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context),
            {llvm::PointerType::get(*context, 0)},
            false
        );
        llvm::Function::Create(
            free_type,
            llvm::Function::ExternalLinkage,
            "free",
            *ir_module
        );
    }
    if (panic_recoverable) {
        // jmp_buf
        if (!ir_module->getGlobalVariable("jmp_buf", true)) {
            llvm::ArrayType* jmp_buf_type = llvm::ArrayType::get(llvm::Type::getInt8Ty(*context), 256);
            new llvm::GlobalVariable(
                *ir_module,
                jmp_buf_type,
                false,
                llvm::GlobalValue::InternalLinkage,
                llvm::Constant::getNullValue(jmp_buf_type),
                "jmp_buf"
            );
        }

        // setjmp (panic recoverable only)
        if (!ir_module->getFunction("setjmp")) {
            llvm::FunctionType* setjmp_type = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(*context),
                {llvm::PointerType::get(*context, 0)},
                false
            );
            llvm::Function::Create(
                setjmp_type,
                llvm::Function::ExternalLinkage,
                "setjmp",
                *ir_module
            );
        }
        // longjmp (panic recoverable only)
        if (!ir_module->getFunction("longjmp")) {
            llvm::FunctionType* longjmp_type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(*context),
                {llvm::PointerType::get(*context, 0), llvm::Type::getInt32Ty(*context)},
                false
            );
            llvm::Function::Create(
                longjmp_type,
                llvm::Function::ExternalLinkage,
                "longjmp",
                *ir_module
            );
        }
    }
}

void CodeGenerator::add_div_zero_check(llvm::Value* divisor, const Location* location) {
    llvm::BasicBlock* current_block = builder->GetInsertBlock();
    llvm::Function* current_function = current_block->getParent();

    llvm::BasicBlock* div_by_zero_block = llvm::BasicBlock::Create(*context, "div_by_zero", current_function);
    llvm::BasicBlock* div_ok_block = llvm::BasicBlock::Create(*context, "div_ok", current_function);

    llvm::Value* is_zero = builder->CreateICmpEQ(
        divisor,
        llvm::ConstantInt::get(divisor->getType(), 0)
    );
    builder->CreateCondBr(is_zero, div_by_zero_block, div_ok_block);

    // div_by_zero_block
    builder->SetInsertPoint(div_by_zero_block);
    add_panic("Division by zero.", location);
    builder->CreateUnreachable();

    // div_ok_block
    builder->SetInsertPoint(div_ok_block);
}

void CodeGenerator::add_panic(std::string_view message, const Location* location) {
    llvm::Value* jmp_buf_ptr;
    if (panic_recoverable) {
        llvm::GlobalVariable* jmp_buf_global = ir_module->getGlobalVariable("jmp_buf", true);
        jmp_buf_ptr = builder->CreateBitCast(jmp_buf_global, llvm::PointerType::get(*context, 0));
    }
    auto fprintf_fn = ir_module->getFunction("fprintf");
    llvm::Value* stderr_stream = builder->CreateLoad(
        llvm::PointerType::get(*context, 0),
        ir_module->getGlobalVariable("stderr")
    );
    llvm::Value* format_string = builder->CreateGlobalStringPtr("Panic: %s: %s\n%s:%d:%d\n");
    llvm::Value* func_name = builder->CreateGlobalStringPtr(block_list->get_function_name());
    llvm::Value* msg = builder->CreateGlobalStringPtr(message);
    auto location_tuple = location->to_tuple();
    llvm::Value* file_name = builder->CreateGlobalStringPtr(std::get<0>(location_tuple));
    llvm::Value* line_number = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), std::get<1>(location_tuple));
    llvm::Value* column_number = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), std::get<2>(location_tuple));
    builder->CreateCall(fprintf_fn, {stderr_stream, format_string, func_name, msg, file_name, line_number, column_number});

    if (panic_recoverable) {
        auto longjmp_fn = ir_module->getFunction("longjmp");
        builder->CreateCall(longjmp_fn, {jmp_buf_ptr, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 1)});
    } else {
        auto abort_fn = ir_module->getFunction("abort");
        builder->CreateCall(abort_fn);
    }
}

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
    llvm::Function* printf_fn = ir_module->getFunction("printf");

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
    llvm::Value* result = nullptr;
    auto left = std::any_cast<llvm::Value*>(expr->left->accept(this, false));
    auto right = std::any_cast<llvm::Value*>(expr->right->accept(this, false));

    if (PTR_INSTANCEOF(expr->type, Type::Float)) {
        switch (expr->op->tok_type) {
        case Tok::Plus:
            result = builder->CreateFAdd(left, right);
            break;
        case Tok::Minus:
            result = builder->CreateFSub(left, right);
            break;
        case Tok::Star:
            result = builder->CreateFMul(left, right);
            break;
        case Tok::Slash:
            result = builder->CreateFDiv(left, right);
            break;
        default:
            panic("CodeGenerator::visit(Expr::Binary*): Unknown binary operator for floating-point number.");
        }
    } else if (PTR_INSTANCEOF(expr->type, Type::Int)) {
        switch (expr->op->tok_type) {
        case Tok::Plus:
            result = builder->CreateAdd(left, right);
            break;
        case Tok::Minus:
            result = builder->CreateSub(left, right);
            break;
        case Tok::Star:
            result = builder->CreateMul(left, right);
            break;
        case Tok::Slash:
            add_div_zero_check(right, &expr->op->location);
            result = builder->CreateSDiv(left, right);
            break;
        default:
            panic("CodeGenerator::visit(Expr::Binary*): Unknown binary operator for integer.");
        }
    } else {
        panic("CodeGenerator::visit(Expr::Binary*): Unsupported type for binary operation.");
    }

    return result;
}

std::any CodeGenerator::visit(Expr::Unary* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;
    auto right = std::any_cast<llvm::Value*>(expr->right->accept(this, false));
    if (PTR_INSTANCEOF(expr->type, Type::Float)) {
        switch (expr->op->tok_type) {
        case Tok::Minus:
            result = builder->CreateFNeg(right);
            break;
        default:
            panic("CodeGenerator::visit(Expr::Unary*): Unknown unary operator for floating-point number.");
        }
    } else if (PTR_INSTANCEOF(expr->type, Type::Int)) {
        switch (expr->op->tok_type) {
        case Tok::Minus:
            result = builder->CreateNeg(right);
            break;
        default:
            panic("CodeGenerator::visit(Expr::Unary*): Unknown unary operator for integer.");
        }
    } else {
        panic("CodeGenerator::visit(Expr::Unary*): Unsupported type for unary operation.");
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

    add_c_functions();

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

    // Set panic recoverable code.
    if (panic_recoverable) {
        // Get jmp_buf
        llvm::GlobalVariable* jmp_buf_global = ir_module->getGlobalVariable("jmp_buf", true);
        llvm::Value* jmp_buf_ptr = builder->CreateBitCast(jmp_buf_global, llvm::PointerType::get(*context, 0));

        // Call setjmp
        llvm::Function* setjmp_fn = ir_module->getFunction("setjmp");
        llvm::Value* setjmp_ret = builder->CreateCall(setjmp_fn, {jmp_buf_ptr});
        llvm::Value* is_setjmp = builder->CreateICmpNE(setjmp_ret, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));

        // Create panic and normal blocks
        llvm::BasicBlock* panic_block = llvm::BasicBlock::Create(*context, "panic", script_fn);
        llvm::BasicBlock* normal_block = llvm::BasicBlock::Create(*context, "normal", script_fn);

        builder->CreateCondBr(is_setjmp, panic_block, normal_block);

        // When longjmp is called, we jump to here and return 101.
        builder->SetInsertPoint(panic_block);
        builder->CreateStore(builder->getInt32(101), ret_val);
        builder->CreateBr(exit_block);

        // Normal code continues from here.
        builder->SetInsertPoint(normal_block);
    }

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

void CodeGenerator::reset() {
    context = std::make_unique<llvm::LLVMContext>();
    ir_module = std::make_unique<llvm::Module>("main", *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
    block_list = nullptr;
    panic_recoverable = false;
}
