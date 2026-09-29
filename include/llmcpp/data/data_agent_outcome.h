/*
 * C++ header for the outcome of one agent generation request.
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
        struct DataAgentOutcome
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
