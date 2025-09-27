#include "frontend/frontend.h"

std::unique_ptr<FrontendContext>&
Frontend::compile(const std::shared_ptr<CodeFile>& file, bool repl_mode) {
    lexer.scan(context, file);
    if (context->status == FrontendContext::Status::Error)
        return context;

    parser.parse(context);
    if (context->status == FrontendContext::Status::Error)
        return context;

    global_checker.check(context);
    if (context->status == FrontendContext::Status::Error)
        return context;

    local_checker.check(context);
    if (context->status == FrontendContext::Status::Error)
        return context;

    codegen.generate_executable_ir(context, true);
    codegen.add_module_to_context(context);

    return context;
}
