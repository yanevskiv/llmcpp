/*
 * C++ file for installing pass callbacks and AST consumers.
 */

// Project headers for preprocessing callbacks and pass AST dispatch.
#include "llmcpp/frontend/pass_action.h"
#include "llmcpp/frontend/callbacks.h"
#include "llmcpp/frontend/pass_consumer.h"
#include "llmcpp/generation/pass.h"

// Clang headers for preprocessing.
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"

// Namespace for pass frontend actions.
namespace llmcpp
{
    // Namespace for llmcpp frontend implementation.
    namespace frontend
    {
        // Associate the action with its compilation pass.
        PassAction::PassAction(generation::Pass &pass)
            : m_pass(pass)
        {
            // Empty.
        }

        // Install prompt-discovery callbacks and return the pass consumer.
        std::unique_ptr<::clang::ASTConsumer>
        PassAction::CreateASTConsumer(::clang::CompilerInstance &ci, llvm::StringRef)
        {
            ci.getPreprocessor().addPPCallbacks(std::make_unique<Callbacks>(
                ci.getSourceManager(), m_pass.keyword_locations(), m_pass.state().m_includes));
            return std::make_unique<PassConsumer>(m_pass);
        }
    }
}
