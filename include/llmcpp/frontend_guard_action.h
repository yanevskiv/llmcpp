/*
 * C++ header for a frontend action that installs llm annotation guards.
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

#ifndef LLMCPP_FRONTEND_GUARD_ACTION_H
#define LLMCPP_FRONTEND_GUARD_ACTION_H

#include "clang/Frontend/FrontendAction.h"

#include <memory>
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Wraps a frontend action with checks for unsupported annotations. */
    class FrontendGuardAction : public ::clang::WrapperFrontendAction
    {
    public:
        /**
         * Wrap the frontend action that performs compilation.
         *
         * @param wrapped Frontend action to run after guard installation.
         */
        explicit FrontendGuardAction(std::unique_ptr<::clang::FrontendAction> wrapped);

    protected:
        /**
         * Install keyword checks before preprocessing begins.
         *
         * @param ci Active compiler instance.
         * @return Whether setup succeeded.
         */
        bool BeginSourceFileAction(::clang::CompilerInstance &ci) override;
    };

}

#endif
