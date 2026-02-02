#ifndef NICO_MIR_BUILDER_H
#define NICO_MIR_BUILDER_H

#include <any>
#include <memory>

#include "nico/frontend/utils/ast_node.h"
#include "nico/frontend/utils/frontend_context.h"
#include "nico/shared/ir_module_context.h"

namespace nico {

class MIRBuilder : public Stmt::Visitor, public Expr::Visitor {
    // The MIR module to store the built MIR.
    const std::shared_ptr<MIRModule> mir_module;
    // The symbol tree used for type checking.
    const std::shared_ptr<SymbolTree> symbol_tree;
    // The current basic block being built.
    std::shared_ptr<BasicBlock> current_block;

    MIRBuilder(
        std::shared_ptr<MIRModule> mir_module,
        std::shared_ptr<SymbolTree> symbol_tree
    )
        : mir_module(mir_module),
          symbol_tree(symbol_tree),
          current_block(mir_module->get_script_function()->get_entry_block()) {}

    std::any visit(Stmt::Expression* stmt) override;
    std::any visit(Stmt::Let* stmt) override;
    std::any visit(Stmt::Func* stmt) override;
    std::any visit(Stmt::Print* stmt) override;
    std::any visit(Stmt::Dealloc* stmt) override;
    std::any visit(Stmt::Pass* stmt) override;
    std::any visit(Stmt::Yield* stmt) override;
    std::any visit(Stmt::Continue* stmt) override;
    std::any visit(Stmt::Eof* stmt) override;

    std::any visit(Expr::Assign* expr, bool as_lvalue) override;
    std::any visit(Expr::Logical* expr, bool as_lvalue) override;
    std::any visit(Expr::Binary* expr, bool as_lvalue) override;
    std::any visit(Expr::Unary* expr, bool as_lvalue) override;
    std::any visit(Expr::Address* expr, bool as_lvalue) override;
    std::any visit(Expr::Deref* expr, bool as_lvalue) override;
    std::any visit(Expr::Cast* expr, bool as_lvalue) override;
    std::any visit(Expr::Access* expr, bool as_lvalue) override;
    std::any visit(Expr::Subscript* expr, bool as_lvalue) override;
    std::any visit(Expr::Call* expr, bool as_lvalue) override;
    std::any visit(Expr::SizeOf* expr, bool as_lvalue) override;
    std::any visit(Expr::Alloc* expr, bool as_lvalue) override;
    std::any visit(Expr::OldNameRef* expr, bool as_lvalue) override;
    std::any visit(Expr::Literal* expr, bool as_lvalue) override;
    std::any visit(Expr::Tuple* expr, bool as_lvalue) override;
    std::any visit(Expr::Array* expr, bool as_lvalue) override;
    std::any visit(Expr::Block* expr, bool as_lvalue) override;
    std::any visit(Expr::Conditional* expr, bool as_lvalue) override;
    std::any visit(Expr::Loop* expr, bool as_lvalue) override;

    void run_build();

public:
    /**
     * @brief Builds the MIR for the given front end context.
     *
     * @param context The front end context containing the AST to build MIR for.
     */
    static void build_mir(std::unique_ptr<FrontendContext>& context);
};

} // namespace nico

#endif // NICO_MIR_BUILDER_H
