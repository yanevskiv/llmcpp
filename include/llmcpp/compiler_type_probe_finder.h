/*
 * C++ header for locating generated type aliases in shadow ASTs.
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

#ifndef LLMCPP_COMPILER_TYPE_PROBE_FINDER_H
#define LLMCPP_COMPILER_TYPE_PROBE_FINDER_H

#include "clang/AST/RecursiveASTVisitor.h"
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class TypeAliasDecl;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Finds the temporary type alias inserted for a type probe. */
    class CompilerTypeProbeFinder : public ::clang::RecursiveASTVisitor<CompilerTypeProbeFinder>
    {
    public:
        /**
         * Record a generated probe alias when encountered.
         *
         * @param declaration Visited type-alias declaration.
         * @return Whether traversal should continue.
         */
        bool VisitTypeAliasDecl(::clang::TypeAliasDecl *declaration);
        /** Generated probe alias found during AST traversal. */
        const ::clang::TypeAliasDecl *m_found = nullptr;
    };

}

#endif
