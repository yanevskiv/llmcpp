/*
 * C++ file for shared translation-unit state and shadow-source
 * construction.
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

// Project headers for shared pass state and source rewriting.
#include "llmcpp/generation_context.h"

#include "llmcpp/compiler_sandbox.h"
#include "llmcpp/data/data_source_edit.h"
#include "llmcpp/source_text.h"

// Type alias for lightweight LLVM string views.
using llvm::StringRef;

// Namespace for translation-unit state implementation.
namespace llmcpp
{
    // Bind state to its active compiler invocation.
    GenerationContext::GenerationContext(::clang::CompilerInstance &ci,
                                         const data::DataGenerationOptions &opts)
        : m_ci(ci)
        , m_opts(opts)
    {
        // Empty.
    }

    // Release owned shadow-compilation resources.
    GenerationContext::~GenerationContext() = default;

    // Build source for compiling one candidate in its original context.
    std::string GenerationContext::shadow_source(const data::DataGenerationTarget &current,
                                                 StringRef candidate, unsigned &begin,
                                                 unsigned &end) const
    {
        std::vector<data::DataSourceEdit> edits;
        for (const data::DataGenerationTarget &other : m_targets) {
            edits.push_back({other.m_keyword_offset, other.m_keyword_end, ""});
            if (&other == &current) {
                edits.push_back(
                    {other.m_l_brace, other.m_r_brace + 1, "{\n" + candidate.str() + "\n}"});
            } else if (other.m_generated) {
                edits.push_back(
                    {other.m_l_brace, other.m_r_brace + 1, "{\n" + other.m_code + "\n}"});
            } else {
                edits.push_back({other.m_l_brace, other.m_r_brace + 1, "{}"});
            }
        }
        std::string out = apply_edits(m_source, edits);
        for (const data::DataSourceEdit &e : edits) {
            if (e.m_begin == current.m_l_brace) {
                begin = e.m_new_begin + 2;
                end = begin + candidate.size();
            }
        }
        return out;
    }

}
