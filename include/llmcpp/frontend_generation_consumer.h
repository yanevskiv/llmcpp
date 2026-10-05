/*
 * C++ header for dispatching parsed translation units to an llm pass.
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
