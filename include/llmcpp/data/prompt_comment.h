/*
 * C++ header for a legacy comment fragment used as prompt text.
 */

#ifndef LLMCPP_DATA_PROMPT_COMMENT_H
#define LLMCPP_DATA_PROMPT_COMMENT_H

#include "llvm/ADT/StringRef.h"
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Describes one legacy prompt comment and its preceding vertical spacing. */
        struct PromptComment
        {
            /** Raw text of one source comment. */
            llvm::StringRef m_text;
            /** Whether a blank source line precedes this comment. */
            bool m_blank_line_before = false;
        };

    }
}

#endif
