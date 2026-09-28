/*
 * C++ header for filtering diagnostics from generated source regions.
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

#ifndef LLMCPP_COMPILER_DIAGNOSTIC_COLLECTOR_H
#define LLMCPP_COMPILER_DIAGNOSTIC_COLLECTOR_H

#include "clang/Basic/Diagnostic.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Collects diagnostics relevant to a generated source region. */
    class CompilerDiagnosticCollector : public ::clang::DiagnosticConsumer
    {
    public:
        /**
         * Configure the generated region and optional source label.
         *
         * @param begin First offset of the generated region.
         * @param end Offset just after the generated region.
         * @param label Optional source label for diagnostics outside the region.
         */
        CompilerDiagnosticCollector(unsigned begin, unsigned end, llvm::StringRef label);
        /**
         * Format diagnostics while filtering unrelated warnings and notes.
         *
         * @param level Diagnostic severity.
         * @param info Diagnostic details.
         */
        void HandleDiagnostic(::clang::DiagnosticsEngine::Level level,
                              const ::clang::Diagnostic &info) override;
        /** Number of retained error diagnostics. */
        unsigned m_errors = 0;
        /** Number of retained warning diagnostics. */
        unsigned m_warnings = 0;
        /** Accumulated formatted diagnostic text. */
        std::string m_text;
        /** Stream writing directly into formatted diagnostic text. */
        llvm::raw_string_ostream m_os{m_text};

    private:
        /** First offset of the generated source region. */
        unsigned m_begin;
        /** Offset just after the generated source region. */
        unsigned m_end;
        /** Optional label for diagnostics outside the generated region. */
        llvm::StringRef m_label;
        /** Whether the preceding retained diagnostic was not a note. */
        bool m_last_kept = false;
    };

}

#endif
