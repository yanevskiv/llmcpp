/*
 * C++ header for a source-text replacement.
 */

#ifndef LLMCPP_DATA_SOURCE_EDIT_H
#define LLMCPP_DATA_SOURCE_EDIT_H

#include <string>
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Replaces a half-open source range and records its resulting offset. */
        struct DataSourceEdit
        {
            /** First replaced source offset. */
            unsigned m_begin;
            /** Offset just after the replaced source. */
            unsigned m_end;
            /** Replacement text. */
            std::string m_text;
            /** Offset of this edit after prior edits are applied. */
            unsigned m_new_begin = 0;
        };

    }
}

#endif
