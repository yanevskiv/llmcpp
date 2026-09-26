/*
 * C++ header for a frontend action that installs llm annotation guards.
 */

#ifndef LLMCPP_GUARD_ACTION_H
#define LLMCPP_GUARD_ACTION_H

#include "clang/Frontend/FrontendAction.h"

#include <memory>
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp frontend declarations. */
    namespace frontend
    {
        /** Wraps a frontend action with checks for unsupported annotations. */
        class GuardAction : public ::clang::WrapperFrontendAction
        {
        public:
            /**
             * Wrap the frontend action that performs compilation.
             *
             * @param wrapped Frontend action to run after guard installation.
             */
            explicit GuardAction(std::unique_ptr<::clang::FrontendAction> wrapped);

        protected:
            /**
             * Install keyword checks before preprocessing begins.
             *
             * @param ci Active compiler instance.
             * @return Whether setup succeeded.
             */
            bool BeginSourceFileAction(::clang::CompilerInstance &ci) override;
        };

    }
}

#endif
