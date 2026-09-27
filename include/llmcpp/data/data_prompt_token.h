/*
 * C++ header for lexical token ranges used while finding prompt bodies.
 */

#ifndef LLMCPP_DATA_PROMPT_TOKEN_H
#define LLMCPP_DATA_PROMPT_TOKEN_H
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Stores a token range relevant to prompt-body discovery. */
        struct DataPromptToken
        {
            /** First source offset covered by the token. */
            unsigned m_begin;
            /** Offset just after the token. */
            unsigned m_end;
            /** Whether the token occurs in a preprocessor directive. */
            bool m_directive;
        };

    }
}

#endif
