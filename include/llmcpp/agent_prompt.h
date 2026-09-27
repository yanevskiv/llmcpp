/*
 * C++ header for prompts shared by native LLM API clients.
 */

#ifndef LLMCPP_AGENT_PROMPT_H
#define LLMCPP_AGENT_PROMPT_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/JSON.h"

#include <string>
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Get the system instructions for compiler-driven generation. @return Prompt text. */
    llvm::StringRef agent_system_prompt();
    /**
     * Format the user message that starts a generation exchange.
     *
     * @param task Structured generation task.
     * @return User prompt for the target function.
     */
    std::string agent_task_message(const llvm::json::Object &task);
}

#endif
