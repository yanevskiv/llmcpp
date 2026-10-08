/*
 * C++ header for semantic inspection after shadow compilation.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * llmc++  is  free  software;  you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free  Software  Foundation;  either  version 3 of the License, or (at
 * your option) any later version.
 *
 * llmc++ is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS  FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You  should  have  received  a copy of the GNU General Public License
 * along with llmc++; if not, see <https://www.gnu.org/licenses/>.
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
