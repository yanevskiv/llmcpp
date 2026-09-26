/*
 * C++ header for filtering diagnostics from generated source regions.
 */

#ifndef LLMCPP_COLLECTOR_H
#define LLMCPP_COLLECTOR_H

#include "clang/Basic/Diagnostic.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp compiler declarations. */
    namespace compiler
    {
        /** Collects diagnostics relevant to a generated source region. */
        class Collector : public ::clang::DiagnosticConsumer
        {
        public:
            /**
             * Configure the generated region and optional source label.
             *
             * @param begin First offset of the generated region.
             * @param end Offset just after the generated region.
             * @param label Optional source label for diagnostics outside the region.
             */
            Collector(unsigned begin, unsigned end, llvm::StringRef label);
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
}

#endif
