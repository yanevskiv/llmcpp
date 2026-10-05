/*
 * C++ header for prompts shared by native LLM API clients.
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

#ifndef LLMCPP_AGENT_PROMPT_H
#define LLMCPP_AGENT_PROMPT_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/JSON.h"

#include "llmcpp/data/data_generation_options.h"
#include <string>
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Get the system instructions for compiler-driven generation. @return Prompt text. */
    llvm::StringRef agent_system_prompt();
    /**
     * Describe effective generation policy for an agent and cache key.
     * @param opts Resolved target options.
     * @return Structured model, cache, and limit settings.
     */
    llvm::json::Object agent_generation_settings(const data::DataGenerationOptions &opts);
    /**
     * Describe the selected agent without exposing credentials.
     * @param opts Resolved target options.
     * @return Agent command or backend identity.
     */
    std::string agent_identity(const data::DataGenerationOptions &opts);
    /**
     * Append a diagnostic event with credentials redacted.
     * @param opts Transcript destination options.
     * @param event Event kind.
     * @param value Structured event data.
     */
    void agent_record(const data::DataGenerationOptions &opts, llvm::StringRef event,
                      llvm::json::Value value);
    /**
     * Format the user message that starts a generation exchange.
     *
     * @param task Structured generation task.
     * @return User prompt for the target function.
     */
    std::string agent_task_message(const llvm::json::Object &task);
}

#endif
