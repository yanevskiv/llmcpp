/*
 * C++ header for in-process execution of Clang frontend jobs.
 */

#ifndef LLMCPP_FRONTEND_RUNNER_H
#define LLMCPP_FRONTEND_RUNNER_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include <string>
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class CompilerInstance;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp driver declarations. */
    namespace driver
    {
        /** Executes Clang frontend jobs against optionally rewritten in-memory inputs. */
        class FrontendRunner
        {
        public:
            /**
             * Run one Clang `-cc1` invocation.
             *
             * @param args Frontend arguments without the `-cc1` marker.
             * @param argv0 Executable path supplied to Clang.
             * @return Zero on success.
             */
            static int run_cc1(llvm::ArrayRef<const char *> args, const char *argv0);
            /**
             * Adapt a Clang driver job to the in-process frontend entry point.
             *
             * @param args Complete job argument vector.
             * @return Zero on success.
             */
            static int execute_cc1_tool(llvm::SmallVectorImpl<const char *> &args);
            /**
             * Supply rewritten source for one frontend input file.
             *
             * @param input Frontend input filename.
             * @param source Replacement source text.
             */
            static void set_rewritten_input(std::string input, std::string source);
            /**
             * Report whether any input has been rewritten.
             *
             * @return True when in-process frontend execution is required.
             */
            static bool has_rewritten_inputs();

        private:
            /**
             * Identify frontend invocations that compile one unpreprocessed C++ source.
             *
             * @param ci Active compiler instance.
             * @return True when annotation guards should be installed.
             */
            static bool compiles_cxx_source(const ::clang::CompilerInstance &ci);
            /**
             * Execute a frontend invocation with header annotation checks.
             *
             * @param ci Active compiler instance.
             * @return Whether Clang completed successfully.
             */
            static bool execute_guarded(::clang::CompilerInstance &ci);
            /** Anchor Clang resource-path discovery to this binary. */
            static void anchor();
        };

    }
}

#endif
