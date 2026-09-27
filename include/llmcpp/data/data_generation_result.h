/*
 * C++ header for the result produced by the __llm__ pass.
 */

#ifndef LLMCPP_DATA_GENERATION_RESULT_H
#define LLMCPP_DATA_GENERATION_RESULT_H

#include "llmcpp/data/data_generation_status.h"

#include <string>
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Contains the status and optional rewritten source produced by the pass. */
        struct DataGenerationResult
        {
            /** Final status of the compilation pass. */
            DataGenerationStatus m_status = DataGenerationStatus::Unchanged;
            /** Rewritten source emitted after successful generation. */
            std::string m_output;
        };

    }
}

#endif
