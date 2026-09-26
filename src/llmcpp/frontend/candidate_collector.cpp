/*
 * C++ file for finding function and lambda annotation candidates.
 */

// Project header for AST candidate collection.
#include "llmcpp/frontend/candidate_collector.h"

// Clang headers for function and lambda declarations.
#include "clang/AST/ASTLambda.h"
#include "clang/AST/Decl.h"

// Namespace for AST candidate collection.
namespace llmcpp
{
    // Namespace for llmcpp frontend implementation.
    namespace frontend
    {
        // Avoid collecting duplicate template instantiations.
        bool CandidateCollector::shouldVisitTemplateInstantiations() const
        {
            return false;
        }

        // Restrict candidates to source-written entities.
        bool CandidateCollector::shouldVisitImplicitCode() const
        {
            return false;
        }

        // Record a source-written non-lambda function declaration.
        bool CandidateCollector::VisitFunctionDecl(::clang::FunctionDecl *declaration)
        {
            if (!declaration->isImplicit() && !::clang::isLambdaCallOperator(declaration)) {
                m_functions.push_back(declaration);
            }
            return true;
        }

        // Record a source-written lambda expression.
        bool CandidateCollector::VisitLambdaExpr(::clang::LambdaExpr *expression)
        {
            m_lambdas.push_back(expression);
            return true;
        }
    }
}
