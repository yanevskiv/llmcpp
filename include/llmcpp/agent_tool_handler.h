/*
 * C++ header for the abstract handler for compiler-context tools.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * llmc++  is  free  software;  you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free  Software  Foundation;  either  version 3 of the License, or (at
 * your option) any later version.
 *
 * llmc++ is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS  FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You  should  have  received  a copy of the GNU General Public License
 * along with llmc++; if not, see <https://www.gnu.org/licenses/>.
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
