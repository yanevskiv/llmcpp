/*
 * C++ file for semantic inspection after shadow compilation.
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

// Project header for shadow AST inspection.
#include "llmcpp/compiler_inspection_consumer.h"

// Clang headers for compiler semantic state.
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"

// Namespace for shadow AST inspection.
namespace llmcpp
{
    // Bind the consumer to a compiler and inspection callback.
    CompilerInspectionConsumer::CompilerInspectionConsumer(::clang::CompilerInstance &ci,
                                                           InspectFn inspect)
        : m_ci(ci)
        , m_inspect(inspect)
    {
        // Empty.
    }

    // Run semantic inspection for the completed translation unit.
    void CompilerInspectionConsumer::HandleTranslationUnit(::clang::ASTContext &ctx)
    {
        if (m_inspect && m_ci.hasSema() && !m_ci.getDiagnostics().hasErrorOccurred()) {
            m_inspect(ctx, m_ci.getSema());
        }
    }
}
