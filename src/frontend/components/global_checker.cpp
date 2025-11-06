#include "nico/frontend/components/global_checker.h"

#include "nico/shared/logger.h"
#include "nico/shared/status.h"

namespace nico {

std::any GlobalChecker::visit(Stmt::Expression*) {
    // Do nothing.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Let*) {
    // Do nothing.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Func*) {
    // TODO: Implement global function checks.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Print*) {
    // Do nothing.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Pass*) {
    // Do nothing.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Yield*) {
    // Do nothing.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Continue*) {
    // Do nothing.
    return std::any();
}

std::any GlobalChecker::visit(Stmt::Eof*) {
    // Do nothing.
    return std::any();
}

void GlobalChecker::run_check(std::unique_ptr<FrontendContext>& context) {
    symbol_tree->clear_modified();

    for (size_t i = context->stmts_processed; i < context->stmts.size(); ++i) {
        context->stmts[i]->accept(this);
    }

    if (Logger::inst().get_errors().empty()) {
        context->status = Status::Ok();
    }
    else if (repl_mode) {
        if (symbol_tree->was_modified()) {
            context->status = Status::Pause(Request::DiscardWarn);
        }
        else {
            context->status = Status::Pause(Request::Discard);
        }
        context->rollback();
    }
    else {
        context->status = Status::Error();
    }
}

void GlobalChecker::check(
    std::unique_ptr<FrontendContext>& context, bool repl_mode
) {
    if (IS_VARIANT(context->status, Status::Error)) {
        panic("GlobalChecker::check: Context is already in an error state.");
    }

    GlobalChecker checker(context->symbol_tree, repl_mode);
    checker.run_check(context);
}

} // namespace nico
