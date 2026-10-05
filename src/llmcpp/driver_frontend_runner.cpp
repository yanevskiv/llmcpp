/*
 * C++ file for in-process execution of Clang frontend jobs.
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

// Project headers for annotation guards and frontend execution.
#include "llmcpp/driver_frontend_runner.h"
#include "llmcpp/frontend_guard_action.h"

// Clang headers for frontend setup and execution.
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/CodeGen/ObjectFilePCHContainerOperations.h"
#include "clang/Driver/Types.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/TextDiagnosticBuffer.h"
#include "clang/FrontendTool/Utils.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PreprocessorOptions.h"

// LLVM headers for remapped source buffers and command-line options.
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"

// Namespace imports for Clang frontend types.
using namespace clang;
// Namespace import for Clang driver input kinds.
using namespace ::clang::driver;

// Namespace for in-process Clang frontend execution.
namespace llmcpp
{

    // Store rewritten inputs used by in-process frontend jobs.
    llvm::StringMap<std::string> rewritten_inputs;

    // Run one Clang -cc1 invocation.
    int DriverFrontendRunner::run_cc1(llvm::ArrayRef<const char *> args, const char *argv0)
    {
        auto ci = std::make_unique<CompilerInstance>();
        auto pchOps = ci->getPCHContainerOperations();
        pchOps->registerWriter(std::make_unique<ObjectFilePCHContainerWriter>());
        pchOps->registerReader(std::make_unique<ObjectFilePCHContainerReader>());

        IntrusiveRefCntPtr<DiagnosticIDs> diagId(new DiagnosticIDs());
        IntrusiveRefCntPtr<DiagnosticOptions> diagOpts = new DiagnosticOptions();
        auto *diagsBuffer = new TextDiagnosticBuffer();
        DiagnosticsEngine diags(diagId, diagOpts, diagsBuffer);
        bool success = CompilerInvocation::CreateFromArgs(ci->getInvocation(), args, diags, argv0);
        ci->getPreprocessorOpts().addMacroDef("__LLMCPP__=1");

        HeaderSearchOptions &hsOpts = ci->getHeaderSearchOpts();
        if (hsOpts.UseBuiltinIncludes && hsOpts.ResourceDir.empty()) {
            hsOpts.ResourceDir =
                CompilerInvocation::GetResourcesPath(argv0, (void *)(intptr_t)anchor);
        }

        for (const FrontendInputFile &input : ci->getFrontendOpts().Inputs) {
            if (!input.isFile()) {
                continue;
            }
            auto it = rewritten_inputs.find(input.getFile());
            if (it == rewritten_inputs.end()) {
                continue;
            }
            ci->getPreprocessorOpts().addRemappedFile(
                input.getFile(),
                llvm::MemoryBuffer::getMemBufferCopy(it->second, input.getFile()).release());
        }

        ci->createDiagnostics();
        if (!ci->hasDiagnostics()) {
            return 1;
        }
        diagsBuffer->FlushDiagnostics(ci->getDiagnostics());
        if (!success) {
            ci->getDiagnosticClient().finish();
            return 1;
        }

        success =
            compiles_cxx_source(*ci) ? execute_guarded(*ci) : ExecuteCompilerInvocation(ci.get());
        if (ci->getFrontendOpts().DisableFree) {
            llvm::BuryPointer(std::move(ci));
            return !success;
        }
        return !success;
    }

    // Adapt a Clang driver job to the in-process frontend entry point.
    int DriverFrontendRunner::execute_cc1_tool(llvm::SmallVectorImpl<const char *> &args)
    {
        return run_cc1(llvm::ArrayRef(args).slice(2), args[0]);
    }

    // Supply rewritten source for one frontend input file.
    void DriverFrontendRunner::set_rewritten_input(std::string input, std::string source)
    {
        rewritten_inputs[std::move(input)] = std::move(source);
    }

    // Report whether any input has been rewritten.
    bool DriverFrontendRunner::has_rewritten_inputs()
    {
        return !rewritten_inputs.empty();
    }

    // Identify frontend invocations that compile one unpreprocessed C++ source.
    bool DriverFrontendRunner::compiles_cxx_source(const CompilerInstance &ci)
    {
        const FrontendOptions &fo = ci.getFrontendOpts();
        if (fo.Inputs.size() != 1 || fo.ShowHelp || fo.ShowVersion) {
            return false;
        }
        InputKind ik = fo.Inputs[0].getKind();
        return ik.getLanguage() == Language::CXX && ik.getFormat() == InputKind::Source &&
               !ik.isPreprocessed();
    }

    // Execute a frontend invocation with header annotation checks.
    bool DriverFrontendRunner::execute_guarded(CompilerInstance &ci)
    {
        ci.LoadRequestedPlugins();
        const std::vector<std::string> &llvmArgs = ci.getFrontendOpts().LLVMArgs;
        if (!llvmArgs.empty()) {
            std::vector<const char *> args{"clang (LLVM option parsing)"};
            for (const std::string &arg : llvmArgs) {
                args.push_back(arg.c_str());
            }
            args.push_back(nullptr);
            llvm::cl::ParseCommandLineOptions(args.size() - 1, args.data());
        }
        if (ci.getDiagnostics().hasErrorOccurred()) {
            return false;
        }

        std::unique_ptr<FrontendAction> action = CreateFrontendAction(ci);
        if (!action) {
            return false;
        }
        if (!action->usesPreprocessorOnly()) {
            ci.getPreprocessorOpts().addMacroDef("__llm__=");
            action = std::make_unique<FrontendGuardAction>(std::move(action));
        }
        bool success = ci.ExecuteAction(*action);
        if (ci.getFrontendOpts().DisableFree) {
            llvm::BuryPointer(std::move(action));
        }
        return success;
    }

    // Anchor Clang resource-path discovery to this binary.
    void DriverFrontendRunner::anchor() {}

}
