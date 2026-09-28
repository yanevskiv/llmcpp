/*
 * C++ file for creating shadow-compilation inspection consumers.
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

// Project headers for shadow AST inspection and actions.
#include "llmcpp/compiler_inspection_action.h"
#include "llmcpp/compiler_inspection_consumer.h"

// Namespace for shadow AST inspection actions.
namespace llmcpp
{
    // Store the inspection callback for consumer construction.
    CompilerInspectionAction::CompilerInspectionAction(InspectFn inspect)
        : m_inspect(inspect)
    {
        // Empty.
    }

    // Create the consumer that receives the shadow AST.
    std::unique_ptr<::clang::ASTConsumer>
    CompilerInspectionAction::CreateASTConsumer(::clang::CompilerInstance &ci, llvm::StringRef)
    {
        return std::make_unique<CompilerInspectionConsumer>(ci, m_inspect);
    }
}
