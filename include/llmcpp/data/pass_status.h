/*
 * C++ header for terminal states of the __llm__ pass.
 */

#ifndef LLMCPP_DATA_PASS_STATUS_H
#define LLMCPP_DATA_PASS_STATUS_H
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Describes the result category of an llmcpp translation-unit pass. */
        enum class PassStatus {
            Unchanged,
            Rewritten,
            Failed,
            DumpedContext,
        };

    }
}

#endif
