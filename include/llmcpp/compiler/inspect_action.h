/*
 * C++ header for creating shadow-compilation inspection consumers.
 */

#ifndef LLMCPP_INSPECT_ACTION_H
#define LLMCPP_INSPECT_ACTION_H

#include "llmcpp/compiler/shadow_compiler.h"

#include "clang/Frontend/FrontendAction.h"
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp compiler declarations. */
    namespace compiler
    {
        /** Creates consumers that inspect successful shadow compilations. */
        class InspectAction : public ::clang::ASTFrontendAction
        {
        public:
            /**
             * Store the inspection callback for consumer construction.
             *
             * @param inspect Callback invoked for a completed shadow AST.
             */
            explicit InspectAction(InspectFn inspect);

        protected:
            /**
             * Create the consumer that receives the shadow AST.
             *
             * @param ci Active shadow compiler.
             * @param inputFile Input filename.
             * @return Consumer for the shadow AST.
             */
            std::unique_ptr<::clang::ASTConsumer>
            CreateASTConsumer(::clang::CompilerInstance &ci, llvm::StringRef inputFile) override;

        private:
            /** Callback passed to constructed inspection consumers. */
            InspectFn m_inspect;
        };

    }
}

#endif
