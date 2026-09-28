/*
 * C++ file for filtering diagnostics from generated source regions.
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

// Project header for generated-region diagnostic collection.
#include "llmcpp/compiler_diagnostic_collector.h"

// Clang headers for source location formatting.
#include "clang/Basic/SourceManager.h"

// LLVM headers for source path formatting.
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Path.h"

// Namespace for generated-region diagnostic collection.
namespace llmcpp
{
    // Configure the generated region and optional source label.
    CompilerDiagnosticCollector::CompilerDiagnosticCollector(unsigned begin, unsigned end,
                                                             llvm::StringRef label)
        : m_begin(begin)
        , m_end(end)
        , m_label(label)
    {
        // Empty.
    }

    // Format diagnostics while filtering unrelated warnings and notes.
    void CompilerDiagnosticCollector::HandleDiagnostic(::clang::DiagnosticsEngine::Level level,
                                                       const ::clang::Diagnostic &info)
    {
        ::clang::DiagnosticConsumer::HandleDiagnostic(level, info);
        if (level == ::clang::DiagnosticsEngine::Ignored ||
            level == ::clang::DiagnosticsEngine::Remark) {
            return;
        }
        bool isNote = level == ::clang::DiagnosticsEngine::Note;
        if (isNote && !m_last_kept) {
            return;
        }

        std::string where;
        std::string snippet;
        bool inRegion = false;
        if (info.hasSourceManager() && info.getLocation().isValid()) {
            ::clang::SourceManager &sourceManager = info.getSourceManager();
            ::clang::SourceLocation location = sourceManager.getFileLoc(info.getLocation());
            auto [fileId, offset] = sourceManager.getDecomposedLoc(location);
            if (fileId == sourceManager.getMainFileID()) {
                llvm::StringRef buffer = sourceManager.getBufferData(fileId);
                if (m_begin < m_end && offset >= m_begin && offset <= m_end) {
                    inRegion = true;
                    llvm::StringRef region = buffer.substr(m_begin, m_end - m_begin);
                    unsigned relative = offset - m_begin;
                    llvm::StringRef before = region.substr(0, relative);
                    size_t lineStart = before.rfind('\n');
                    lineStart = lineStart == llvm::StringRef::npos ? 0 : lineStart + 1;
                    unsigned line = 1 + before.count('\n');
                    unsigned column = relative - lineStart + 1;
                    llvm::StringRef lineText = region.substr(lineStart).split('\n').first;
                    where = "line " + std::to_string(line) + ":" + std::to_string(column);
                    snippet =
                        "    " + lineText.str() + "\n    " + std::string(column - 1, ' ') + "^\n";
                } else {
                    std::string line = std::to_string(sourceManager.getLineNumber(fileId, offset));
                    if (m_label.empty()) {
                        where = "outside the body (line " + line + " of the file)";
                    } else {
                        where = m_label.str() + ":" + line + ":" +
                                std::to_string(sourceManager.getColumnNumber(fileId, offset));
                    }
                }
            } else {
                ::clang::PresumedLoc presumed = sourceManager.getPresumedLoc(location);
                if (presumed.isValid()) {
                    where = llvm::sys::path::filename(presumed.getFilename()).str() + ":" +
                            std::to_string(presumed.getLine());
                }
            }
        }

        if (!isNote) {
            m_last_kept = level >= ::clang::DiagnosticsEngine::Error || inRegion;
            if (!m_last_kept) {
                return;
            }
            if (level >= ::clang::DiagnosticsEngine::Error) {
                ++m_errors;
            } else {
                ++m_warnings;
            }
        }

        llvm::SmallString<256> message;
        info.FormatDiagnostic(message);
        const char *kind = isNote                                         ? "note"
                           : level == ::clang::DiagnosticsEngine::Warning ? "warning"
                                                                          : "error";
        if (!where.empty()) {
            m_os << where << ": ";
        }
        m_os << kind << ": " << message << "\n" << snippet;
    }
}
