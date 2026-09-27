/*
 * C++ file for finding function and lambda annotation candidates.
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
