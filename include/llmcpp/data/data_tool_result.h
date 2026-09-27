/*
 * C++ header for a compiler-context tool response.
 */

#ifndef LLMCPP_DATA_TOOL_RESULT_H
#define LLMCPP_DATA_TOOL_RESULT_H

#include <string>
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Contains the text and error state returned by a compiler-context tool. */
        struct DataToolResult
        {
            /** Textual response returned to the agent. */
            std::string m_text;
            /** Whether the response represents a tool error. */
            bool m_is_error = false;
        };

    }
}

#endif
