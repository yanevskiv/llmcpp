/*
 * C++ file for isolated shadow compilation and diagnostic collection.
 */

// Project header for isolated compiler execution.
#include "llmcpp/compiler/shadow_compiler.h"
#include "llmcpp/compiler/collector.h"
#include "llmcpp/compiler/inspect_action.h"

// Clang headers for frontend execution and diagnostic collection.
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/DependencyOutputOptions.h"
#include "clang/Lex/PreprocessorOptions.h"

// LLVM headers for algorithms, paths, and memory buffers.
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/MemoryBuffer.h"

// Namespace import for Clang frontend types.
using namespace clang;
// Type alias for lightweight LLVM string views.
using llvm::StringRef;

// Namespace for isolated compiler runs used to validate generated code.
namespace llmcpp
{
    // Namespace for llmcpp compiler implementation.
    namespace compiler
    {
        // Capture the compiler invocation used for subsequent shadow runs.
        ShadowCompiler::ShadowCompiler(std::vector<std::string> cc1Args, std::string mainFile,
                                       std::string executable)
            : m_args(std::move(cc1Args))
            , m_main_file(std::move(mainFile))
            , m_executable(std::move(executable))
        {
            // Empty.
        }

        // Compile candidate source and return filtered diagnostics and inspection results.
        data::ShadowResult ShadowCompiler::compile(StringRef source, unsigned regionBegin,
                                                   unsigned regionEnd, bool extraWarnings,
                                                   StringRef fileLabel, InspectFn inspect)
        {
            std::vector<const char *> argv;
            for (const std::string &a : m_args) {
                argv.push_back(a.c_str());
            }

            auto inv = std::make_shared<CompilerInvocation>();
            IgnoringDiagConsumer ignoreArgs;
            DiagnosticsEngine argDiags(new DiagnosticIDs(), new DiagnosticOptions(), &ignoreArgs,
                                       false);
            CompilerInvocation::CreateFromArgs(*inv, argv, argDiags, m_executable.c_str());

            FrontendOptions &fo = inv->getFrontendOpts();
            fo.ProgramAction = frontend::ParseSyntaxOnly;
            fo.OutputFile.clear();
            fo.DisableFree = false;
            inv->getDependencyOutputOpts() = DependencyOutputOptions();

            PreprocessorOptions &ppo = inv->getPreprocessorOpts();
            ppo.addMacroDef("__llm__=");
            ppo.addRemappedFile(
                m_main_file, llvm::MemoryBuffer::getMemBufferCopy(source, m_main_file).release());

            DiagnosticOptions &diagOpts = inv->getDiagnosticOpts();
            diagOpts.ShowColors = false;
            diagOpts.ShowCarets = false;
            diagOpts.ErrorLimit = 20;
            if (extraWarnings && !llvm::is_contained(diagOpts.Warnings, "error")) {
                diagOpts.Warnings.push_back("all");
                diagOpts.Warnings.push_back("extra");
            }

            CompilerInstance ci;
            ci.setInvocation(std::move(inv));
            Collector diags(regionBegin, regionEnd, fileLabel);
            ci.createDiagnostics(&diags, false);
            InspectAction action(inspect);
            ci.ExecuteAction(action);

            data::ShadowResult r;
            r.m_errors = diags.m_errors;
            r.m_warnings = diags.m_warnings;
            r.m_ok = diags.m_errors == 0;
            r.m_text = std::move(diags.m_text);
            return r;
        }

    }
}
