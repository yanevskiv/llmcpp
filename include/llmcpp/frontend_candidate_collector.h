/*
 * C++ header for finding function and lambda annotation candidates.
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

#ifndef LLMCPP_FRONTEND_CANDIDATE_COLLECTOR_H
#define LLMCPP_FRONTEND_CANDIDATE_COLLECTOR_H

#include "clang/AST/RecursiveASTVisitor.h"

#include <vector>
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class FunctionDecl;
    class LambdaExpr;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Collects source-written functions and lambda expressions from an AST. */
    class FrontendCandidateCollector
        : public ::clang::RecursiveASTVisitor<FrontendCandidateCollector>
    {
    public:
        /**
         * Avoid collecting duplicate template instantiations.
         *
         * @return Whether template instantiations should be visited.
         */
        bool shouldVisitTemplateInstantiations() const;
        /**
         * Restrict candidates to source-written entities.
         *
         * @return Whether implicit code should be visited.
         */
        bool shouldVisitImplicitCode() const;
        /**
         * Record a source-written non-lambda function declaration.
         *
         * @param declaration Visited function declaration.
         * @return Whether traversal should continue.
         */
        bool VisitFunctionDecl(::clang::FunctionDecl *declaration);
        /**
         * Record a source-written lambda expression.
         *
         * @param expression Visited lambda expression.
         * @return Whether traversal should continue.
         */
        bool VisitLambdaExpr(::clang::LambdaExpr *expression);
        /** Collected source-written function declarations. */
        std::vector<::clang::FunctionDecl *> m_functions;
        /** Collected source-written lambda expressions. */
        std::vector<::clang::LambdaExpr *> m_lambdas;
    };

}

#endif
