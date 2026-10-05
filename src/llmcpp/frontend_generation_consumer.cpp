/*
 * C++ file for dispatching parsed translation units to an llm pass.
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

// Project headers for pass AST dispatch.
#include "llmcpp/frontend_generation_consumer.h"
#include "llmcpp/generation_pass.h"

// Namespace for pass AST dispatch.
namespace llmcpp
{
    // Associate the consumer with its compilation pass.
    FrontendGenerationConsumer::FrontendGenerationConsumer(GenerationPass &pass)
        : m_pass(pass)
    {
        // Empty.
    }

    // Dispatch the parsed AST to the pass.
    void FrontendGenerationConsumer::HandleTranslationUnit(::clang::ASTContext &ctx)
    {
        m_pass.run(ctx);
    }
}
