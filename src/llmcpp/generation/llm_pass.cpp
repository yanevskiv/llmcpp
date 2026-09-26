/*
 * C++ file for the public __llm__ pass entry point.
 */

// Project headers for pass execution and frontend actions.
#include "llmcpp/generation/llm_pass.h"
#include "llmcpp/frontend/pass_action.h"
#include "llmcpp/generation/pass.h"

// Clang headers for frontend invocation and compiler execution.
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/CodeGen/ObjectFilePCHContainerOperations.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/DependencyOutputOptions.h"
#include "clang/Lex/PreprocessorOptions.h"

// LLVM headers for source buffers.
#include "llvm/Support/MemoryBuffer.h"

// Standard library headers for ownership and argument storage.
#include <memory>
#include <string>
#include <vector>

// Namespace import for Clang frontend types.
using namespace clang;
// Type alias for lightweight LLVM string views.
using llvm::StringRef;

// Namespace for the public llmcpp compilation pass.
namespace llmcpp
{
    // Namespace for llmcpp generation implementation.
    namespace generation
    {
        // Create a parseable source copy with generation prompts erased.
        std::string make_parseable_source(StringRef source);

        // Run the llm pass against one source file and return rewritten source.
        data::PassResult run_llm_pass(llvm::ArrayRef<const char *> cc1Args,
                                      const data::Options &opts)
        {
            data::PassResult result;
            auto invocation = std::make_shared<CompilerInvocation>();
            {
                IgnoringDiagConsumer ignore;
                DiagnosticsEngine argumentDiagnostics(new DiagnosticIDs(), new DiagnosticOptions(),
                                                      &ignore, false);
                if (!CompilerInvocation::CreateFromArgs(*invocation, cc1Args, argumentDiagnostics,
                                                        opts.m_executable.c_str())) {
                    return result;
                }
            }

            FrontendOptions &frontendOptions = invocation->getFrontendOpts();
            if (frontendOptions.Inputs.size() != 1 || !frontendOptions.Inputs[0].isFile()) {
                return result;
            }
            std::string mainFile = frontendOptions.Inputs[0].getFile().str();
            auto mainBuffer = llvm::MemoryBuffer::getFile(mainFile, true);
            if (!mainBuffer) {
                return result;
            }
            std::string originalSource = (*mainBuffer)->getBuffer().str();
            std::string parseableSource = make_parseable_source(originalSource);
            frontendOptions.ProgramAction = ::clang::frontend::ParseSyntaxOnly;
            frontendOptions.OutputFile.clear();
            frontendOptions.DisableFree = false;
            invocation->getDependencyOutputOpts() = DependencyOutputOptions();
            invocation->getLangOpts().CommentOpts.ParseAllComments = true;
            invocation->getPreprocessorOpts().addMacroDef("__llm__=");
            invocation->getPreprocessorOpts().addRemappedFile(
                mainFile,
                llvm::MemoryBuffer::getMemBufferCopy(parseableSource, mainFile).release());
            invocation->getDiagnosticOpts().IgnoreWarnings = true;

            CompilerInstance compiler;
            compiler.getPCHContainerOperations()->registerWriter(
                std::make_unique<ObjectFilePCHContainerWriter>());
            compiler.getPCHContainerOperations()->registerReader(
                std::make_unique<ObjectFilePCHContainerReader>());
            compiler.setInvocation(std::move(invocation));
            compiler.createDiagnostics();

            Pass pass(compiler, opts, std::vector<std::string>(cc1Args.begin(), cc1Args.end()),
                      std::move(originalSource), result);
            pass.state().m_main_file = mainFile;
            frontend::PassAction action(pass);
            if (!compiler.ExecuteAction(action) || compiler.getDiagnostics().hasErrorOccurred()) {
                result.m_status = data::PassStatus::Failed;
            }
            return result;
        }
    }
}
