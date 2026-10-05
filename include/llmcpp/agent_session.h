/*
 * C++ header for native and out-of-process LLM generation sessions.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * llmc++ is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * llmc++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with llmc++; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef LLMCPP_AGENT_SESSION_H
#define LLMCPP_AGENT_SESSION_H

#include "llmcpp/agent_tool_handler.h"
#include "llmcpp/data/data_agent_outcome.h"
#include "llmcpp/data/data_generation_options.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/JSON.h"

#include <chrono>
#include <optional>
#include <string>
#include <sys/types.h>
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /**
     * Build MCP tool definitions exposed to an agent.
     *
     * @return JSON array of tool schemas.
     */
    llvm::json::Array tool_definitions();
    /** Selects native generation or owns an external agent connection. */
    class AgentSession
    {
    public:
        /**
         * Create a lazy agent session.
         *
         * @param opts Driver options controlling the agent process and limits.
         */
        explicit AgentSession(const data::DataGenerationOptions &opts);
        /** Close the connection and reap the agent process. */
        ~AgentSession();
        /**
         * Send a generation request and serve tool calls until completion.
         *
         * @param task Generation task sent to the agent.
         * @param tools Handler for compiler-context tool calls.
         * @param result Destination for the agent outcome.
         * @param error Destination for transport or protocol diagnostics.
         * @return True when the exchange completed at the protocol level.
         */
        bool generate(llvm::json::Object task, AgentToolHandler &tools,
                      data::DataAgentOutcome &result, std::string &error);
        /**
         * Get the model identifier reported during initialization.
         *
         * @return Model identifier, or an empty string before initialization.
         */
        const std::string &model() const
        {
            return m_model;
        }

    private:
        /**
         * Start the external agent and complete its MCP handshake.
         *
         * @param error Destination for startup diagnostics.
         * @return True when the agent is ready.
         */
        bool start(std::string &error);
        /**
         * Close and reap the agent process.
         *
         * @param shouldKill Whether to terminate the process before waiting.
         * @return Human-readable process exit information.
         */
        std::string stop(bool shouldKill);
        /**
         * Send one JSON-RPC message.
         *
         * @param message Message to serialize.
         * @param error Destination for transport diagnostics.
         * @return True when the complete message was written.
         */
        bool send(llvm::json::Value message, std::string &error);
        /**
         * Receive one JSON-RPC message before a deadline.
         *
         * @param deadline Absolute receive deadline.
         * @param error Destination for transport or parsing diagnostics.
         * @return Parsed message, or no value on failure.
         */
        std::optional<llvm::json::Value> receive(std::chrono::steady_clock::time_point deadline,
                                                 std::string &error);
        /**
         * Handle one request, notification, or response from the agent.
         *
         * @param message Incoming JSON-RPC object.
         * @param tools Optional tool handler for generation requests.
         * @param toolCalls Mutable count of serviced tool calls.
         * @param error Destination for protocol diagnostics.
         * @return True when processing may continue.
         */
        bool handle_incoming(const llvm::json::Object &message, AgentToolHandler *tools,
                             unsigned &toolCalls, std::string &error);
        /**
         * Resolve the configured agent command.
         *
         * @return Shell command used to start the agent.
         */
        std::string command() const;
        /** Immutable options controlling the agent process. */
        const data::DataGenerationOptions &m_opts;
        /** Connected external-agent socket, or -1 when disconnected. */
        int m_fd = -1;
        /** External-agent process identifier, or -1 before startup. */
        pid_t m_pid = -1;
        /** Whether the active request exceeded its deadline. */
        bool m_timed_out = false;
        /** Partial bytes buffered while receiving JSON-RPC frames. */
        std::string m_buffer;
        /** Model identifier reported by the agent. */
        std::string m_model;
        /** Next outgoing JSON-RPC request identifier. */
        unsigned m_next_id = 1;
    };

}

#endif
