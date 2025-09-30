#include "nico/frontend/frontend.h"

std::unique_ptr<FrontendContext>&
Frontend::compile(const std::shared_ptr<CodeFile>& file, bool repl_mode) {
    Lexer::scan(context, file);
    if (context->status == FrontendContext::Status::Error)
        return context;

    Parser parser;
    parser.parse(context);
    if (context->status == FrontendContext::Status::Error)
        return context;

    GlobalChecker global_checker;
    global_checker.check(context);
    if (context->status == FrontendContext::Status::Error)
        return context;

    LocalChecker local_checker;
    local_checker.check(context);
    if (context->status == FrontendContext::Status::Error)
        return context;

    CodeGenerator codegen;
    codegen.set_panic_recoverable(panic_recoverable);
    codegen.set_ir_printing_enabled(ir_printing_enabled);
    codegen.generate_executable_ir(context, true);
    codegen.add_module_to_context(context);

    return context;
}
