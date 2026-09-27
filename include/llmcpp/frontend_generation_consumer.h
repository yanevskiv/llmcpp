/*
 * C++ header for dispatching parsed translation units to an llm pass.
 */

#ifndef LLMCPP_FRONTEND_GENERATION_CONSUMER_H
#define LLMCPP_FRONTEND_GENERATION_CONSUMER_H

#include "clang/AST/ASTConsumer.h"
/** Namespace for required llmcpp forward declarations. */
namespace llmcpp
{
    class GenerationPass;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Dispatches a completed translation unit to an llm pass. */
    class FrontendGenerationConsumer : public ::clang::ASTConsumer
    {
    public:
        /**
         * Associate the consumer with its compilation pass.
         *
         * @param pass Active compilation pass.
         */
        explicit FrontendGenerationConsumer(GenerationPass &pass);
        /**
         * Dispatch the parsed AST to the pass.
         *
         * @param ctx Completed AST context.
         */
        void HandleTranslationUnit(::clang::ASTContext &ctx) override;

    private:
        /** Compilation pass receiving the completed AST. */
        GenerationPass &m_pass;
    };

}

#endif
