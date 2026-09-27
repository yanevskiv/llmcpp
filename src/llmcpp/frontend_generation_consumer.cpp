/*
 * C++ file for dispatching parsed translation units to an llm pass.
 */

// Project headers for pass AST dispatch.
#include "llmcpp/frontend_generation_consumer.h"
#include "llmcpp/generation_pass.h"

// Namespace for pass AST dispatch.
namespace llmcpp
{
    // Associate the consumer with its compilation pass.
    FrontendGenerationConsumer::FrontendGenerationConsumer(GenerationPass &pass)
        : m_pass(pass)
    {
        // Empty.
    }

    // Dispatch the parsed AST to the pass.
    void FrontendGenerationConsumer::HandleTranslationUnit(::clang::ASTContext &ctx)
    {
        m_pass.run(ctx);
    }
}
