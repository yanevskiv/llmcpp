/*
 * C++ header for installing pass callbacks and AST consumers.
 */

#ifndef LLMCPP_FRONTEND_GENERATION_ACTION_H
#define LLMCPP_FRONTEND_GENERATION_ACTION_H

#include "clang/Frontend/FrontendAction.h"
/** Namespace for required llmcpp forward declarations. */
namespace llmcpp
{
    class GenerationPass;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Installs preprocessing callbacks and creates the pass consumer. */
    class FrontendGenerationAction : public ::clang::ASTFrontendAction
    {
    public:
        /**
         * Associate the action with its compilation pass.
         *
         * @param pass Active compilation pass.
         */
        explicit FrontendGenerationAction(GenerationPass &pass);

    protected:
        /**
         * Install prompt-discovery callbacks and return the pass consumer.
         *
         * @param ci Active compiler instance.
         * @param inputFile Input filename.
         * @return Consumer for the parsed AST.
         */
        std::unique_ptr<::clang::ASTConsumer> CreateASTConsumer(::clang::CompilerInstance &ci,
                                                                llvm::StringRef inputFile) override;

    private:
        /** Compilation pass receiving preprocessing and AST callbacks. */
        GenerationPass &m_pass;
    };

}

#endif
