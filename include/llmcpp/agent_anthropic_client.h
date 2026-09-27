/*
 * C++ header for direct Anthropic Messages API generation.
 */

#ifndef LLMCPP_AGENT_ANTHROPIC_CLIENT_H
#define LLMCPP_AGENT_ANTHROPIC_CLIENT_H

#include "llmcpp/agent_tool_handler.h"
#include "llmcpp/data/data_agent_outcome.h"
#include "llmcpp/data/data_generation_options.h"

#include "llvm/Support/JSON.h"

#include <string>
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /**
     * Decide whether the built-in Anthropic client should handle generation.
     *
     * Explicit external-agent overrides always take precedence.
     *
     * @param opts Driver options that may select an external agent.
     * @return True when generation should use the native API client.
     */
    bool use_native_anthropic(const data::DataGenerationOptions &opts);
    /**
     * Run one generation loop directly against the Anthropic Messages API.
     *
     * @param opts Generation limits and diagnostics settings.
     * @param task Generation task for one annotated function.
     * @param tools Compiler-context tool handler.
     * @param result Destination for the generation outcome.
     * @param error Destination for transport and protocol diagnostics.
     * @return True when the API exchange completed at the protocol level.
     */
    bool generate_anthropic(const data::DataGenerationOptions &opts, llvm::json::Object task,
                            AgentToolHandler &tools, data::DataAgentOutcome &result,
                            std::string &error);

}

#endif
