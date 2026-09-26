/*
 * C++ header for diagnostics returned by a shadow compilation.
 */

#ifndef LLMCPP_DATA_SHADOW_RESULT_H
#define LLMCPP_DATA_SHADOW_RESULT_H

#include <string>
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Summarizes diagnostics from an isolated shadow compilation. */
        struct ShadowResult
        {
            /** Whether shadow compilation completed without errors. */
            bool m_ok = false;
            /** Number of filtered error diagnostics. */
            unsigned m_errors = 0;
            /** Number of filtered warning diagnostics. */
            unsigned m_warnings = 0;
            /** Formatted diagnostics from shadow compilation. */
            std::string m_text;
        };

    }
}

#endif
