/*
 * C++ header for the abstract handler for compiler-context tools.
 */

#ifndef LLMCPP_AGENT_TOOL_HANDLER_H
#define LLMCPP_AGENT_TOOL_HANDLER_H

#include "llmcpp/data/data_tool_result.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/JSON.h"
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Handles compiler-context tool calls made by an LLM agent. */
    class AgentToolHandler
    {
    public:
        /** Enable safe destruction through a base pointer. */
        virtual ~AgentToolHandler() = default;
        /**
         * Execute a named compiler-context tool.
         *
         * @param name Tool name from the MCP request.
         * @param arguments JSON arguments supplied by the agent.
         * @return Tool response and error state.
         */
        virtual data::DataToolResult call_tool(llvm::StringRef name,
                                               const llvm::json::Object &arguments) = 0;
    };

}

#endif
