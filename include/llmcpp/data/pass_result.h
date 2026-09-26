/*
 * C++ header for the result produced by the __llm__ pass.
 */

#ifndef LLMCPP_DATA_PASS_RESULT_H
#define LLMCPP_DATA_PASS_RESULT_H

#include "llmcpp/data/pass_status.h"

#include <string>
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Contains the status and optional rewritten source produced by the pass. */
        struct PassResult
        {
            /** Final status of the compilation pass. */
            PassStatus m_status = PassStatus::Unchanged;
            /** Rewritten source emitted after successful generation. */
            std::string m_output;
        };

    }
}

#endif
