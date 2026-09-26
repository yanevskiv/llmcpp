/*
 * C++ file for shared translation-unit state and shadow-source construction.
 */

// Project headers for shared pass state and source rewriting.
#include "llmcpp/generation/pass_state.h"

#include "llmcpp/compiler/shadow_compiler.h"
#include "llmcpp/data/edit.h"
#include "llmcpp/source/text.h"

// Type alias for lightweight LLVM string views.
using llvm::StringRef;

// Namespace for translation-unit state implementation.
namespace llmcpp
{
    // Namespace for llmcpp generation implementation.
    namespace generation
    {
        // Bind state to its active compiler invocation.
        PassState::PassState(::clang::CompilerInstance &ci, const data::Options &opts)
            : m_ci(ci)
            , m_opts(opts)
        {
            // Empty.
        }

        // Release owned shadow-compilation resources.
        PassState::~PassState() = default;

        // Build source for compiling one candidate in its original context.
        std::string PassState::shadow_source(const data::Target &current, StringRef candidate,
                                             unsigned &begin, unsigned &end) const
        {
            std::vector<data::Edit> edits;
            for (const data::Target &other : m_targets) {
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
            std::string out = source::apply_edits(m_source, edits);
            for (const data::Edit &e : edits) {
                if (e.m_begin == current.m_l_brace) {
                    begin = e.m_new_begin + 2;
                    end = begin + candidate.size();
                }
            }
            return out;
        }

    }
}
