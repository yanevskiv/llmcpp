/*
 * C++ header for semantic inspection after shadow compilation.
 */

#ifndef LLMCPP_COMPILER_INSPECTION_CONSUMER_H
#define LLMCPP_COMPILER_INSPECTION_CONSUMER_H

#include "llmcpp/compiler_sandbox.h"

#include "clang/AST/ASTConsumer.h"
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class CompilerInstance;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Invokes semantic inspection after a shadow translation unit completes. */
    class CompilerInspectionConsumer : public ::clang::ASTConsumer
    {
    public:
        /**
         * Bind the consumer to a compiler and inspection callback.
         *
         * @param ci Active shadow compiler.
         * @param inspect Callback invoked for a completed AST.
         */
        CompilerInspectionConsumer(::clang::CompilerInstance &ci, InspectFn inspect);
        /**
         * Run semantic inspection for the completed translation unit.
         *
         * @param ctx Completed AST context.
         */
        void HandleTranslationUnit(::clang::ASTContext &ctx) override;

    private:
        /** Active shadow compiler. */
        ::clang::CompilerInstance &m_ci;
        /** Callback invoked after successful parsing. */
        InspectFn m_inspect;
    };

}

#endif
