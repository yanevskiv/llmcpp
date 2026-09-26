/*
 * C++ header for state shared while processing one translation unit.
 */

#ifndef LLMCPP_PASS_STATE_H
#define LLMCPP_PASS_STATE_H

#include "llmcpp/data/inclusion.h"
#include "llmcpp/data/options.h"
#include "llmcpp/data/target.h"

#include "llvm/ADT/StringRef.h"

#include <memory>
#include <string>
#include <vector>
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class CompilerInstance;
}
/** Namespace for llmcpp Clang integration declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp compiler declarations. */
    namespace compiler
    {
        class ShadowCompiler;
    }
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp generation declarations. */
    namespace generation
    {
        /** Owns state shared by generation and tools for one translation unit. */
        struct PassState
        {
            /**
             * Create translation-unit state.
             *
             * @param ci Active compiler instance.
             * @param opts Immutable driver and generation options.
             */
            PassState(::clang::CompilerInstance &ci, const data::Options &opts);
            /** Destroy owned shadow-compilation state. */
            ~PassState();
            /**
             * Build shadow source with one candidate body installed.
             *
             * @param current data::Target receiving the candidate.
             * @param candidate Candidate body statements.
             * @param begin Destination for the candidate's starting offset.
             * @param end Destination for the candidate's ending offset.
             * @return Complete source for isolated compilation.
             */
            std::string shadow_source(const data::Target &current, llvm::StringRef candidate,
                                      unsigned &begin, unsigned &end) const;
            /** Active compiler instance. */
            ::clang::CompilerInstance &m_ci;
            /** Immutable driver and generation options. */
            const data::Options &m_opts;
            /** Main source file as spelled by the frontend invocation. */
            std::string m_main_file;
            /** Original main-source contents. */
            llvm::StringRef m_source;
            /** Annotation targets collected from the translation unit. */
            std::vector<data::Target> m_targets;
            /** Include directives observed during preprocessing. */
            std::vector<data::Inclusion> m_includes;
            /** Reusable compiler for isolated validation and type probes. */
            std::unique_ptr<compiler::ShadowCompiler> m_shadow;
        };

    }
}

#endif
