#include "nico/frontend/frontend.h"

#include "nico/shared/status.h"

std::unique_ptr<FrontendContext>&
Frontend::compile(const std::shared_ptr<CodeFile>& file, bool repl_mode) {
    Lexer::scan(context, file, repl_mode);
    if (!IS_VARIANT(context->status, Status::Ok))
        return context;

    Parser::parse(context, repl_mode);
    if (!IS_VARIANT(context->status, Status::Ok))
        return context;

    GlobalChecker::check(context);
    if (!IS_VARIANT(context->status, Status::Ok))
        return context;

    LocalChecker::check(context);
    if (!IS_VARIANT(context->status, Status::Ok))
        return context;

    if (repl_mode) {
        CodeGenerator::generate_repl_ir(context, ir_printing_enabled);
    }
    else {
        CodeGenerator::generate_exe_ir(
            context,
            ir_printing_enabled,
            panic_recoverable
        );
    }
    context->stmts_processed = context->stmts.size();

    return context;
}
