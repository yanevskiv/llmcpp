/*
 * C++ header for collecting llm annotations and source inclusions.
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

#ifndef LLMCPP_FRONTEND_SOURCE_COLLECTOR_H
#define LLMCPP_FRONTEND_SOURCE_COLLECTOR_H

#include "llmcpp/data/data_include_directive.h"

#include "clang/Lex/PPCallbacks.h"

#include <vector>
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class SourceManager;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Collects llm annotation locations and resolved include directives. */
    class FrontendSourceCollector : public ::clang::PPCallbacks
    {
    public:
        /**
         * Bind collection outputs to the active source manager.
         *
         * @param sourceManager Active source manager.
         * @param keywords Output annotation locations.
         * @param includes Output include records.
         */
        FrontendSourceCollector(::clang::SourceManager &sourceManager,
                                std::vector<::clang::SourceLocation> &keywords,
                                std::vector<data::DataIncludeDirective> &includes);
        /**
         * Record an annotation macro expansion.
         *
         * @param name Expanded macro name.
         * @param definition Macro definition information.
         * @param range Expansion source range.
         * @param args Macro arguments when present.
         */
        void MacroExpands(const ::clang::Token &name, const ::clang::MacroDefinition &definition,
                          ::clang::SourceRange range, const ::clang::MacroArgs *args) override;
        /**
         * Record an include directive and resolved file path.
         *
         * @param hashLoc Hash-token location.
         * @param includeToken Include directive token.
         * @param fileName Spelled include filename.
         * @param isAngled Whether the include uses angle brackets.
         * @param filenameRange Filename source range.
         * @param file Resolved included file.
         * @param searchPath Include search path.
         * @param relativePath Relative include path.
         * @param imported Module imported through the include.
         * @param moduleImported Whether a module was imported.
         * @param fileType Characteristic of the included file.
         */
        void InclusionDirective(::clang::SourceLocation hashLoc, const ::clang::Token &includeToken,
                                llvm::StringRef fileName, bool isAngled,
                                ::clang::CharSourceRange filenameRange,
                                ::clang::OptionalFileEntryRef file, llvm::StringRef searchPath,
                                llvm::StringRef relativePath, const ::clang::Module *imported,
                                bool moduleImported,
                                ::clang::SrcMgr::CharacteristicKind fileType) override;

    private:
        /** Source manager used to classify include locations. */
        ::clang::SourceManager &m_source_manager;
        /** Destination for observed annotation locations. */
        std::vector<::clang::SourceLocation> &m_keywords;
        /** Destination for observed include records. */
        std::vector<data::DataIncludeDirective> &m_includes;
    };

}

#endif
