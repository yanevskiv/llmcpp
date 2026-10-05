/*
 * C++ header for installing pass callbacks and AST consumers.
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
