/*
 * C++ file for a frontend action that installs llm annotation guards.
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

// Project headers for annotation expansion checks and guard actions.
#include "llmcpp/frontend_guard_action.h"
#include "llmcpp/frontend_header_annotation_guard.h"

// Clang headers for preprocessing.
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"

// Namespace for guarded frontend actions.
namespace llmcpp
{
    // Wrap the frontend action that performs compilation.
    FrontendGuardAction::FrontendGuardAction(std::unique_ptr<::clang::FrontendAction> wrapped)
        : ::clang::WrapperFrontendAction(std::move(wrapped))
    {
        // Empty.
    }

    // Install keyword checks before preprocessing begins.
    bool FrontendGuardAction::BeginSourceFileAction(::clang::CompilerInstance &ci)
    {
        ci.getPreprocessor().addPPCallbacks(std::make_unique<FrontendHeaderAnnotationGuard>(ci));
        return ::clang::WrapperFrontendAction::BeginSourceFileAction(ci);
    }
}
