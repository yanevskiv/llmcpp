/*
 * C++ header for rejecting unsupported llm annotations in included headers.
 */

#ifndef LLMCPP_KEYWORD_GUARD_H
#define LLMCPP_KEYWORD_GUARD_H

#include "clang/Lex/PPCallbacks.h"
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class CompilerInstance;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp frontend declarations. */
    namespace frontend
    {
        /** Rejects llm annotations that expand from included headers. */
        class KeywordGuard : public ::clang::PPCallbacks
        {
        public:
            /**
             * Associate the guard with the active compiler instance.
             *
             * @param ci Active compiler instance.
             */
            explicit KeywordGuard(::clang::CompilerInstance &ci);
            /**
             * Diagnose an unsupported llm annotation macro expansion.
             *
             * @param name Expanded macro name.
             * @param definition Macro definition information.
             * @param range Expansion source range.
             * @param args Macro arguments when present.
             */
            void MacroExpands(const ::clang::Token &name,
                              const ::clang::MacroDefinition &definition,
                              ::clang::SourceRange range, const ::clang::MacroArgs *args) override;

        private:
            /** Active compiler instance used to emit diagnostics. */
            ::clang::CompilerInstance &m_ci;
        };

    }
}

#endif
