/*
 * C++ header for isolated shadow compilation for validation and type
 * probes.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * llmc++  is  free  software;  you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free  Software  Foundation;  either  version 3 of the License, or (at
 * your option) any later version.
 *
 * llmc++ is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS  FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You  should  have  received  a copy of the GNU General Public License
 * along with llmc++; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LLMCPP_COMPILER_SANDBOX_H
#define LLMCPP_COMPILER_SANDBOX_H

#include "llmcpp/data/data_compilation_result.h"

#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/StringRef.h"

#include <string>
#include <vector>
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class ASTContext;
    class Sema;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Invokes semantic inspection after successful shadow compilation. */
    using InspectFn = llvm::function_ref<void(::clang::ASTContext &, ::clang::Sema &)>;
    /** Compiles modified copies of a translation unit without mutating live Sema state. */
    class CompilerSandbox
    {
    public:
        /**
         * Create a reusable shadow compiler.
         *
         * @param cc1Args Original Clang frontend arguments.
         * @param mainFile Main source file as spelled by the driver.
         * @param executable Absolute path to llmc++.
         */
        CompilerSandbox(std::vector<std::string> cc1Args, std::string mainFile,
                        std::string executable);
        /**
         * Compile replacement source and optionally inspect its AST.
         *
         * @param source Complete replacement source for the main file.
         * @param regionBegin Start of the diagnostic region.
         * @param regionEnd End of the diagnostic region.
         * @param extraWarnings Whether strict candidate warnings are enabled.
         * @param fileLabel Optional filename used in formatted diagnostics.
         * @param inspect Optional callback invoked after successful parsing.
         * @return Compilation status and formatted diagnostics.
         */
        data::DataCompilationResult compile(llvm::StringRef source, unsigned regionBegin,
                                            unsigned regionEnd, bool extraWarnings,
                                            llvm::StringRef fileLabel = "",
                                            InspectFn inspect = nullptr);

    private:
        /** Original Clang frontend arguments. */
        std::vector<std::string> m_args;
        /** Main source file as spelled by the driver. */
        std::string m_main_file;
        /** Absolute path to the llmc++ executable. */
        std::string m_executable;
    };

}

#endif
