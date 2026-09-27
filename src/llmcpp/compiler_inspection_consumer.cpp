/*
 * C++ file for semantic inspection after shadow compilation.
 */

// Project header for shadow AST inspection.
#include "llmcpp/compiler_inspection_consumer.h"

// Clang headers for compiler semantic state.
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"

// Namespace for shadow AST inspection.
namespace llmcpp
{
    // Bind the consumer to a compiler and inspection callback.
    CompilerInspectionConsumer::CompilerInspectionConsumer(::clang::CompilerInstance &ci,
                                                           InspectFn inspect)
        : m_ci(ci)
        , m_inspect(inspect)
    {
        // Empty.
    }

    // Run semantic inspection for the completed translation unit.
    void CompilerInspectionConsumer::HandleTranslationUnit(::clang::ASTContext &ctx)
    {
        if (m_inspect && m_ci.hasSema() && !m_ci.getDiagnostics().hasErrorOccurred()) {
            m_inspect(ctx, m_ci.getSema());
        }
    }
}
