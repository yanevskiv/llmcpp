/*
 * C++ header for state shared while processing one translation unit.
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

#ifndef LLMCPP_GENERATION_CONTEXT_H
#define LLMCPP_GENERATION_CONTEXT_H

#include "llmcpp/data/data_generation_options.h"
#include "llmcpp/data/data_generation_target.h"
#include "llmcpp/data/data_include_directive.h"

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
    class CompilerSandbox;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Owns state shared by generation and tools for one translation unit. */
    struct GenerationContext
    {
        /**
         * Create translation-unit state.
         *
         * @param ci Active compiler instance.
         * @param opts Immutable driver and generation options.
         */
        GenerationContext(::clang::CompilerInstance &ci, const data::DataGenerationOptions &opts);
        /** Destroy owned shadow-compilation state. */
        ~GenerationContext();
        /**
         * Build shadow source with one candidate body installed.
         *
         * @param current data::DataGenerationTarget receiving the candidate.
         * @param candidate Candidate body statements.
         * @param begin Destination for the candidate's starting offset.
         * @param end Destination for the candidate's ending offset.
         * @return Complete source for isolated compilation.
         */
        std::string shadow_source(const data::DataGenerationTarget &current,
                                  llvm::StringRef candidate, unsigned &begin, unsigned &end) const;
        /** Active compiler instance. */
        ::clang::CompilerInstance &m_ci;
        /** Immutable driver and generation options. */
        const data::DataGenerationOptions &m_opts;
        /** Main source file as spelled by the frontend invocation. */
        std::string m_main_file;
        /** Original main-source contents. */
        llvm::StringRef m_source;
        /** Annotation targets collected from the translation unit. */
        std::vector<data::DataGenerationTarget> m_targets;
        /** Include directives observed during preprocessing. */
        std::vector<data::DataIncludeDirective> m_includes;
        /** Reusable compiler for isolated validation and type probes. */
        std::unique_ptr<CompilerSandbox> m_shadow;
    };

}

#endif
