/*
 * C++ file for a frontend action that installs llm annotation guards.
 */

// Project headers for annotation expansion checks and guard actions.
#include "llmcpp/frontend_guard_action.h"
#include "llmcpp/frontend_header_annotation_guard.h"

// Clang headers for preprocessing.
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"

// Namespace for guarded frontend actions.
namespace llmcpp
{
    // Wrap the frontend action that performs compilation.
    FrontendGuardAction::FrontendGuardAction(std::unique_ptr<::clang::FrontendAction> wrapped)
        : ::clang::WrapperFrontendAction(std::move(wrapped))
    {
        // Empty.
    }

    // Install keyword checks before preprocessing begins.
    bool FrontendGuardAction::BeginSourceFileAction(::clang::CompilerInstance &ci)
    {
        ci.getPreprocessor().addPPCallbacks(std::make_unique<FrontendHeaderAnnotationGuard>(ci));
        return ::clang::WrapperFrontendAction::BeginSourceFileAction(ci);
    }
}
