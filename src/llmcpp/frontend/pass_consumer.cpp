/*
 * C++ file for dispatching parsed translation units to an llm pass.
 */

// Project headers for pass AST dispatch.
#include "llmcpp/frontend/pass_consumer.h"
#include "llmcpp/generation/pass.h"

// Namespace for pass AST dispatch.
namespace llmcpp
{
    // Namespace for llmcpp frontend implementation.
    namespace frontend
    {
        // Associate the consumer with its compilation pass.
        PassConsumer::PassConsumer(generation::Pass &pass)
            : m_pass(pass)
        {
            // Empty.
        }

        // Dispatch the parsed AST to the pass.
        void PassConsumer::HandleTranslationUnit(::clang::ASTContext &ctx)
        {
            m_pass.run(ctx);
        }
    }
}
