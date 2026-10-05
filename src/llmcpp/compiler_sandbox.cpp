/*
 * C++ file for isolated shadow compilation and diagnostic collection.
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

// Project header for isolated compiler execution.
#include "llmcpp/compiler_sandbox.h"
#include "llmcpp/compiler_diagnostic_collector.h"
#include "llmcpp/compiler_inspection_action.h"

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
    // Capture the compiler invocation used for subsequent shadow runs.
    CompilerSandbox::CompilerSandbox(std::vector<std::string> cc1Args, std::string mainFile,
                                     std::string executable)
        : m_args(std::move(cc1Args))
        , m_main_file(std::move(mainFile))
        , m_executable(std::move(executable))
    {
        // Empty.
    }

    // Compile candidate source and return filtered diagnostics and inspection results.
    data::DataCompilationResult CompilerSandbox::compile(StringRef source, unsigned regionBegin,
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
        ppo.addMacroDef("__LLMCPP__=1");
        ppo.addRemappedFile(m_main_file,
                            llvm::MemoryBuffer::getMemBufferCopy(source, m_main_file).release());

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
        CompilerDiagnosticCollector diags(regionBegin, regionEnd, fileLabel);
        ci.createDiagnostics(&diags, false);
        CompilerInspectionAction action(inspect);
        ci.ExecuteAction(action);

        data::DataCompilationResult r;
        r.m_errors = diags.m_errors;
        r.m_warnings = diags.m_warnings;
        r.m_ok = diags.m_errors == 0;
        r.m_text = std::move(diags.m_text);
        return r;
    }

}
