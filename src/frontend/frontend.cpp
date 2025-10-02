#include "nico/frontend/frontend.h"

std::unique_ptr<FrontendContext>&
Frontend::compile(const std::shared_ptr<CodeFile>& file, bool repl_mode) {
    Lexer::scan(context, file, repl_mode);
    if (context->status == FrontendContext::Status::Error)
        return context;

    Parser::parse(context, repl_mode);
    if (context->status == FrontendContext::Status::Error)
        return context;

    GlobalChecker::check(context);
    if (context->status == FrontendContext::Status::Error)
        return context;

    LocalChecker::check(context);
    if (context->status == FrontendContext::Status::Error)
        return context;

    context->stmts_checked = context->stmts.size();

    CodeGenerator::generate_exe_ir(
        context,
        ir_printing_enabled,
        panic_recoverable
    );

    return context;
}
