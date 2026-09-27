/*
 * C++ file for direct OpenAI Responses API generation.
 */

// Project headers for native generation and compiler-context tools.
#include "llmcpp/agent_openai_client.h"
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
            error = "OPENAI_BASE_URL must start with http:// or https://";
            return false;
        }
        size_t slash = url.find('/', scheme + 3);
        origin = url.take_front(slash).str();
        prefix = slash == StringRef::npos ? "" : url.drop_front(slash).rtrim('/').str();
        if (origin.size() == scheme + 3) {
            error = "OPENAI_BASE_URL has no host";
            return false;
        }
        return true;
    }

    // Own the HTTP configuration and retry loop for one generation request.
    class OpenAIHttpClient
    {
    public:
        OpenAIHttpClient(std::string baseUrl, std::string apiKey, unsigned timeoutSeconds,
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
            httplib::Headers headers{{"Authorization", "Bearer " + m_api_key}};

            for (unsigned attempt = 0; attempt != 6; ++attempt) {
                auto remaining =
                    std::chrono::duration_cast<std::chrono::seconds>(m_deadline - Clock::now());
                if (remaining <= std::chrono::seconds::zero()) {
                    error = "OpenAI API request timed out";
                    return false;
                }
                m_client->set_connection_timeout(remaining);
                m_client->set_read_timeout(remaining);
                m_client->set_write_timeout(remaining);

                httplib::Result result =
                    m_client->Post(m_prefix + "/v1/responses", headers, body, "application/json");
                if (result && result->status >= 200 && result->status < 300) {
                    Expected<json::Value> parsed = json::parse(result->body);
                    if (!parsed) {
                        error = "invalid JSON from OpenAI API: " + toString(parsed.takeError());
                        return false;
                    }
                    json::Object *object = parsed->getAsObject();
                    if (!object) {
                        error = "OpenAI API returned a non-object JSON response";
                        return false;
                    }
                    response = std::move(*object);
                    return true;
                }

                int status = result ? result->status : 0;
                bool transient = !result || status == 408 || status == 409 || status == 429 ||
                                 status == 500 || status == 502 || status == 503 || status == 504;
                if (transient && attempt != 5) {
                    unsigned delay = std::min(60U, 2U << attempt);
                    if (m_verbose) {
                        errs() << "llmc++: OpenAI "
                               << (result ? formatv("HTTP {0}", status).str()
                                          : httplib::to_string(result.error()))
                               << ", retrying in " << delay << "s\n";
                    }
                    auto wake = std::min(m_deadline, Clock::now() + std::chrono::seconds(delay));
                    std::this_thread::sleep_until(wake);
                    continue;
                }

                if (result) {
                    error = formatv("OpenAI API error {0}: {1}", status,
                                    StringRef(result->body).take_front(500))
                                .str();
                } else {
                    error = "cannot reach the OpenAI API: " + httplib::to_string(result.error());
                }
                return false;
            }
            error = "OpenAI API request failed";
            return false;
        }

    private:
        std::unique_ptr<httplib::Client> m_client;
        std::string m_api_key;
        std::string m_prefix;
        Clock::time_point m_deadline;
        bool m_verbose;
    };

    // Convert MCP-flavored schemas to Responses API function tools.
    json::Value openai_tools()
    {
        json::Array definitions = llmcpp::tool_definitions();
        json::Array converted;
        for (json::Value &value : definitions) {
            json::Object *definition = value.getAsObject();
            if (!definition) {
                continue;
            }
            json::Object tool{{"type", "function"},
                              {"name", definition->getString("name").value_or("")},
                              {"description", definition->getString("description").value_or("")},
                              {"strict", false}};
            if (json::Value *schema = definition->get("inputSchema")) {
                tool["parameters"] = *schema;
            } else {
                tool["parameters"] = json::Object{{"type", "object"}};
            }
            converted.emplace_back(std::move(tool));
        }
        return json::Value(std::move(converted));
    }
}

// Namespace for the native OpenAI session implementation.
namespace llmcpp
{
    // Select native OpenAI unless an external-agent override takes precedence.
    bool use_native_openai(const data::DataGenerationOptions &opts)
    {
        if (!opts.m_agent_command.empty() || !opts.m_agent_config_file.empty() ||
            !environment("LLMCPP_AGENT").empty()) {
            return false;
        }
        std::string backend = environment("LLMCPP_BACKEND");
        if (backend == "openai") {
            return true;
        }
        return (backend.empty() || backend == "auto") && !environment("OPENAI_API_KEY").empty();
    }

    // Run the Responses API function-calling loop in the compiler process.
    bool generate_openai(const data::DataGenerationOptions &opts, json::Object task,
                         AgentToolHandler &tools, data::DataAgentOutcome &result,
                         std::string &error)
    {
        std::string apiKey = environment("OPENAI_API_KEY");
        if (apiKey.empty()) {
            error = "LLMCPP_BACKEND=openai needs OPENAI_API_KEY";
            return false;
        }
        std::string model = opts.m_model;
        if (model.empty()) {
            model = "gpt-6-astra";
        }
        std::string baseUrl = environment("OPENAI_BASE_URL");
        if (baseUrl.empty()) {
            baseUrl = "https://api.openai.com";
        }
        OpenAIHttpClient client(baseUrl, std::move(apiKey), opts.m_timeout_seconds, opts.m_verbose,
                                error);
        if (!client) {
            return false;
        }

        json::Value toolsValue = openai_tools();
        json::Value inputValue(agent_task_message(task));
        std::string previousResponseId;
        unsigned maxTurns = opts.m_max_tool_calls + 5;

        for (unsigned turn = 0; turn != maxTurns; ++turn) {
            json::Object request{{"model", model},
                                 {"instructions", opts.m_system_prompt},
                                 {"tools", toolsValue},
                                 {"input", inputValue},
                                 {"max_output_tokens", 16000}};
            if (!previousResponseId.empty()) {
                request["previous_response_id"] = previousResponseId;
            }
            json::Object response;
            if (!client.post(request, response, error)) {
                return false;
            }
            if (std::optional<StringRef> reported = response.getString("model")) {
                model = reported->str();
            }
            std::optional<StringRef> responseId = response.getString("id");
            if (!responseId) {
                error = "OpenAI API response has no id";
                return false;
            }
            previousResponseId = responseId->str();
            json::Array *output = response.getArray("output");
            if (!output) {
                error = "OpenAI API response has no output array";
                return false;
            }

            json::Array toolOutputs;
            bool usedTool = false;
            bool accepted = false;
            for (const json::Value &itemValue : *output) {
                const json::Object *item = itemValue.getAsObject();
                if (!item || item->getString("type").value_or("") != "function_call") {
                    continue;
                }
                usedTool = true;
                StringRef callId = item->getString("call_id").value_or("");
                StringRef name = item->getString("name").value_or("");
                StringRef encodedArguments = item->getString("arguments").value_or("{}");
                json::Object noArguments;
                json::Object *arguments = nullptr;
                Expected<json::Value> parsedArguments = json::parse(encodedArguments);
                if (parsedArguments) {
                    arguments = parsedArguments->getAsObject();
                }

                data::DataToolResult toolResult;
                if (!arguments) {
                    if (!parsedArguments) {
                        consumeError(parsedArguments.takeError());
                    }
                    arguments = &noArguments;
                    toolResult = {"function arguments were not a JSON object", true};
                } else if (++result.m_tool_calls > opts.m_max_tool_calls) {
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
                std::string outputText = toolResult.m_text.empty() ? "(empty)" : toolResult.m_text;
                if (toolResult.m_is_error) {
                    outputText = "[error] " + outputText;
                }
                toolOutputs.emplace_back(json::Object{{"type", "function_call_output"},
                                                      {"call_id", callId.str()},
                                                      {"output", std::move(outputText)}});
            }

            if (accepted) {
                result.m_status = "ok";
                result.m_model = model;
                return true;
            }
            if (!usedTool) {
                break;
            }
            inputValue = json::Value(std::move(toolOutputs));
        }

        result.m_status = "error";
        result.m_model = model;
        result.m_message = "the model stopped without an accepted submit";
        return true;
    }
}
