/*
 * C++ header for locating generated type aliases in shadow ASTs.
 */

#ifndef LLMCPP_PROBE_FINDER_H
#define LLMCPP_PROBE_FINDER_H

#include "clang/AST/RecursiveASTVisitor.h"
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class TypeAliasDecl;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp compiler declarations. */
    namespace compiler
    {
        /** Finds the temporary type alias inserted for a type probe. */
        class ProbeFinder : public ::clang::RecursiveASTVisitor<ProbeFinder>
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
}

#endif
