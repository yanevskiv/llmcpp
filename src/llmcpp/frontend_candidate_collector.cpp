/*
 * C++ file for finding function and lambda annotation candidates.
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

// Project header for AST candidate collection.
#include "llmcpp/frontend_candidate_collector.h"

// Clang headers for function and lambda declarations.
#include "clang/AST/ASTLambda.h"
#include "clang/AST/Decl.h"

// Namespace for AST candidate collection.
namespace llmcpp
{
    // Avoid collecting duplicate template instantiations.
    bool FrontendCandidateCollector::shouldVisitTemplateInstantiations() const
    {
        return false;
    }

    // Restrict candidates to source-written entities.
    bool FrontendCandidateCollector::shouldVisitImplicitCode() const
    {
        return false;
    }

    // Record a source-written non-lambda function declaration.
    bool FrontendCandidateCollector::VisitFunctionDecl(::clang::FunctionDecl *declaration)
    {
        if (!declaration->isImplicit() && !::clang::isLambdaCallOperator(declaration)) {
            m_functions.push_back(declaration);
        }
        return true;
    }

    // Record a source-written lambda expression.
    bool FrontendCandidateCollector::VisitLambdaExpr(::clang::LambdaExpr *expression)
    {
        m_lambdas.push_back(expression);
        return true;
    }
}
