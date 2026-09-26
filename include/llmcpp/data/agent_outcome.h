/*
 * C++ header for the outcome of one agent generation request.
 */

#ifndef LLMCPP_DATA_AGENT_OUTCOME_H
#define LLMCPP_DATA_AGENT_OUTCOME_H

#include <string>
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Describes the terminal status and usage reported by an agent generation. */
        struct AgentOutcome
        {
            /** Completion status reported by the agent. */
            std::string m_status;
            /** Human-readable completion message. */
            std::string m_message;
            /** Model identifier reported by the agent. */
            std::string m_model;
            /** Number of tool calls serviced during generation. */
            unsigned m_tool_calls = 0;
        };

    }
}

#endif
