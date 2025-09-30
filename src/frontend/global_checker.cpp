#include "nico/frontend/global_checker.h"

void GlobalChecker::run_check(std::unique_ptr<FrontendContext>& context) {
    // Currently, the global checker does nothing.
    // This is a placeholder for future functionality.
    return;
}

void GlobalChecker::check(std::unique_ptr<FrontendContext>& context) {
    if (context->status == FrontendContext::Status::Error) {
        panic("GlobalChecker::check: Context is already in an error state.");
    }

    GlobalChecker checker;
    checker.run_check(context);
}
