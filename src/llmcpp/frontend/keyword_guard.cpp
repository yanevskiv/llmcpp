/*
 * C++ file for rejecting unsupported llm annotations in included headers.
 */

// Project header for annotation expansion checks.
#include "llmcpp/frontend/keyword_guard.h"

// Clang headers for compiler state and source diagnostics.
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Token.h"

// LLVM headers for source path formatting.
#include "llvm/Support/Path.h"

// Namespace for annotation expansion checks.
namespace llmcpp
{
    // Namespace for llmcpp frontend implementation.
    namespace frontend
    {
        // Associate the guard with the active compiler instance.
        KeywordGuard::KeywordGuard(::clang::CompilerInstance &ci)
            : m_ci(ci)
        {
            // Empty.
        }

        // Diagnose an unsupported llm annotation macro expansion.
        void KeywordGuard::MacroExpands(const ::clang::Token &name,
                                        const ::clang::MacroDefinition &, ::clang::SourceRange,
                                        const ::clang::MacroArgs *)
        {
            const ::clang::IdentifierInfo *identifier = name.getIdentifierInfo();
            if (!identifier || identifier->getName() != "__llm__") {
                return;
            }
            ::clang::SourceManager &sourceManager = m_ci.getSourceManager();
            ::clang::SourceLocation location = sourceManager.getExpansionLoc(name.getLocation());
            ::clang::DiagnosticsEngine &diagnostics = m_ci.getDiagnostics();
            if (sourceManager.isInMainFile(location)) {
                diagnostics.Report(location, diagnostics.getCustomDiagID(
                                                 ::clang::DiagnosticsEngine::Error,
                                                 "__llm__ is only supported in C++ source files "
                                                 "named on the command line"));
            } else {
                diagnostics.Report(location,
                                   diagnostics.getCustomDiagID(
                                       ::clang::DiagnosticsEngine::Error,
                                       "__llm__ function in included header '%0'; llmc++ only "
                                       "rewrites the main file"))
                    << llvm::sys::path::filename(sourceManager.getFilename(location));
            }
        }
    }
}
