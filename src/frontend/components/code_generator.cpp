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

namespace nico {

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
    auto field_entry = stmt->field_entry.lock();
    auto llvm_type = field_entry->field.type->get_llvm_type(builder);
    auto symbol = field_entry->symbol;

    llvm::Value* allocation = nullptr;

    if (field_entry->is_global) {
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
            symbol
        );
    }
    else {
        auto alloca_inst = builder->CreateAlloca(llvm_type, nullptr, symbol);

        stmt->field_entry.lock()->llvm_ptr = alloca_inst;
        allocation = alloca_inst;
    }

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

std::any CodeGenerator::visit(Stmt::Static* /*stmt*/) {
    /*
    We don't actually need to do anything here.

    Static variables are always global.
    When attempting to access a global variable, the `get_llvm_allocation`
    function will create a global variable if it doesn't already exist. We can
    rely on this behavior to create the global variable on the first access.
    */

    return std::any();
}

std::any CodeGenerator::visit(Stmt::Func* stmt) {

    auto script_block = builder->GetInsertBlock();

    auto field_entry_type = stmt->field_entry.lock()->field.type;
    auto func_type =
        std::dynamic_pointer_cast<Type::Function>(field_entry_type);

    llvm::FunctionType* llvm_func_type =
        func_type->get_llvm_function_type(builder);
    llvm::Function* function = llvm::Function::Create(
        llvm_func_type,
        llvm::Function::ExternalLinkage,
        // We add "$func" to differentiate this from the global variable
        // declared below.
        stmt->field_entry.lock()->symbol + "$func",
        mod_ctx.ir_module.get()
    );

    // Create the function's blocks.
    llvm::BasicBlock* entry_block =
        llvm::BasicBlock::Create(*mod_ctx.llvm_context, "entry", function);
    llvm::BasicBlock* exit_block =
        llvm::BasicBlock::Create(*mod_ctx.llvm_context, "exit", function);

    // Start inserting into the entry block.
    builder->SetInsertPoint(entry_block);

    // Allocate space for every parameter and store the incoming values.
    for (size_t i = 0; i < stmt->parameters.size(); ++i) {
        auto& param = stmt->parameters[i];
        llvm::Argument* llvm_param = function->getArg(i);
        llvm::AllocaInst* param_alloca = builder->CreateAlloca(
            llvm_param->getType(),
            nullptr,
            param.field_entry.lock()->symbol
        );
        builder->CreateStore(llvm_param, param_alloca);
        param.field_entry.lock()->llvm_ptr = param_alloca;
    }
    // Allocate space for the return value.
    llvm::AllocaInst* return_alloca = builder->CreateAlloca(
        func_type->return_type->get_llvm_type(builder),
        nullptr,
        "$retval"
    );
    // Add the block to the control stack.
    control_stack.add_function_block(
        return_alloca,
        exit_block,
        stmt->identifier->lexeme
    );

    // Generate code for the function body.
    stmt->body.value()->accept(this, false);
    // The body is always a block.

    // Jump to the exit block.
    builder->CreateBr(exit_block);
    builder->SetInsertPoint(exit_block);
    // Load the return value and return it.
    llvm::Value* return_value = builder->CreateLoad(
        func_type->return_type->get_llvm_type(builder),
        return_alloca
    );
    builder->CreateRet(return_value);

    // Pop the function block from the control stack.
    control_stack.pop_block();

    // Use a global variable to hold the function pointer.
    llvm::GlobalVariable* llvm_global = llvm::cast<llvm::GlobalVariable>(
        stmt->field_entry.lock()->get_llvm_allocation(builder, true)
    );
    // `get_llvm_allocation` will create a global variable if it doesn't already
    // exist. The variable may already exist if it was referenced before the
    // function was declared.

    // New variable or not, the initializer is initialially set to null.
    llvm_global->setInitializer(function);
    // We set the initializer to the function.

    builder->SetInsertPoint(script_block);

    return std::any();
}

std::any CodeGenerator::visit(Stmt::Print* stmt) {
    llvm::Function* printf_fn = mod_ctx.ir_module->getFunction("printf");

    llvm::Value* format_str = nullptr;
    for (const auto& expr : stmt->expressions) {
        auto value = std::any_cast<llvm::Value*>(expr->accept(this, false));
        auto [fmt, fmt_args] = expr->type->to_print_args(builder, value);
        auto args =
            std::vector<llvm::Value*>{builder->CreateGlobalStringPtr(fmt)};
        args.insert(args.end(), fmt_args.begin(), fmt_args.end());
        builder->CreateCall(printf_fn, args);
    }

    if (repl_mode) {
        // In REPL mode, add an extra newline.
        format_str = builder->CreateGlobalStringPtr("\n");
        builder->CreateCall(printf_fn, {format_str});
    }

    // Generate code for the print statement
    return std::any();
}

std::any CodeGenerator::visit(Stmt::Dealloc* stmt) {
    auto expr_value =
        std::any_cast<llvm::Value*>(stmt->expression->accept(this, false));
    llvm::Function* free_fn = mod_ctx.ir_module->getFunction("free");

    builder->CreateCall(free_fn, {expr_value});

    return std::any();
}

std::any CodeGenerator::visit(Stmt::Pass* /*stmt*/) {
    // A pass statement does nothing.
    return std::any();
}

std::any CodeGenerator::visit(Stmt::Yield* stmt) {
    // Evaluate the expression to yield.
    auto yield_value =
        std::any_cast<llvm::Value*>(stmt->expression->accept(this, false));

    // Determine if this yield statement requires an unreachable block after it.
    bool require_unreachable_block = false;

    if (stmt->yield_token->tok_type == Tok::KwYield) {
        Expr::Block::Kind target_kind = stmt->target_block.lock()->kind;
        builder->CreateStore(
            yield_value,
            control_stack.get_yield_allocation(target_kind)
        );
    }
    else if (stmt->yield_token->tok_type == Tok::KwBreak) {
        // For break statements, find the nearest enclosing loop block.
        builder->CreateStore(
            yield_value,
            control_stack.get_yield_allocation(Expr::Block::Kind::Loop)
        );
        builder->CreateBr(
            control_stack.get_exit_block(Expr::Block::Kind::Loop)
        );
        require_unreachable_block = true;
    }
    else if (stmt->yield_token->tok_type == Tok::KwReturn) {
        // For return statements, find the nearest enclosing function block.
        builder->CreateStore(
            yield_value,
            control_stack.get_yield_allocation(Expr::Block::Kind::Function)
        );
        builder->CreateBr(
            control_stack.get_exit_block(Expr::Block::Kind::Function)
        );
        require_unreachable_block = true;
    }
    else {
        panic("CodeGenerator::visit(Stmt::Yield*): Unknown yield type.");
        return std::any();
    }

    if (require_unreachable_block) {
        // After a break or return, we need to create a new block to continue
        // generating code in.
        auto unreachable_block = llvm::BasicBlock::Create(
            *mod_ctx.llvm_context,
            "unreachable",
            builder->GetInsertBlock()->getParent()
        );
        builder->SetInsertPoint(unreachable_block);
        // We allow statements to appear after a break or return, but they will
        // unreachable. LLVM has an "unreachable" instruction, but only as a
        // block terminator.
        // In the future, we can change prevent code generation after a break or
        // return.
    }

    return std::any();
}

std::any CodeGenerator::visit(Stmt::Continue* /*stmt*/) {
    // Generate code for the continue statement
    builder->CreateBr(control_stack.get_continue_block());
    auto unreachable_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "unreachable",
        builder->GetInsertBlock()->getParent()
    );
    builder->SetInsertPoint(unreachable_block);
    return std::any();
}

std::any CodeGenerator::visit(Stmt::Namespace* stmt) {
    // Visit each statement in the namespace block.
    for (const auto& decl : stmt->stmts) {
        decl->accept(this);
    }
    return std::any();
}

std::any CodeGenerator::visit(Stmt::Extern* stmt) {
    // TODO: Implement code generation for extern blocks.
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

    if (PTR_INSTANCEOF(expr->left->type, Type::Float)) {
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
        case Tok::EqEq:
            result = builder->CreateFCmpUEQ(left, right);
            break;
        case Tok::Percent:
            result = builder->CreateFRem(left, right);
            break;
        case Tok::BangEq:
            result = builder->CreateFCmpUNE(left, right);
            break;
        case Tok::Lt:
            result = builder->CreateFCmpULT(left, right);
            break;
        case Tok::LtEq:
            result = builder->CreateFCmpULE(left, right);
            break;
        case Tok::Gt:
            result = builder->CreateFCmpUGT(left, right);
            break;
        case Tok::GtEq:
            result = builder->CreateFCmpUGE(left, right);
            break;
        default:
            panic(
                "CodeGenerator::visit(Expr::Binary*): Unknown binary operator "
                "for floating-point number."
            );
        }
    }
    else if (PTR_INSTANCEOF(expr->left->type, Type::Int) ||
             PTR_INSTANCEOF(expr->left->type, Type::Bool) ||
             PTR_INSTANCEOF(expr->left->type, Type::RawTypedPtr)) {
        auto int_type = std::dynamic_pointer_cast<Type::Int>(expr->left->type);
        auto is_signed_int = int_type ? int_type->is_signed : false;

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
            result = is_signed_int ? builder->CreateSDiv(left, right)
                                   : builder->CreateUDiv(left, right);
            break;
        case Tok::Percent:
            add_div_zero_check(right, &expr->op->location);
            result = is_signed_int ? builder->CreateSRem(left, right)
                                   : builder->CreateURem(left, right);
            break;
        case Tok::EqEq:
            result = builder->CreateICmpEQ(left, right);
            break;
        case Tok::BangEq:
            result = builder->CreateICmpNE(left, right);
            break;
        case Tok::Lt:
            result = is_signed_int ? builder->CreateICmpSLT(left, right)
                                   : builder->CreateICmpULT(left, right);
            break;
        case Tok::LtEq:
            result = is_signed_int ? builder->CreateICmpSLE(left, right)
                                   : builder->CreateICmpULE(left, right);
            break;
        case Tok::Gt:
            result = is_signed_int ? builder->CreateICmpSGT(left, right)
                                   : builder->CreateICmpUGT(left, right);
            break;
        case Tok::GtEq:
            result = is_signed_int ? builder->CreateICmpSGE(left, right)
                                   : builder->CreateICmpUGE(left, right);
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
    if (PTR_INSTANCEOF(expr->right->type, Type::Float)) {
        switch (expr->op->tok_type) {
        case Tok::Negative:
            result = builder->CreateFNeg(right);
            break;
        default:
            panic(
                "CodeGenerator::visit(Expr::Unary*): Unknown unary operator "
                "for floating-point number."
            );
        }
    }
    else if (PTR_INSTANCEOF(expr->right->type, Type::Int)) {
        switch (expr->op->tok_type) {
        case Tok::Negative:
            result = builder->CreateNeg(right);
            break;
        default:
            panic(
                "CodeGenerator::visit(Expr::Unary*): Unknown unary operator "
                "for integer."
            );
        }
    }
    else if (PTR_INSTANCEOF(expr->right->type, Type::Bool)) {
        switch (expr->op->tok_type) {
        case Tok::KwNot:
        case Tok::Bang:
            result = builder->CreateNot(right);
            break;
        default:
            panic(
                "CodeGenerator::visit(Expr::Unary*): Unknown unary operator "
                "for boolean."
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

std::any CodeGenerator::visit(Expr::Address* expr, bool as_lvalue) {
    return std::any_cast<llvm::Value*>(expr->right->accept(this, true));
}

std::any CodeGenerator::visit(Expr::Deref* expr, bool as_lvalue) {
    llvm::Value* result =
        std::any_cast<llvm::Value*>(expr->right->accept(this, false));

    if (as_lvalue) {
        return result;
    }
    else {
        result =
            builder->CreateLoad(expr->type->get_llvm_type(builder), result);
        return result;
    }
}

std::any CodeGenerator::visit(Expr::Cast* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;
    auto inner_expr =
        std::any_cast<llvm::Value*>(expr->expression->accept(this, false));

    switch (expr->operation) {
    case Expr::Cast::Operation::NoOp:
        result = inner_expr;
        break;
    case Expr::Cast::Operation::SignExt:
        result =
            builder->CreateSExt(inner_expr, expr->type->get_llvm_type(builder));
        break;
    case Expr::Cast::Operation::ZeroExt:
        result =
            builder->CreateZExt(inner_expr, expr->type->get_llvm_type(builder));
        break;
    case Expr::Cast::Operation::FPExt:
        result = builder->CreateFPExt(
            inner_expr,
            expr->type->get_llvm_type(builder)
        );
        break;
    case Expr::Cast::Operation::IntTrunc:
        result = builder->CreateTrunc(
            inner_expr,
            expr->type->get_llvm_type(builder)
        );
        break;
    case Expr::Cast::Operation::FPTrunc:
        result = builder->CreateFPTrunc(
            inner_expr,
            expr->type->get_llvm_type(builder)
        );
        break;
    case Expr::Cast::Operation::FPToSInt: {
        // Clamp the FP value to the range of the target signed integer type
        auto int_type = std::dynamic_pointer_cast<Type::Int>(expr->type);
        auto min_val = llvm::ConstantFP::get(
            inner_expr->getType(),
            (double)int_type->get_min_value()
        );
        auto max_val = llvm::ConstantFP::get(
            inner_expr->getType(),
            (double)int_type->get_max_value()
        );
        auto clamped = builder->CreateSelect(
            builder->CreateFCmpOLT(inner_expr, min_val),
            min_val,
            builder->CreateSelect(
                builder->CreateFCmpOGT(inner_expr, max_val),
                max_val,
                inner_expr
            )
        );
        result =
            builder->CreateFPToSI(clamped, expr->type->get_llvm_type(builder));
        break;
    }
    case Expr::Cast::Operation::FPToUInt: {
        // Clamp the FP value to the range of the target unsigned integer type
        auto int_type = std::dynamic_pointer_cast<Type::Int>(expr->type);
        auto min_val = llvm::ConstantFP::get(
            inner_expr->getType(),
            0.0
        ); // Unsigned integers have a minimum of 0
        auto max_val = llvm::ConstantFP::get(
            inner_expr->getType(),
            (double)int_type->get_max_value()
        );
        auto clamped = builder->CreateSelect(
            builder->CreateFCmpOLT(inner_expr, min_val),
            min_val,
            builder->CreateSelect(
                builder->CreateFCmpOGT(inner_expr, max_val),
                max_val,
                inner_expr
            )
        );
        result =
            builder->CreateFPToUI(clamped, expr->type->get_llvm_type(builder));
        break;
    }
    case Expr::Cast::Operation::SIntToFP:
        result = builder->CreateSIToFP(
            inner_expr,
            expr->type->get_llvm_type(builder)
        );
        break;
    case Expr::Cast::Operation::UIntToFP:
        result = builder->CreateUIToFP(
            inner_expr,
            expr->type->get_llvm_type(builder)
        );
        break;
    case Expr::Cast::Operation::IntToBool:
        result = builder->CreateICmpNE(
            inner_expr,
            llvm::ConstantInt::get(inner_expr->getType(), 0)
        );
        break;
    case Expr::Cast::Operation::FPToBool:
        result = builder->CreateFCmpUNE(
            inner_expr,
            llvm::ConstantFP::get(inner_expr->getType(), 0.0)
        );
        break;
    case Expr::Cast::Operation::ReinterpretBits:
        result = builder->CreateBitCast(
            inner_expr,
            expr->type->get_llvm_type(builder)
        );
        break;
    default:
        panic("CodeGenerator::visit(Expr::Cast*): Unknown cast operation.");
    }

    return result;
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

std::any CodeGenerator::visit(Expr::Subscript* expr, bool as_lvalue) {
    // Get the array pointer
    auto array_ptr =
        std::any_cast<llvm::Value*>(expr->left->accept(this, true));
    // Get the index
    auto index = std::any_cast<llvm::Value*>(expr->index->accept(this, false));

    // Handle array types
    if (auto array_type =
            std::dynamic_pointer_cast<Type::Array>(expr->left->type)) {

        if (array_type->size.has_value()) {
            add_array_bounds_check(
                index,
                array_type->size.value(),
                expr->index->location
            );
        }

        // Get the element pointer
        llvm::Value* element_ptr = builder->CreateGEP(
            array_type->base->get_llvm_type(builder),
            array_ptr,
            index,
            "array_element"
        );
        if (as_lvalue) {
            return element_ptr;
        }
        else {
            llvm::Value* result = builder->CreateLoad(
                expr->type->get_llvm_type(builder),
                element_ptr
            );
            return result;
        }
    }

    panic(
        "CodeGenerator::visit(Expr::Subscript*): Left expression is not "
        "an array type."
    );
    return std::any();
}

std::any CodeGenerator::visit(Expr::Call* expr, bool as_lvalue) {
    // Visit the callee.
    auto callee =
        std::any_cast<llvm::Value*>(expr->callee->accept(this, false));
    auto callee_fn_type =
        std::dynamic_pointer_cast<Type::Function>(expr->callee->type);
    if (!callee_fn_type) {
        panic(
            "Codegenerator::visit(Expr::Call*): Callee is not a function type. "
            "Found: " +
            expr->callee->type->to_string()
        );
    }
    // callee is a global variable containing a function pointer.

    // Visit each argument.
    std::vector<llvm::Value*> args;
    for (const auto& [_, arg_weak_ptr] : expr->actual_args) {
        args.push_back(
            std::any_cast<llvm::Value*>(
                arg_weak_ptr.lock()->accept(this, false)
            )
        );
    }

    // Make the call.
    llvm::Value* result = builder->CreateCall(
        callee_fn_type->get_llvm_function_type(builder),
        callee,
        args
    );

    return result;
}

std::any CodeGenerator::visit(Expr::SizeOf* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;
    size_t type_size = expr->inner_type->get_llvm_type_size(builder);

    result = llvm::ConstantInt::get(
        llvm::Type::getInt64Ty(*mod_ctx.llvm_context),
        type_size
    );

    return result;
}

std::any CodeGenerator::visit(Expr::Alloc* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;
    llvm::Value* alloc_size = nullptr;

    auto pointer_type =
        std::dynamic_pointer_cast<Type::RawTypedPtr>(expr->type);

    if (expr->amount_expr.has_value()) {
        // `alloc for`
        size_t type_size =
            std::dynamic_pointer_cast<Type::Array>(pointer_type->base)
                ->base->get_llvm_type_size(builder);

        llvm::Value* type_size_value = llvm::ConstantInt::get(
            llvm::Type::getInt64Ty(*mod_ctx.llvm_context),
            type_size
        );
        auto amount_value = std::any_cast<llvm::Value*>(
            expr->amount_expr.value()->accept(this, false)
        );
        // amount_value is definitely an integer, but may not be a u64.
        // Use ZExt to convert it.
        if (amount_value->getType()->isIntegerTy(64) == false) {
            amount_value = builder->CreateZExt(
                amount_value,
                llvm::Type::getInt64Ty(*mod_ctx.llvm_context),
                "alloc_amount_u64"
            );
        }
        add_negative_alloc_size_check(amount_value, expr->location);

        alloc_size = builder->CreateMul(
            type_size_value,
            amount_value,
            "total_alloc_size"
        );
    }
    else {
        // `alloc`
        size_t type_size = pointer_type->base->get_llvm_type_size(builder);
        alloc_size = llvm::ConstantInt::get(
            llvm::Type::getInt64Ty(*mod_ctx.llvm_context),
            type_size
        );
    }

    llvm::Function* malloc_fn = mod_ctx.ir_module->getFunction("malloc");
    result = builder->CreateCall(malloc_fn, {alloc_size}, "alloc_ptr");
    add_alloc_nullptr_check(result, expr->location);

    if (expr->expression.has_value()) {
        auto expr_value = std::any_cast<llvm::Value*>(
            expr->expression.value()->accept(this, false)
        );
        builder->CreateStore(expr_value, result);
    }

    return result;
}

std::any CodeGenerator::visit(Expr::NameRef* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;
    llvm::Value* ptr =
        expr->field_entry.lock()->get_llvm_allocation(builder, repl_mode);

    if (as_lvalue) {
        // We use the pointer to the variable (its alloca inst or global ptr)
        result = ptr;
    }
    else {
        // We load the value from the variable's memory location
        result = builder->CreateLoad(expr->type->get_llvm_type(builder), ptr);
    }

    return result;
}

std::any CodeGenerator::visit(Expr::Literal* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;

    switch (expr->token->tok_type) {
    case Tok::Int8:
        result = llvm::ConstantInt::get(
            llvm::Type::getInt8Ty(*mod_ctx.llvm_context),
            std::any_cast<int8_t>(expr->token->literal)
        );
        break;
    case Tok::Int16:
        result = llvm::ConstantInt::get(
            llvm::Type::getInt16Ty(*mod_ctx.llvm_context),
            std::any_cast<int16_t>(expr->token->literal)
        );
        break;
    case Tok::Int32:
        result = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(*mod_ctx.llvm_context),
            std::any_cast<int32_t>(expr->token->literal)
        );
        break;
    case Tok::Int64:
        result = llvm::ConstantInt::get(
            llvm::Type::getInt64Ty(*mod_ctx.llvm_context),
            std::any_cast<int64_t>(expr->token->literal)
        );
        break;
    case Tok::UInt8:
        result = llvm::ConstantInt::get(
            llvm::Type::getInt8Ty(*mod_ctx.llvm_context),
            std::any_cast<uint8_t>(expr->token->literal)
        );
        break;
    case Tok::UInt16:
        result = llvm::ConstantInt::get(
            llvm::Type::getInt16Ty(*mod_ctx.llvm_context),
            std::any_cast<uint16_t>(expr->token->literal)
        );
        break;
    case Tok::UInt32:
        result = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(*mod_ctx.llvm_context),
            std::any_cast<uint32_t>(expr->token->literal)
        );
        break;
    case Tok::UInt64:
        result = llvm::ConstantInt::get(
            llvm::Type::getInt64Ty(*mod_ctx.llvm_context),
            std::any_cast<uint64_t>(expr->token->literal)
        );
        break;
    case Tok::Float32:
        if (expr->token->lexeme == "inf32") {
            result = llvm::ConstantFP::getInfinity(
                llvm::Type::getFloatTy(*mod_ctx.llvm_context)
            );
        }
        else if (expr->token->lexeme == "nan32") {
            result = llvm::ConstantFP::getNaN(
                llvm::Type::getFloatTy(*mod_ctx.llvm_context)
            );
        }
        else {
            result = llvm::ConstantFP::get(
                llvm::Type::getFloatTy(*mod_ctx.llvm_context),
                std::any_cast<float>(expr->token->literal)
            );
        }
        break;
    case Tok::Float64:
        if (expr->token->lexeme == "inf" || expr->token->lexeme == "inf64") {
            result = llvm::ConstantFP::getInfinity(
                llvm::Type::getDoubleTy(*mod_ctx.llvm_context)
            );
        }
        else if (expr->token->lexeme == "nan" ||
                 expr->token->lexeme == "nan64") {
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
    case Tok::Nullptr:
        result = llvm::ConstantPointerNull::get(
            llvm::PointerType::get(*mod_ctx.llvm_context, 0)
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

std::any CodeGenerator::visit(Expr::Array* expr, bool as_lvalue) {
    llvm::Value* result = nullptr;
    std::vector<llvm::Value*> element_values;
    for (const auto& element : expr->elements) {
        element_values.push_back(
            std::any_cast<llvm::Value*>(element->accept(this, false))
        );
    }
    llvm::ArrayType* array_type =
        llvm::cast<llvm::ArrayType>(expr->type->get_llvm_type(builder));
    llvm::Value* array_alloc =
        builder->CreateAlloca(array_type, nullptr, "array");

    for (size_t i = 0; i < element_values.size(); ++i) {
        llvm::Value* element_ptr = builder->CreateGEP(
            array_type,
            array_alloc,
            {llvm::ConstantInt::get(
                 llvm::Type::getInt64Ty(*mod_ctx.llvm_context),
                 0
             ),
             llvm::ConstantInt::get(
                 llvm::Type::getInt64Ty(*mod_ctx.llvm_context),
                 i
             )},
            "element"
        );
        builder->CreateStore(element_values[i], element_ptr);
    }
    result = builder->CreateLoad(array_type, array_alloc);

    return result;
}

std::any CodeGenerator::visit(Expr::Block* expr, bool as_lvalue) {
    // Blocks get their own yield allocation.
    llvm::AllocaInst* yield_allocation = builder->CreateAlloca(
        expr->type->get_llvm_type(builder),
        nullptr,
        "$yieldval"
    );
    // If this is a loop or function block, this yield value may go unused, but
    // that's okay.
    control_stack.add_block(yield_allocation);

    for (auto& stmt : expr->statements) {
        stmt->accept(this);
    }

    llvm::Value* yield_value = builder->CreateLoad(
        expr->type->get_llvm_type(builder),
        yield_allocation
    );

    control_stack.pop_block();

    return yield_value;
}

std::any CodeGenerator::visit(Expr::Conditional* expr, bool as_lvalue) {
    llvm::Function* current_function = builder->GetInsertBlock()->getParent();

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
    then_block = builder->GetInsertBlock();
    builder->CreateBr(merge_block);

    // Else block
    builder->SetInsertPoint(else_block);
    auto else_value =
        std::any_cast<llvm::Value*>(expr->else_branch->accept(this, false));
    else_block = builder->GetInsertBlock();
    builder->CreateBr(merge_block);

    // We reassign then_block and else_block to the current block after
    // generating their branch code. This is in case their blocks added
    // additional llvm::BasicBlocks. And we reuse the variables the save on
    // space.

    // Merge block
    builder->SetInsertPoint(merge_block);
    auto phi =
        builder->CreatePHI(expr->type->get_llvm_type(builder), 2, "cond_val");
    phi->addIncoming(then_value, then_block);
    phi->addIncoming(else_value, else_block);
    llvm::Value* yield_value = phi;

    // We *could* use a $yieldval allocation like we do with loops and
    // functions, but control flow for conditionals is more linear and easier to
    // manage with a phi node.

    return yield_value;
}

std::any CodeGenerator::visit(Expr::Loop* expr, bool as_lvalue) {
    llvm::AllocaInst* yield_allocation = builder->CreateAlloca(
        expr->type->get_llvm_type(builder),
        nullptr,
        "$breakval"
    );

    // Loops are complicated because they allow break statements, which
    // interrupt the control flow in a block. This potentially causes the yield
    // value of a plain block to become unreachable. So rather than trying to
    // salvage the yield value of the inner expression, we create a new yield
    // allocation for the loop itself. Break statements will have the ability to
    // set this yield allocation.

    llvm::Function* current_function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* do_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "loop_start",
        current_function
    );
    llvm::BasicBlock* merge_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "loop_end",
        current_function
    );

    if (expr->condition.has_value()) {
        // Conditional loops, as the name implies, have a condition block.
        llvm::BasicBlock* condition_block = llvm::BasicBlock::Create(
            *mod_ctx.llvm_context,
            "loop_cond",
            current_function
        );
        control_stack
            .add_loop_block(yield_allocation, merge_block, condition_block);

        // Conditional loops include while loops and do-while loops.
        // The flow is the same: cond->do->cond->do...
        if (expr->loops_once) {
            // For do-while loops, we enter the do block first, skipping the
            // first conditional check.
            builder->CreateBr(do_block);
        }
        else {
            // For while loops, we check the condition first.
            builder->CreateBr(condition_block);
        }

        // Setup the condition block.
        builder->SetInsertPoint(condition_block);
        auto condition = std::any_cast<llvm::Value*>(
            expr->condition.value()->accept(this, false)
        );
        builder->CreateCondBr(condition, do_block, merge_block);

        builder->SetInsertPoint(do_block);
        // For conditional loops, we ignore the value of the loop body because
        // the yield value is always `()`, which is already the default value
        // for the yield allocation.
        expr->body->accept(this, false);
        builder->CreateBr(condition_block);
    }
    else {
        control_stack.add_loop_block(yield_allocation, merge_block, do_block);

        // Non-conditional loops have no condition block.
        // The flow is simply: do->do->do...
        // The only way to exit the loop is with a break statement, which is
        // accessible through the block list.
        builder->CreateBr(do_block);
        builder->SetInsertPoint(do_block);
        expr->body->accept(this, false);
        // For non-conditional loops, we also ignore the value of
        // the loop body because the yield value is set by break statements,
        // which are the only way to exit the loop.
        builder->CreateBr(do_block);
    }

    builder->SetInsertPoint(merge_block);
    llvm::Value* yield_value = builder->CreateLoad(
        expr->type->get_llvm_type(builder),
        control_stack.get_yield_allocation(Expr::Block::Kind::Loop)
    );

    control_stack.pop_block();

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

void CodeGenerator::add_array_bounds_check(
    llvm::Value* index, size_t array_size, const Location* location
) {
    llvm::BasicBlock* current_block = builder->GetInsertBlock();
    llvm::Function* current_function = current_block->getParent();

    llvm::BasicBlock* out_of_bounds_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "array_out_of_bounds",
        current_function
    );
    llvm::BasicBlock* in_bounds_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "array_in_bounds",
        current_function
    );

    llvm::Value* is_oob = builder->CreateICmpUGE(
        index,
        llvm::ConstantInt::get(index->getType(), array_size)
    );
    builder->CreateCondBr(is_oob, out_of_bounds_block, in_bounds_block);

    // out_of_bounds_block
    builder->SetInsertPoint(out_of_bounds_block);
    add_panic(
        "Array index out of bounds for array of size " +
            std::to_string(array_size) + ".",
        location
    );
    builder->CreateUnreachable();

    // in_bounds_block
    builder->SetInsertPoint(in_bounds_block);
}

void CodeGenerator::add_alloc_nullptr_check(
    llvm::Value* ptr, const Location* location
) {
    llvm::BasicBlock* current_block = builder->GetInsertBlock();
    llvm::Function* current_function = current_block->getParent();

    llvm::BasicBlock* null_ptr_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "alloc_nullptr",
        current_function
    );
    llvm::BasicBlock* not_null_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "not_alloc_nullptr",
        current_function
    );

    llvm::Value* is_null = builder->CreateICmpEQ(
        ptr,
        llvm::ConstantPointerNull::get(
            llvm::PointerType::get(*mod_ctx.llvm_context, 0)
        )
    );
    builder->CreateCondBr(is_null, null_ptr_block, not_null_block);

    // null_ptr_block
    builder->SetInsertPoint(null_ptr_block);
    add_panic("Memory allocation failed.", location);
    builder->CreateUnreachable();

    // not_null_block
    builder->SetInsertPoint(not_null_block);
}

void CodeGenerator::add_negative_alloc_size_check(
    llvm::Value* size_value, const Location* location
) {
    llvm::BasicBlock* current_block = builder->GetInsertBlock();
    llvm::Function* current_function = current_block->getParent();

    llvm::BasicBlock* negative_size_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "alloc_negative_size",
        current_function
    );
    llvm::BasicBlock* non_negative_size_block = llvm::BasicBlock::Create(
        *mod_ctx.llvm_context,
        "alloc_non_negative_size",
        current_function
    );

    llvm::Value* is_negative = builder->CreateICmpSLT(
        size_value,
        llvm::ConstantInt::get(size_value->getType(), 0)
    );
    builder->CreateCondBr(
        is_negative,
        negative_size_block,
        non_negative_size_block
    );

    // negative_size_block
    builder->SetInsertPoint(negative_size_block);
    add_panic(
        "Allocation amount expression evaluated to a negative value.",
        location
    );
    builder->CreateUnreachable();

    // non_negative_size_block
    builder->SetInsertPoint(non_negative_size_block);
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
    llvm::Value* func_name = builder->CreateGlobalStringPtr(
        control_stack.get_current_function_name()
    );
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

void CodeGenerator::generate_script_func(
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
    control_stack.add_script_block(ret_val, exit_block);

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

    control_stack.pop_block();

    // Return the value from ret_val
    builder->CreateRet(builder->CreateLoad(builder->getInt32Ty(), ret_val));
}

void CodeGenerator::generate_main_func(
    std::string_view script_fn_name, std::string_view main_fn_name
) {
    // Generate the main function that calls $script
    llvm::FunctionType* main_fn_type = llvm::FunctionType::get(
        builder->getInt32Ty(),
        {builder->getInt32Ty(), builder->getPtrTy()},
        false
    );

    llvm::Function* main_fn = llvm::Function::Create(
        main_fn_type,
        llvm::Function::ExternalLinkage,
        main_fn_name,
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
    if (IS_VARIANT(context->status, Status::Error)) {
        panic("CodeGenerator::generate_exe_ir: Context is in an error state.");
    }

    CodeGenerator codegen(
        module_name,
        ir_printing_enabled,
        panic_recoverable,
        false // repl_mode
    );

    codegen.generate_script_func(context);
    codegen.generate_main_func();
    if (require_verification && !codegen.verify_ir()) {
        panic("CodeGenerator::generate_exe_ir(): IR verification failed.");
    }

    context->mod_ctx = std::move(codegen.mod_ctx);
    context->main_fn_name = "main";
}

void CodeGenerator::generate_repl_ir(
    std::unique_ptr<FrontendContext>& context,
    bool ir_printing_enabled,
    bool require_verification
) {
    if (IS_VARIANT(context->status, Status::Error)) {
        panic("CodeGenerator::generate_repl_ir: Context is in an error state.");
    }

    auto repl_counter_str = std::to_string(++repl_counter);
    auto script_fn_name = "$script_" + repl_counter_str;
    auto main_fn_name = "main_" + repl_counter_str;

    CodeGenerator codegen(
        main_fn_name,
        ir_printing_enabled,
        true, // panic_recoverable
        true  // repl_mode
    );

    codegen.generate_script_func(context, script_fn_name);
    codegen.generate_main_func(script_fn_name, main_fn_name);
    if (require_verification && !codegen.verify_ir()) {
        panic("CodeGenerator::generate_repl_ir(): IR verification failed.");
    }

    context->mod_ctx = std::move(codegen.mod_ctx);
    context->main_fn_name = main_fn_name;
}

} // namespace nico
