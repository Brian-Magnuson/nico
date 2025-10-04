#include "nico/frontend/components/code_generator.h"

#include <iostream>
#include <string_view>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>

#include "nico/frontend/utils/symbol_node.h"
#include "nico/frontend/utils/type_node.h"
#include "nico/shared/logger.h"
#include "nico/shared/status.h"
#include "nico/shared/utils.h"

int CodeGenerator::repl_counter = 0;

CodeGenerator::CodeGenerator(
    std::string_view module_name,
    bool ir_printing_enabled,
    bool panic_recoverable,
    bool repl_mode
)
    : ir_printing_enabled(ir_printing_enabled),
      panic_recoverable(panic_recoverable),
      repl_mode(repl_mode),
      mod_ctx() {

    mod_ctx.initialize(module_name);
    builder = std::make_unique<llvm::IRBuilder<>>(*mod_ctx.llvm_context);
}

// MARK: Statements

std::any CodeGenerator::visit(Stmt::Expression* stmt) {
    stmt->expression->accept(this, false);
    return std::any();
}

std::any CodeGenerator::visit(Stmt::Let* stmt) {
    auto llvm_type =
        stmt->field_entry.lock()->field.type->get_llvm_type(builder);
    llvm::Value* allocation = nullptr;

    if (PTR_INSTANCEOF(block_list, Block::Script)) {
        // In REPL mode, global variables have external linkage so that they can
        // be accessed across multiple submissions.
        auto linkage = repl_mode ? llvm::GlobalValue::ExternalLinkage
                                 : llvm::GlobalValue::InternalLinkage;

        // If we are in a script, create a global variable instead of a local
        allocation = new llvm::GlobalVariable(
            *mod_ctx.ir_module,
            llvm_type,
            false,
            linkage,
            llvm::Constant::getNullValue(
                llvm_type
            ), // We cannot assign non-constants here, so we use this instead
            stmt->identifier->lexeme
        );
    }
    else {
        allocation =
            builder->CreateAlloca(llvm_type, nullptr, stmt->identifier->lexeme);
    }

    stmt->field_entry.lock()->llvm_ptr = allocation;

    if (stmt->expression.has_value()) {
        // Here, storing a non-constant is okay.
        builder->CreateStore(
            std::any_cast<llvm::Value*>(
                stmt->expression.value()->accept(this, false)
            ),
            allocation
        );
    }
    return std::any();
}

std::any CodeGenerator::visit(Stmt::Print* stmt) {
    llvm::Function* printf_fn = mod_ctx.ir_module->getFunction("printf");

    llvm::Value* format_str = nullptr;
    for (const auto& expr : stmt->expressions) {
        auto value = std::any_cast<llvm::Value*>(expr->accept(this, false));
        if (PTR_INSTANCEOF(expr->type, Type::Int)) {
            format_str = builder->CreateGlobalStringPtr("%d");
            builder->CreateCall(printf_fn, {format_str, value});
        }
        else if (PTR_INSTANCEOF(expr->type, Type::Float)) {
            format_str = builder->CreateGlobalStringPtr("%g");
            builder->CreateCall(printf_fn, {format_str, value});
        }
        else if (PTR_INSTANCEOF(expr->type, Type::Bool)) {
            format_str = builder->CreateGlobalStringPtr("%s");
            llvm::Value* bool_str = builder->CreateSelect(
                value,
                builder->CreateGlobalStringPtr("true"),
                builder->CreateGlobalStringPtr("false")
            );
            builder->CreateCall(printf_fn, {format_str, bool_str});
        }
        else if (PTR_INSTANCEOF(expr->type, Type::Str)) {
            format_str = builder->CreateGlobalStringPtr("%s");
            builder->CreateCall(printf_fn, {format_str, value});
        }
        else {
            panic(
                "CodeGenerator::visit(Stmt::Print*): Cannot print expression "
                "of this type."
            );
        }
    }

    // Generate code for the print statement
    return std::any();
}

std::any CodeGenerator::visit(Stmt::Pass* /*stmt*/) {
    // A pass statement does nothing.
    return std::any();
}

std::any CodeGenerator::visit(Stmt::Yield* stmt) {
    // return stmt->expression->accept(this, false);
    auto value =
        std::any_cast<llvm::Value*>(stmt->expression->accept(this, false));
    builder->CreateStore(value, block_list->yield_allocation);
    return std::any();
}

std::any CodeGenerator::visit(Stmt::Eof* stmt) {
    // Generate code for the end-of-file (EOF) statement
    return std::any();
}

// MARK: Expressions

std::any CodeGenerator::visit(Expr::Assign* expr, bool as_lvalue) {
    auto left_ptr = std::any_cast<llvm::Value*>(expr->left->accept(this, true));
    auto right = std::any_cast<llvm::Value*>(expr->right->accept(this, false));

    builder->CreateStore(right, left_ptr);
    return right;
}

std::any CodeGenerator::visit(Expr::Logical* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;
    llvm::BasicBlock* current_block = builder->GetInsertBlock();
    llvm::Function* current_function = current_block->getParent();

    llvm::BasicBlock* then_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "logic_then",
        current_function
    );
    llvm::BasicBlock* skip_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "logic_skip",
        current_function
    );
    llvm::BasicBlock* merge_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "logic_end",
        current_function
    );

    auto left = std::any_cast<llvm::Value*>(expr->left->accept(this, false));

    // This will store the value to use when skipping the right side; either
    // true or false
    llvm::Value* skip_val = nullptr;

    if (expr->op->tok_type == Tok::KwAnd) {
        skip_val = llvm::ConstantInt::getFalse(*mod_ctx.llvm_context);
        builder->CreateCondBr(left, then_block, skip_block);
    }
    else if (expr->op->tok_type == Tok::KwOr) {
        skip_val = llvm::ConstantInt::getTrue(*mod_ctx.llvm_context);
        builder->CreateCondBr(left, skip_block, then_block);
    }
    else {
        panic(
            "CodeGenerator::visit(Expr::Logical*): Unknown logical operator."
        );
        return std::any();
    }

    // then_block: evaluate right
    builder->SetInsertPoint(then_block);
    auto right_val =
        std::any_cast<llvm::Value*>(expr->right->accept(this, false));
    llvm::BasicBlock* right_block = builder->GetInsertBlock();
    builder->CreateBr(merge_block);

    // skip_block: jumps to merge (used for phi node)
    builder->SetInsertPoint(skip_block);
    builder->CreateBr(merge_block);

    // merge_block: phi node to select the correct value
    builder->SetInsertPoint(merge_block);
    llvm::PHINode* phi =
        builder->CreatePHI(expr->type->get_llvm_type(builder), 2);
    phi->addIncoming(right_val, right_block);
    phi->addIncoming(skip_val, skip_block);
    result = phi;
    return result;
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
            panic(
                "CodeGenerator::visit(Expr::Binary*): Unknown binary operator "
                "for floating-point number."
            );
        }
    }
    else if (PTR_INSTANCEOF(expr->type, Type::Int)) {
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
            panic(
                "CodeGenerator::visit(Expr::Binary*): Unknown binary operator "
                "for integer."
            );
        }
    }
    else {
        panic(
            "CodeGenerator::visit(Expr::Binary*): Unsupported type for binary "
            "operation."
        );
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
            panic(
                "CodeGenerator::visit(Expr::Unary*): Unknown unary operator "
                "for floating-point number."
            );
        }
    }
    else if (PTR_INSTANCEOF(expr->type, Type::Int)) {
        switch (expr->op->tok_type) {
        case Tok::Minus:
            result = builder->CreateNeg(right);
            break;
        default:
            panic(
                "CodeGenerator::visit(Expr::Unary*): Unknown unary operator "
                "for integer."
            );
        }
    }
    else {
        panic(
            "CodeGenerator::visit(Expr::Unary*): Unsupported type for unary "
            "operation."
        );
    }

    return result;
}

std::any CodeGenerator::visit(Expr::Deref* expr, bool as_lvalue) {
    // TODO: Implement dereference expressions.
    panic("CodeGenerator::visit(Expr::Deref*): Not implemented yet.");
    return std::any();
}

std::any CodeGenerator::visit(Expr::Access* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;

    auto left = std::any_cast<llvm::Value*>(expr->left->accept(this, true));
    auto struct_type =
        llvm::cast<llvm::StructType>(expr->left->type->get_llvm_type(builder));

    if (PTR_INSTANCEOF(expr->left->type, Type::Tuple)) {
        llvm::Value* element_ptr = builder->CreateStructGEP(
            struct_type,
            left,
            std::any_cast<size_t>(expr->right_token->literal),
            "tuple_element"
        );

        if (as_lvalue) {
            result = element_ptr;
        }
        else {
            result = builder->CreateLoad(
                expr->type->get_llvm_type(builder),
                element_ptr
            );
        }
    }
    else {
        panic(
            "CodeGenerator::visit(Expr::Access*): Accessing fields of this "
            "type is not supported yet."
        );
    }
    return result;
}

std::any CodeGenerator::visit(Expr::NameRef* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;

    if (as_lvalue) {
        // We use the pointer to the variable (its alloca inst or global ptr)
        result = expr->field_entry.lock()->llvm_ptr;
    }
    else {
        // We load the value from the variable's memory location
        result = builder->CreateLoad(
            expr->type->get_llvm_type(builder),
            expr->field_entry.lock()->llvm_ptr
        );
    }

    return result;
}

std::any CodeGenerator::visit(Expr::Literal* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;

    switch (expr->token->tok_type) {
    case Tok::Int: {
        result = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(*mod_ctx.llvm_context),
            std::any_cast<int32_t>(expr->token->literal)
        );
        break;
    }
    case Tok::Float:
        if (expr->token->lexeme == "inf") {
            result = llvm::ConstantFP::getInfinity(
                llvm::Type::getDoubleTy(*mod_ctx.llvm_context)
            );
        }
        else if (expr->token->lexeme == "NaN") {
            result = llvm::ConstantFP::getNaN(
                llvm::Type::getDoubleTy(*mod_ctx.llvm_context)
            );
        }
        else {
            result = llvm::ConstantFP::get(
                llvm::Type::getDoubleTy(*mod_ctx.llvm_context),
                std::any_cast<double>(expr->token->literal)
            );
        }
        break;
    case Tok::Bool:
        result = llvm::ConstantInt::get(
            llvm::Type::getInt1Ty(*mod_ctx.llvm_context),
            expr->token->lexeme == "true"
        );
        break;
    case Tok::Str:
        result = builder->CreateGlobalStringPtr(
            std::any_cast<std::string>(expr->token->literal)
        );
        break;
    default:
        panic("CodeGenerator::visit(Expr::Literal*): Unknown literal type.");
    }

    // Generate code for the literal expression
    return result;
}

std::any CodeGenerator::visit(Expr::Tuple* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;
    std::vector<llvm::Value*> element_values;
    for (const auto& element : expr->elements) {
        element_values.push_back(
            std::any_cast<llvm::Value*>(element->accept(this, false))
        );
    }
    llvm::StructType* tuple_type =
        llvm::cast<llvm::StructType>(expr->type->get_llvm_type(builder));
    llvm::Value* tuple_alloc =
        builder->CreateAlloca(tuple_type, nullptr, "tuple");

    for (size_t i = 0; i < element_values.size(); ++i) {
        llvm::Value* element_ptr =
            builder->CreateStructGEP(tuple_type, tuple_alloc, i, "element");
        builder->CreateStore(element_values[i], element_ptr);
    }

    result = builder->CreateLoad(tuple_type, tuple_alloc);
    return result;
}

std::any CodeGenerator::visit(Expr::Block* expr, bool as_lvalue) {
    llvm::Value* yield_allocation = builder->CreateAlloca(
        expr->type->get_llvm_type(builder),
        nullptr,
        "$yieldval"
    );
    block_list = std::make_shared<Block::Plain>(block_list, yield_allocation);

    for (auto& stmt : expr->statements) {
        stmt->accept(this);
    }

    block_list = block_list->prev;
    llvm::Value* yield_value = builder->CreateLoad(
        expr->type->get_llvm_type(builder),
        yield_allocation
    );

    return yield_value;
}

std::any CodeGenerator::visit(Expr::Conditional* expr, bool as_lvalue) {
    llvm::Value* yield_allocation = builder->CreateAlloca(
        expr->type->get_llvm_type(builder),
        nullptr,
        "$yieldval"
    );
    llvm::BasicBlock* current_block = builder->GetInsertBlock();
    llvm::Function* current_function = current_block->getParent();

    llvm::BasicBlock* then_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "cond_then",
        current_function
    );
    llvm::BasicBlock* else_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "cond_else",
        current_function
    );
    llvm::BasicBlock* merge_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "cond_end",
        current_function
    );

    auto condition =
        std::any_cast<llvm::Value*>(expr->condition->accept(this, false));
    builder->CreateCondBr(condition, then_block, else_block);

    // Then block
    builder->SetInsertPoint(then_block);
    auto then_value =
        std::any_cast<llvm::Value*>(expr->then_branch->accept(this, false));
    builder->CreateStore(then_value, yield_allocation);
    builder->CreateBr(merge_block);

    // Else block
    builder->SetInsertPoint(else_block);
    auto else_value =
        std::any_cast<llvm::Value*>(expr->else_branch->accept(this, false));
    builder->CreateStore(else_value, yield_allocation);
    builder->CreateBr(merge_block);

    // Merge block
    builder->SetInsertPoint(merge_block);
    llvm::Value* yield_value = builder->CreateLoad(
        expr->type->get_llvm_type(builder),
        yield_allocation
    );

    return yield_value;
}

// MARK: Helpers

void CodeGenerator::add_c_functions() {
    // stderr
    if (!mod_ctx.ir_module->getGlobalVariable("stderr")) {
        llvm::PointerType* file_ptr_type =
            llvm::PointerType::get(*mod_ctx.llvm_context, 0);
        new llvm::GlobalVariable(
            *mod_ctx.ir_module,
            file_ptr_type,
            true, // isConstant
            llvm::GlobalValue::ExternalLinkage,
            nullptr,
            "stderr"
        );
    }
    // printf
    if (!mod_ctx.ir_module->getFunction("printf")) {
        llvm::FunctionType* printf_type = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(*mod_ctx.llvm_context),
            {llvm::PointerType::get(*mod_ctx.llvm_context, 0)},
            true // true = variadic
        );
        llvm::Function::Create(
            printf_type,
            llvm::Function::ExternalLinkage,
            "printf",
            *mod_ctx.ir_module
        );
    }
    // fprintf
    if (!mod_ctx.ir_module->getFunction("fprintf")) {
        llvm::FunctionType* fprintf_type = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(*mod_ctx.llvm_context),
            {llvm::PointerType::get(*mod_ctx.llvm_context, 0),
             llvm::PointerType::get(*mod_ctx.llvm_context, 0)},
            true // true = variadic
        );
        llvm::Function::Create(
            fprintf_type,
            llvm::Function::ExternalLinkage,
            "fprintf",
            *mod_ctx.ir_module
        );
    }
    // abort
    if (!mod_ctx.ir_module->getFunction("abort")) {
        llvm::FunctionType* abort_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*mod_ctx.llvm_context),
            {},
            false
        );
        llvm::Function::Create(
            abort_type,
            llvm::Function::ExternalLinkage,
            "abort",
            *mod_ctx.ir_module
        );
    }
    // exit
    if (!mod_ctx.ir_module->getFunction("exit")) {
        llvm::FunctionType* exit_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*mod_ctx.llvm_context),
            {llvm::Type::getInt32Ty(*mod_ctx.llvm_context)},
            false
        );
        llvm::Function::Create(
            exit_type,
            llvm::Function::ExternalLinkage,
            "exit",
            *mod_ctx.ir_module
        );
    }
    // malloc
    if (!mod_ctx.ir_module->getFunction("malloc")) {
        llvm::FunctionType* malloc_type = llvm::FunctionType::get(
            llvm::PointerType::get(*mod_ctx.llvm_context, 0),
            {llvm::Type::getIntNTy(*mod_ctx.llvm_context, sizeof(size_t) * 8)},
            false
        );
        llvm::Function::Create(
            malloc_type,
            llvm::Function::ExternalLinkage,
            "malloc",
            *mod_ctx.ir_module
        );
    }
    // free
    if (!mod_ctx.ir_module->getFunction("free")) {
        llvm::FunctionType* free_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*mod_ctx.llvm_context),
            {llvm::PointerType::get(*mod_ctx.llvm_context, 0)},
            false
        );
        llvm::Function::Create(
            free_type,
            llvm::Function::ExternalLinkage,
            "free",
            *mod_ctx.ir_module
        );
    }
    if (panic_recoverable) {
        // jmp_buf
        if (!mod_ctx.ir_module->getGlobalVariable("jmp_buf", true)) {
            llvm::ArrayType* jmp_buf_type = llvm::ArrayType::get(
                llvm::Type::getInt8Ty(*mod_ctx.llvm_context),
                256
            );
            new llvm::GlobalVariable(
                *mod_ctx.ir_module,
                jmp_buf_type,
                false,
                llvm::GlobalValue::InternalLinkage,
                llvm::Constant::getNullValue(jmp_buf_type),
                "jmp_buf"
            );
        }

        // setjmp (panic recoverable only)
        if (!mod_ctx.ir_module->getFunction("setjmp")) {
            llvm::FunctionType* setjmp_type = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(*mod_ctx.llvm_context),
                {llvm::PointerType::get(*mod_ctx.llvm_context, 0)},
                false
            );
            llvm::Function::Create(
                setjmp_type,
                llvm::Function::ExternalLinkage,
                "setjmp",
                *mod_ctx.ir_module
            );
        }
        // longjmp (panic recoverable only)
        if (!mod_ctx.ir_module->getFunction("longjmp")) {
            llvm::FunctionType* longjmp_type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(*mod_ctx.llvm_context),
                {llvm::PointerType::get(*mod_ctx.llvm_context, 0),
                 llvm::Type::getInt32Ty(*mod_ctx.llvm_context)},
                false
            );
            llvm::Function::Create(
                longjmp_type,
                llvm::Function::ExternalLinkage,
                "longjmp",
                *mod_ctx.ir_module
            );
        }
    }
}

void CodeGenerator::add_div_zero_check(
    llvm::Value* divisor, const Location* location
) {
    llvm::BasicBlock* current_block = builder->GetInsertBlock();
    llvm::Function* current_function = current_block->getParent();

    llvm::BasicBlock* div_by_zero_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "div_by_zero",
        current_function
    );
    llvm::BasicBlock* div_ok_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "div_ok",
        current_function
    );

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

void CodeGenerator::add_panic(
    std::string_view message, const Location* location
) {
    llvm::Value* jmp_buf_ptr;
    if (panic_recoverable) {
        llvm::GlobalVariable* jmp_buf_global =
            mod_ctx.ir_module->getGlobalVariable("jmp_buf", true);
        jmp_buf_ptr = builder->CreateBitCast(
            jmp_buf_global,
            llvm::PointerType::get(*mod_ctx.llvm_context, 0)
        );
    }
    auto fprintf_fn = mod_ctx.ir_module->getFunction("fprintf");
    llvm::Value* stderr_stream = builder->CreateLoad(
        llvm::PointerType::get(*mod_ctx.llvm_context, 0),
        mod_ctx.ir_module->getGlobalVariable("stderr")
    );
    llvm::Value* format_string =
        builder->CreateGlobalStringPtr("Panic: %s: %s\n%s:%d:%d\n");
    llvm::Value* func_name =
        builder->CreateGlobalStringPtr(block_list->get_function_name());
    llvm::Value* msg = builder->CreateGlobalStringPtr(message);
    auto location_tuple = location->to_tuple();
    llvm::Value* file_name =
        builder->CreateGlobalStringPtr(std::get<0>(location_tuple));
    llvm::Value* line_number = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(*mod_ctx.llvm_context),
        std::get<1>(location_tuple)
    );
    llvm::Value* column_number = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(*mod_ctx.llvm_context),
        std::get<2>(location_tuple)
    );
    builder->CreateCall(
        fprintf_fn,
        {stderr_stream,
         format_string,
         func_name,
         msg,
         file_name,
         line_number,
         column_number}
    );

    if (panic_recoverable) {
        auto longjmp_fn = mod_ctx.ir_module->getFunction("longjmp");
        builder->CreateCall(
            longjmp_fn,
            {jmp_buf_ptr,
             llvm::ConstantInt::get(
                 llvm::Type::getInt32Ty(*mod_ctx.llvm_context),
                 1
             )}
        );
    }
    else {
        auto abort_fn = mod_ctx.ir_module->getFunction("abort");
        builder->CreateCall(abort_fn);
    }
}

bool CodeGenerator::verify_ir() {
    if (ir_printing_enabled) {
        mod_ctx.ir_module->print(llvm::outs(), nullptr);
    }

    std::string error_str;
    llvm::raw_string_ostream error_stream(error_str);

    bool failed = llvm::verifyModule(*mod_ctx.ir_module, &error_stream);
    if (failed) {
        error_stream.flush(); // Ensure all output is written to error_str
        panic(
            std::string("CodeGenerator::verify_ir: IR verification failed: ") +
            error_str
        );
    }
    return !failed;
}

void CodeGenerator::generate_script(
    const std::unique_ptr<FrontendContext>& context,
    std::string_view script_fn_name
) {
    add_c_functions();

    llvm::FunctionType* script_fn_type =
        llvm::FunctionType::get(builder->getInt32Ty(), false);
    llvm::Function* script_fn = llvm::Function::Create(
        script_fn_type,
        llvm::Function::ExternalLinkage,
        script_fn_name,
        mod_ctx.ir_module.get()
    );

    // Create a basic block for the script function
    llvm::BasicBlock* entry_block =
        llvm::BasicBlock::Create(*mod_ctx.llvm_context, "entry", script_fn);
    llvm::BasicBlock* exit_block =
        llvm::BasicBlock::Create(*mod_ctx.llvm_context, "exit", script_fn);

    // Start inserting instructions into the entry block.
    builder->SetInsertPoint(entry_block);

    // Allocate space for the return value.
    llvm::AllocaInst* ret_val =
        builder->CreateAlloca(builder->getInt32Ty(), nullptr, "$retval");

    // Append the exit block to the block list.
    block_list =
        std::make_shared<Block::Script>(block_list, ret_val, exit_block);

    // Set panic recoverable code.
    if (panic_recoverable) {
        // Get jmp_buf
        llvm::GlobalVariable* jmp_buf_global =
            mod_ctx.ir_module->getGlobalVariable("jmp_buf", true);
        llvm::Value* jmp_buf_ptr = builder->CreateBitCast(
            jmp_buf_global,
            llvm::PointerType::get(*mod_ctx.llvm_context, 0)
        );

        // Call setjmp
        llvm::Function* setjmp_fn = mod_ctx.ir_module->getFunction("setjmp");
        llvm::Value* setjmp_ret = builder->CreateCall(setjmp_fn, {jmp_buf_ptr});
        llvm::Value* is_setjmp = builder->CreateICmpNE(
            setjmp_ret,
            llvm::ConstantInt::get(
                llvm::Type::getInt32Ty(*mod_ctx.llvm_context),
                0
            )
        );

        // Create panic and normal blocks
        llvm::BasicBlock* panic_block =
            llvm::BasicBlock::Create(*mod_ctx.llvm_context, "panic", script_fn);
        llvm::BasicBlock* normal_block = llvm::BasicBlock::Create(
            *mod_ctx.llvm_context,
            "normal",
            script_fn
        );

        builder->CreateCondBr(is_setjmp, panic_block, normal_block);

        // When longjmp is called, we jump to here and return 101.
        builder->SetInsertPoint(panic_block);
        builder->CreateStore(builder->getInt32(101), ret_val);
        builder->CreateBr(exit_block);

        // Normal code continues from here.
        builder->SetInsertPoint(normal_block);
    }

    // CODE STARTS HERE

    for (size_t i = context->stmts_processed; i < context->stmts.size(); ++i) {
        context->stmts[i]->accept(this);
    }

    // CODE ENDS HERE

    // Assign to ret_val
    builder->CreateStore(builder->getInt32(0), ret_val);

    // Jump to exit block
    builder->CreateBr(exit_block);
    builder->SetInsertPoint(exit_block);

    // Return the value from ret_val
    builder->CreateRet(builder->CreateLoad(builder->getInt32Ty(), ret_val));
}

void CodeGenerator::generate_main(std::string_view script_fn_name) {
    // Generate the main function that calls $script
    llvm::FunctionType* main_fn_type = llvm::FunctionType::get(
        builder->getInt32Ty(),
        {builder->getInt32Ty(), builder->getPtrTy()},
        false
    );

    llvm::Function* main_fn = llvm::Function::Create(
        main_fn_type,
        llvm::Function::ExternalLinkage,
        "main",
        mod_ctx.ir_module.get()
    );

    // Create a basic block for the main function
    llvm::BasicBlock* main_entry =
        llvm::BasicBlock::Create(*mod_ctx.llvm_context, "entry", main_fn);
    builder->SetInsertPoint(main_entry);

    // Call the script function
    llvm::Function* script_fn = mod_ctx.ir_module->getFunction(script_fn_name);
    llvm::Value* script_ret = builder->CreateCall(script_fn);

    // We discard the args for now; later we can come up with a way to make them
    // accessible to the user.

    // Return the result
    builder->CreateRet(script_ret);
}

void CodeGenerator::generate_exe_ir(
    std::unique_ptr<FrontendContext>& context,
    bool ir_printing_enabled,
    bool panic_recoverable,
    std::string_view module_name,
    bool require_verification
) {
    if (context->status == Status::Error) {
        panic("CodeGenerator::generate_exe_ir: Context is in an error state.");
    }

    CodeGenerator codegen(
        module_name,
        ir_printing_enabled,
        panic_recoverable,
        false // repl_mode
    );

    codegen.generate_script(context);
    codegen.generate_main();
    if (require_verification && !codegen.verify_ir()) {
        panic("CodeGenerator::generate_exe_ir(): IR verification failed.");
    }

    context->mod_ctx = std::move(codegen.mod_ctx);
}
void CodeGenerator::generate_repl_ir(
    std::unique_ptr<FrontendContext>& context,
    bool ir_printing_enabled,
    bool require_verification
) {
    if (context->status == Status::Error) {
        panic("CodeGenerator::generate_repl_ir: Context is in an error state.");
    }

    auto repl_counter_str = std::to_string(++repl_counter);
    auto script_fn_name = "$entry_" + repl_counter_str;
    CodeGenerator codegen(
        "repl_" + repl_counter_str,
        ir_printing_enabled,
        true, // panic_recoverable
        true  // repl_mode
    );

    codegen.generate_script(context, script_fn_name);
    // FIXME: We cannot name the main function "main" every time in REPL mode.
    codegen.generate_main(script_fn_name);
    if (require_verification && !codegen.verify_ir()) {
        panic("CodeGenerator::generate_repl_ir(): IR verification failed.");
    }

    context->mod_ctx = std::move(codegen.mod_ctx);
}
