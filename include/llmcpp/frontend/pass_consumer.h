/*
 * C++ header for dispatching parsed translation units to an llm pass.
 */

#ifndef LLMCPP_PASS_CONSUMER_H
#define LLMCPP_PASS_CONSUMER_H

#include "clang/AST/ASTConsumer.h"
/** Namespace for required llmcpp forward declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp generation declarations. */
    namespace generation
    {
        class Pass;
    }
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp frontend declarations. */
    namespace frontend
    {
        /** Dispatches a completed translation unit to an llm pass. */
        class PassConsumer : public ::clang::ASTConsumer
        {
        public:
            /**
             * Associate the consumer with its compilation pass.
             *
             * @param pass Active compilation pass.
             */
            explicit PassConsumer(generation::Pass &pass);
            /**
             * Dispatch the parsed AST to the pass.
             *
             * @param ctx Completed AST context.
             */
            void HandleTranslationUnit(::clang::ASTContext &ctx) override;

        private:
            /** Compilation pass receiving the completed AST. */
            generation::Pass &m_pass;
        };

    }
}

#endif
