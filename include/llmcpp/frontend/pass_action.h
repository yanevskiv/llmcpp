/*
 * C++ header for installing pass callbacks and AST consumers.
 */

#ifndef LLMCPP_PASS_ACTION_H
#define LLMCPP_PASS_ACTION_H

#include "clang/Frontend/FrontendAction.h"
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
        /** Installs preprocessing callbacks and creates the pass consumer. */
        class PassAction : public ::clang::ASTFrontendAction
        {
        public:
            /**
             * Associate the action with its compilation pass.
             *
             * @param pass Active compilation pass.
             */
            explicit PassAction(generation::Pass &pass);

        protected:
            /**
             * Install prompt-discovery callbacks and return the pass consumer.
             *
             * @param ci Active compiler instance.
             * @param inputFile Input filename.
             * @return Consumer for the parsed AST.
             */
            std::unique_ptr<::clang::ASTConsumer>
            CreateASTConsumer(::clang::CompilerInstance &ci, llvm::StringRef inputFile) override;

        private:
            /** Compilation pass receiving preprocessing and AST callbacks. */
            generation::Pass &m_pass;
        };

    }
}

#endif
