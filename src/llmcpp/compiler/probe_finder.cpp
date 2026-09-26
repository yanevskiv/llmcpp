/*
 * C++ file for locating generated type aliases in shadow ASTs.
 */

// Project header for generated type-alias discovery.
#include "llmcpp/compiler/probe_finder.h"

// Clang headers for type-alias declarations.
#include "clang/AST/Decl.h"

// Namespace for generated type-alias discovery.
namespace llmcpp
{
    // Namespace for llmcpp compiler implementation.
    namespace compiler
    {
        // Record a generated probe alias when encountered.
        bool ProbeFinder::VisitTypeAliasDecl(::clang::TypeAliasDecl *declaration)
        {
            if (declaration->getName() != "__llmcpp_probe_type") {
                return true;
            }
            m_found = declaration;
            return false;
        }
    }
}
