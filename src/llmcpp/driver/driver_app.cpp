/*
 * C++ file for the llmc++ driver application.
 */

// Project headers for driver orchestration and generation.
#include "llmcpp/driver/driver_app.h"
#include "llmcpp/driver/driver_options.h"
#include "llmcpp/driver/frontend_runner.h"
#include "llmcpp/generation/llm_pass.h"

// Clang headers for driver jobs and diagnostics.
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/InputInfo.h"
#include "clang/Driver/Job.h"
#include "clang/Driver/Types.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

// LLVM headers for driver services and filesystem access.
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/VirtualFileSystem.h"
#include "llvm/TargetParser/Host.h"

// Standard library header for optional values.
#include <optional>

// Namespace imports for Clang driver types.
using namespace clang;
// Namespace import for Clang driver types.
using namespace ::clang::driver;
// Type alias for lightweight LLVM string views.
using llvm::StringRef;

// Namespace for llmc++ driver implementation details.
namespace
{

    // Find the C++ source consumed by a frontend job.
    std::optional<std::string> cxx_input_of(const Command &job)
    {
        const auto &args = job.getArguments();
        if (args.empty() || StringRef(args[0]) != "-cc1") {
            return std::nullopt;
        }
        if (job.getInputInfos().size() != 1) {
            return std::nullopt;
        }
        const InputInfo &input = job.getInputInfos()[0];
        if (!input.isFilename() || input.getType() != types::TY_CXX) {
            return std::nullopt;
        }
        return std::string(input.getFilename());
    }

    // Report whether a frontend job only preprocesses its input.
    bool is_preprocess_job(const Command &job)
    {
        for (const char *arg : job.getArguments()) {
            if (StringRef(arg) == "-E") {
                return true;
            }
        }
        return false;
    }

    // Read an entire file when it exists.
    std::optional<std::string> read_file(StringRef path)
    {
        auto buffer = llvm::MemoryBuffer::getFile(path, false, false);
        if (!buffer) {
            return std::nullopt;
        }
        return (*buffer)->getBuffer().str();
    }

    // Derive the generated source path beside its original input.
    std::string default_output_path(StringRef input, bool preprocessed)
    {
        llvm::SmallString<256> path(input);
        std::string extension = llvm::sys::path::extension(input).str();
        llvm::sys::path::replace_extension(path, "");
        path += preprocessed ? ".llm.ii" : ".llm" + (extension.empty() ? ".cpp" : extension);
        return std::string(path);
    }

    // Write generated source only when its contents changed.
    bool write_if_changed(StringRef path, StringRef contents, DiagnosticsEngine &diags)
    {
        if (auto old = read_file(path); old && *old == contents) {
            return true;
        }
        std::error_code error;
        llvm::raw_fd_ostream stream(path, error, llvm::sys::fs::OF_None);
        if (error) {
            diags.Report(
                diags.getCustomDiagID(DiagnosticsEngine::Error, "cannot open output file '%0': %1"))
                << path << error.message();
            return false;
        }
        stream << contents;
        return true;
    }

}

// Namespace for llmc++ driver application behavior.
namespace llmcpp
{
    // Namespace for llmcpp driver implementation.
    namespace driver
    {

        // Bind the driver application to process arguments.
        DriverApp::DriverApp(int argc, char **argv)
            : m_argc(argc)
            , m_argv(argv)
        {
            // Empty.
        }

        // Run the llmc++ driver.
        int DriverApp::run()
        {
            llvm::SmallVector<const char *, 64> args(m_argv, m_argv + m_argc);
            if (args.size() >= 2 && StringRef(args[1]) == "-cc1") {
                return FrontendRunner::run_cc1(llvm::ArrayRef(args).slice(2), args[0]);
            }

            std::string executable = executable_path();
            DriverOptions options(executable);
            std::string optionError;
            bool validOptions = options.parse(llvm::ArrayRef(args).slice(1), optionError);

            llvm::SmallString<256> resourceDir(
                llvm::sys::path::parent_path(llvm::sys::path::parent_path(executable)));
            llvm::sys::path::append(resourceDir, "lib", "clang", LLMCPP_LLVM_VERSION);
            std::vector<const char *> driverArgs =
                options.driver_arguments(args[0], llvm::StringRef(resourceDir.c_str()));

            IntrusiveRefCntPtr<DiagnosticOptions> diagOpts = CreateAndPopulateDiagOpts(driverArgs);
            auto *diagClient = new TextDiagnosticPrinter(llvm::errs(), &*diagOpts);
            diagClient->setPrefix(llvm::sys::path::filename(executable).str());
            IntrusiveRefCntPtr<DiagnosticIDs> diagId(new DiagnosticIDs());
            DiagnosticsEngine diags(diagId, diagOpts, diagClient);
            ProcessWarningOptions(diags, *diagOpts, false);

            if (!validOptions) {
                diags.Report(diags.getCustomDiagID(DiagnosticsEngine::Error, "%0")) << optionError;
                return 1;
            }

            Driver driver(executable, llvm::sys::getDefaultTargetTriple(), diags,
                          "llmc++ (clang with __llm__ support)");
            driver.CC1Main = FrontendRunner::execute_cc1_tool;
            llvm::CrashRecoveryContext::Enable();

            std::unique_ptr<Compilation> compilation(driver.BuildCompilation(driverArgs));
            if (!compilation || compilation->containsError()) {
                return 1;
            }

            if (options.options().m_emit_source) {
                std::vector<const Command *> jobs;
                for (const Command &job : compilation->getJobs()) {
                    if (cxx_input_of(job)) {
                        jobs.push_back(&job);
                    }
                }
                if (!options.output_path().empty() && jobs.size() > 1) {
                    diags.Report(diags.getCustomDiagID(
                        DiagnosticsEngine::Error,
                        "cannot specify -o when generating multiple output files"));
                    return 1;
                }
                if (jobs.empty() && !diags.hasErrorOccurred()) {
                    diags.Report(diags.getCustomDiagID(DiagnosticsEngine::Error,
                                                       "--llm: no C++ input files"));
                    return 1;
                }

                int result = 0;
                for (const Command *job : jobs) {
                    std::string input = *cxx_input_of(*job);
                    std::optional<std::string> source = read_file(input);
                    if (!source) {
                        diags.Report(diags.getCustomDiagID(DiagnosticsEngine::Error,
                                                           "no such file or directory: '%0'"))
                            << input;
                        result = 1;
                        continue;
                    }

                    llvm::SmallVector<const char *, 128> cc1Args(job->getArguments().begin() + 1,
                                                                 job->getArguments().end());
                    std::string output = *source;
                    data::PassResult passResult =
                        generation::run_llm_pass(cc1Args, options.options());
                    if (passResult.m_status == data::PassStatus::Failed) {
                        result = 1;
                        continue;
                    }
                    if (passResult.m_status == data::PassStatus::Rewritten) {
                        output = std::move(passResult.m_output);
                    }
                    if (options.options().m_dump_context) {
                        continue;
                    }

                    std::string outputPath =
                        options.output_path().empty()
                            ? default_output_path(input, options.wants_preprocess())
                            : options.output_path();
                    if (!options.wants_preprocess()) {
                        if (!write_if_changed(outputPath, output, diags)) {
                            result = 1;
                        }
                        continue;
                    }

                    FrontendRunner::set_rewritten_input(input, std::move(output));
                    llvm::SmallVector<const char *, 128> preprocessArgs(cc1Args.begin(),
                                                                        cc1Args.end());
                    preprocessArgs.push_back("-E");
                    preprocessArgs.push_back("-o");
                    preprocessArgs.push_back(outputPath.c_str());
                    if (FrontendRunner::run_cc1(preprocessArgs, executable.c_str()) != 0) {
                        result = 1;
                    }
                }
                return result;
            }

            bool failed = false;
            for (const Command &job : compilation->getJobs()) {
                std::optional<std::string> input = cxx_input_of(job);
                if (!input || is_preprocess_job(job)) {
                    continue;
                }
                std::optional<std::string> source = read_file(*input);
                if (!source || !StringRef(*source).contains("__llm__")) {
                    continue;
                }
                llvm::SmallVector<const char *, 128> cc1Args(job.getArguments().begin() + 1,
                                                             job.getArguments().end());
                data::PassResult passResult = generation::run_llm_pass(cc1Args, options.options());
                if (passResult.m_status == data::PassStatus::Failed) {
                    failed = true;
                } else if (passResult.m_status == data::PassStatus::Rewritten) {
                    FrontendRunner::set_rewritten_input(*input, std::move(passResult.m_output));
                }
            }
            if (failed) {
                return 1;
            }
            if (options.options().m_dump_context) {
                return 0;
            }

            if (FrontendRunner::has_rewritten_inputs()) {
                for (Command &job : compilation->getJobs()) {
                    if (!job.getArguments().empty() && StringRef(job.getArguments()[0]) == "-cc1") {
                        job.InProcess = true;
                    }
                }
            }

            llvm::SmallVector<std::pair<int, const Command *>, 4> failingCommands;
            int result = driver.ExecuteCompilation(*compilation, failingCommands);
            if (!failingCommands.empty() && result == 0) {
                result = 1;
            }
            return result == 0 ? 0 : 1;
        }

        // Resolve the absolute path of the running executable.
        std::string DriverApp::executable_path() const
        {
            return llvm::sys::fs::getMainExecutable(m_argv[0], (void *)(intptr_t)anchor);
        }

        // Anchor executable-path discovery to this binary.
        void DriverApp::anchor() {}

    }
}
