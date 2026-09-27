/*
 * C++ file for shared translation-unit state and shadow-source construction.
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
