/*
 * C++ file for installing pass callbacks and AST consumers.
 */

// Project headers for preprocessing callbacks and pass AST dispatch.
#include "llmcpp/frontend_generation_action.h"
#include "llmcpp/frontend_generation_consumer.h"
#include "llmcpp/frontend_source_collector.h"
#include "llmcpp/generation_pass.h"

// Clang headers for preprocessing.
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"

// Namespace for pass frontend actions.
namespace llmcpp
{
    // Associate the action with its compilation pass.
    FrontendGenerationAction::FrontendGenerationAction(GenerationPass &pass)
        : m_pass(pass)
    {
        // Empty.
    }

    // Install prompt-discovery callbacks and return the pass consumer.
    std::unique_ptr<::clang::ASTConsumer>
    FrontendGenerationAction::CreateASTConsumer(::clang::CompilerInstance &ci, llvm::StringRef)
    {
        ci.getPreprocessor().addPPCallbacks(std::make_unique<FrontendSourceCollector>(
            ci.getSourceManager(), m_pass.keyword_locations(), m_pass.state().m_includes));
        return std::make_unique<FrontendGenerationConsumer>(m_pass);
    }
}
