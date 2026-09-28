/*
 * C++ header for creating shadow-compilation inspection consumers.
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

#ifndef LLMCPP_COMPILER_INSPECTION_ACTION_H
#define LLMCPP_COMPILER_INSPECTION_ACTION_H

#include "llmcpp/compiler_sandbox.h"

#include "clang/Frontend/FrontendAction.h"
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Creates consumers that inspect successful shadow compilations. */
    class CompilerInspectionAction : public ::clang::ASTFrontendAction
    {
    public:
        /**
         * Store the inspection callback for consumer construction.
         *
         * @param inspect Callback invoked for a completed shadow AST.
         */
        explicit CompilerInspectionAction(InspectFn inspect);

    protected:
        /**
         * Create the consumer that receives the shadow AST.
         *
         * @param ci Active shadow compiler.
         * @param inputFile Input filename.
         * @return Consumer for the shadow AST.
         */
        std::unique_ptr<::clang::ASTConsumer> CreateASTConsumer(::clang::CompilerInstance &ci,
                                                                llvm::StringRef inputFile) override;

    private:
        /** Callback passed to constructed inspection consumers. */
        InspectFn m_inspect;
    };

}

#endif
