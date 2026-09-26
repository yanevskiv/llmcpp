/*
 * C++ header for the abstract handler for compiler-context tools.
 */

#ifndef LLMCPP_TOOL_HANDLER_H
#define LLMCPP_TOOL_HANDLER_H

#include "llmcpp/data/tool_result.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/JSON.h"
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp agent declarations. */
    namespace agent
    {
        /** Handles compiler-context tool calls made by an LLM agent. */
        class ToolHandler
        {
        public:
            /** Enable safe destruction through a base pointer. */
            virtual ~ToolHandler() = default;
            /**
             * Execute a named compiler-context tool.
             *
             * @param name Tool name from the MCP request.
             * @param arguments JSON arguments supplied by the agent.
             * @return Tool response and error state.
             */
            virtual data::ToolResult call_tool(llvm::StringRef name,
                                               const llvm::json::Object &arguments) = 0;
        };

    }
}

#endif
