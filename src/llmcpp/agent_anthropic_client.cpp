/*
 * C++ file for direct Anthropic Messages API generation.
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

// Project headers for native generation and compiler-context tools.
#include "llmcpp/agent_anthropic_client.h"
#include "llmcpp/agent_prompt.h"
#include "llmcpp/agent_session.h"

// LLVM headers for JSON serialization and diagnostics.
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

// Header-only HTTP client, built with OpenSSL support by CMake.
#include <httplib.h>

// Standard headers for environment configuration, limits, and retries.
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

// Namespace imports for LLVM JSON and string support.
using namespace llvm;
using Clock = std::chrono::steady_clock;

namespace
{

    // Return a nonempty environment setting, or an empty string.
    std::string environment(StringRef name)
    {
        const char *value = std::getenv(name.str().c_str());
        return value && *value ? value : "";
    }

    // Keep verbose tool diagnostics compact and single-line.
    std::string truncated(StringRef text, size_t maximum)
    {
        std::string line = text.split('\n').first.str();
        if (line.size() > maximum || line.size() < text.size()) {
            line = line.substr(0, maximum) + " ...";
        }
        return line;
    }

    // Split an API base URL into the origin accepted by httplib and a path prefix.
    bool split_base_url(StringRef url, std::string &origin, std::string &prefix, std::string &error)
    {
        size_t scheme = url.find("://");
        if (scheme == StringRef::npos ||
            (url.take_front(scheme) != "http" && url.take_front(scheme) != "https")) {
            error = "ANTHROPIC_BASE_URL must start with http:// or https://";
            return false;
        }
        size_t slash = url.find('/', scheme + 3);
        origin = url.take_front(slash).str();
        prefix = slash == StringRef::npos ? "" : url.drop_front(slash).rtrim('/').str();
        if (origin.size() == scheme + 3) {
            error = "ANTHROPIC_BASE_URL has no host";
            return false;
        }
        return true;
    }

    // Own the HTTP configuration and retry loop for one generation request.
    class AnthropicHttpClient
    {
    public:
        AnthropicHttpClient(std::string baseUrl, std::string apiKey, unsigned timeoutSeconds,
                            bool verbose, std::string &error)
            : m_api_key(std::move(apiKey))
            , m_deadline(Clock::now() + std::chrono::seconds(timeoutSeconds))
            , m_verbose(verbose)
        {
            std::string origin;
            if (!split_base_url(baseUrl, origin, m_prefix, error)) {
                return;
            }
            m_client = std::make_unique<httplib::Client>(origin);
            m_client->enable_server_certificate_verification(true);
            m_client->set_follow_location(false);
        }

        // Report whether URL parsing and client construction succeeded.
        explicit operator bool() const
        {
            return bool(m_client);
        }

        // Post one JSON request, retrying transient service and transport failures.
        bool post(const json::Object &request, json::Object &response, std::string &error)
        {
            std::string body = formatv("{0}", json::Value(json::Object(request))).str();
            httplib::Headers headers{{"x-api-key", m_api_key}, {"anthropic-version", "2023-06-01"}};

            for (unsigned attempt = 0; attempt != 6; ++attempt) {
                auto remaining =
                    std::chrono::duration_cast<std::chrono::seconds>(m_deadline - Clock::now());
                if (remaining <= std::chrono::seconds::zero()) {
                    error = "Anthropic API request timed out";
                    return false;
                }
                m_client->set_connection_timeout(remaining);
                m_client->set_read_timeout(remaining);
                m_client->set_write_timeout(remaining);

                httplib::Result result =
                    m_client->Post(m_prefix + "/v1/messages", headers, body, "application/json");
                if (result && result->status >= 200 && result->status < 300) {
                    Expected<json::Value> parsed = json::parse(result->body);
                    if (!parsed) {
                        error = "invalid JSON from Anthropic API: " + toString(parsed.takeError());
                        return false;
                    }
                    json::Object *object = parsed->getAsObject();
                    if (!object) {
                        error = "Anthropic API returned a non-object JSON response";
                        return false;
                    }
                    response = std::move(*object);
                    return true;
                }

                int status = result ? result->status : 0;
                bool transient = !result || status == 429 || status == 500 || status == 502 ||
                                 status == 503 || status == 504 || status == 529;
                if (transient && attempt != 5) {
                    unsigned delay = std::min(60U, 2U << attempt);
                    if (m_verbose) {
                        errs() << "llmc++: Anthropic "
                               << (result ? formatv("HTTP {0}", status).str()
                                          : httplib::to_string(result.error()))
                               << ", retrying in " << delay << "s\n";
                    }
                    auto wake = std::min(m_deadline, Clock::now() + std::chrono::seconds(delay));
                    std::this_thread::sleep_until(wake);
                    continue;
                }

                if (result) {
                    error = formatv("Anthropic API error {0}: {1}", status,
                                    StringRef(result->body).take_front(500))
                                .str();
                } else {
                    error = "cannot reach the Anthropic API: " + httplib::to_string(result.error());
                }
                return false;
            }
            error = "Anthropic API request failed";
            return false;
        }

    private:
        std::unique_ptr<httplib::Client> m_client;
        std::string m_api_key;
        std::string m_prefix;
        Clock::time_point m_deadline;
        bool m_verbose;
    };

    // Convert MCP-flavored schemas to the names expected by the Messages API.
    json::Value anthropic_tools()
    {
        json::Array definitions = llmcpp::tool_definitions();
        json::Array converted;
        for (json::Value &value : definitions) {
            json::Object *definition = value.getAsObject();
            if (!definition) {
                continue;
            }
            json::Object tool;
            tool["name"] = definition->getString("name").value_or("");
            tool["description"] = definition->getString("description").value_or("");
            if (json::Value *schema = definition->get("inputSchema")) {
                tool["input_schema"] = *schema;
            } else {
                tool["input_schema"] = json::Object{{"type", "object"}};
            }
            converted.emplace_back(std::move(tool));
        }
        return json::Value(std::move(converted));
    }

}

// Namespace for the native Anthropic session implementation.
namespace llmcpp
{

    // Select native Anthropic unless an external-agent override takes precedence.
    bool use_native_anthropic(const data::DataGenerationOptions &opts)
    {
        if (!opts.m_agent_command.empty() || !opts.m_agent_config_file.empty() ||
            !environment("LLMCPP_AGENT").empty()) {
            return false;
        }
        return opts.m_backend == "anthropic";
    }

    // Run the Messages API tool-use loop in the compiler process.
    bool generate_anthropic(const data::DataGenerationOptions &opts, json::Object task,
                            AgentToolHandler &tools, data::DataAgentOutcome &result,
                            std::string &error)
    {
        std::string apiKey = environment("ANTHROPIC_API_KEY");
        if (apiKey.empty()) {
            error = "LLMCPP_BACKEND=anthropic needs ANTHROPIC_API_KEY";
            return false;
        }
        std::string model = opts.m_model;
        if (model.empty()) {
            model = "claude-opus-5";
        }
        std::string baseUrl = environment("ANTHROPIC_BASE_URL");
        if (baseUrl.empty()) {
            baseUrl = "https://api.anthropic.com";
        }
        AnthropicHttpClient client(baseUrl, std::move(apiKey), opts.m_timeout_seconds,
                                   opts.m_verbose, error);
        if (!client) {
            return false;
        }

        json::Value toolsValue = anthropic_tools();
        json::Value messagesValue(
            json::Array{json::Object{{"role", "user"}, {"content", agent_task_message(task)}}});
        unsigned maxTurns = opts.m_max_tool_calls + 5;

        for (unsigned turn = 0; turn != maxTurns; ++turn) {
            json::Object request{
                {"model", model},
                {"max_tokens", opts.m_max_output_tokens ? opts.m_max_output_tokens : 16000},
                {"system", opts.m_system_prompt},
                {"tools", toolsValue},
                {"messages", messagesValue}};
            json::Object response;
            if (!client.post(request, response, error)) {
                return false;
            }
            if (std::optional<StringRef> reported = response.getString("model")) {
                model = reported->str();
            }
            json::Array *content = response.getArray("content");
            if (!content) {
                error = "Anthropic API response has no content array";
                return false;
            }

            json::Array toolResults;
            bool usedTool = false;
            bool accepted = false;
            for (const json::Value &blockValue : *content) {
                const json::Object *block = blockValue.getAsObject();
                if (!block || block->getString("type").value_or("") != "tool_use") {
                    continue;
                }
                usedTool = true;
                StringRef id = block->getString("id").value_or("");
                StringRef name = block->getString("name").value_or("");
                json::Object noArguments;
                const json::Object *arguments = block->getObject("input");
                if (!arguments) {
                    arguments = &noArguments;
                }

                data::DataToolResult toolResult;
                if (++result.m_tool_calls > opts.m_max_tool_calls) {
                    toolResult = {
                        formatv("tool call limit ({0}) reached; stop now", opts.m_max_tool_calls)
                            .str(),
                        true};
                } else {
                    toolResult = tools.call_tool(name, *arguments);
                }
                if (opts.m_verbose) {
                    errs() << "llmc++: tool " << name << " "
                           << truncated(formatv("{0}", json::Value(json::Object(*arguments))).str(),
                                        160)
                           << "\n         -> " << (toolResult.m_is_error ? "[error] " : "")
                           << truncated(toolResult.m_text, 160) << "\n";
                }
                accepted |= name == "submit" && !toolResult.m_is_error;
                toolResults.emplace_back(json::Object{
                    {"type", "tool_result"},
                    {"tool_use_id", id},
                    {"content", toolResult.m_text.empty() ? "(empty)" : toolResult.m_text},
                    {"is_error", toolResult.m_is_error}});
            }

            json::Array *messages = messagesValue.getAsArray();
            json::Array contentCopy = *content;
            messages->emplace_back(
                json::Object{{"role", "assistant"}, {"content", std::move(contentCopy)}});
            if (!toolResults.empty()) {
                messages->emplace_back(
                    json::Object{{"role", "user"}, {"content", std::move(toolResults)}});
            }
            if (accepted) {
                result.m_status = "ok";
                result.m_model = model;
                return true;
            }
            if (!usedTool) {
                break;
            }
        }

        result.m_status = "error";
        result.m_model = model;
        result.m_message = "the model stopped without an accepted submit";
        return true;
    }

}
