/*
 * C++ header for rejecting unsupported llm annotations in included headers.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * llmc++ is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * llmc++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with llmc++; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef LLMCPP_FRONTEND_HEADER_ANNOTATION_GUARD_H
#define LLMCPP_FRONTEND_HEADER_ANNOTATION_GUARD_H

#include "clang/Lex/PPCallbacks.h"
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class CompilerInstance;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Rejects llm annotations that expand from included headers. */
    class FrontendHeaderAnnotationGuard : public ::clang::PPCallbacks
    {
    public:
        /**
         * Associate the guard with the active compiler instance.
         *
         * @param ci Active compiler instance.
         */
        explicit FrontendHeaderAnnotationGuard(::clang::CompilerInstance &ci);
        /**
         * Diagnose an unsupported llm annotation macro expansion.
         *
         * @param name Expanded macro name.
         * @param definition Macro definition information.
         * @param range Expansion source range.
         * @param args Macro arguments when present.
         */
        void MacroExpands(const ::clang::Token &name, const ::clang::MacroDefinition &definition,
                          ::clang::SourceRange range, const ::clang::MacroArgs *args) override;

    private:
        /** Active compiler instance used to emit diagnostics. */
        ::clang::CompilerInstance &m_ci;
    };

}

#endif
