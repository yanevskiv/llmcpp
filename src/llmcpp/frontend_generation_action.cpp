/*
 * C++ file for installing pass callbacks and AST consumers.
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

// Project headers for preprocessing callbacks and pass AST dispatch.
#include "llmcpp/frontend_generation_action.h"
#include "llmcpp/frontend_generation_consumer.h"
#include "llmcpp/frontend_source_collector.h"
#include "llmcpp/generation_pass.h"

// Clang headers for preprocessing.
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"

// Namespace for pass frontend actions.
namespace llmcpp
{
    // Associate the action with its compilation pass.
    FrontendGenerationAction::FrontendGenerationAction(GenerationPass &pass)
        : m_pass(pass)
    {
        // Empty.
    }

    // Install prompt-discovery callbacks and return the pass consumer.
    std::unique_ptr<::clang::ASTConsumer>
    FrontendGenerationAction::CreateASTConsumer(::clang::CompilerInstance &ci, llvm::StringRef)
    {
        ci.getPreprocessor().addPPCallbacks(std::make_unique<FrontendSourceCollector>(
            ci.getSourceManager(), m_pass.keyword_locations(), m_pass.state().m_includes));
        return std::make_unique<FrontendGenerationConsumer>(m_pass);
    }
}
