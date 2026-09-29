/*
 * C++ file for LLM backend selection and external-agent transport.
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

// Project headers for agent transport and generation options.
#include "llmcpp/agent_session.h"
#include "llmcpp/agent_anthropic_client.h"
#include "llmcpp/agent_openai_client.h"
#include "llmcpp/agent_prompt.h"
#include "llmcpp/data/data_generation_options.h"

// LLVM headers for JSON transport and process support.
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

// Standard and POSIX headers for subprocess communication.
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <poll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

// Namespace import for LLVM transport and support types.
using namespace llvm;
// Type alias for monotonic protocol deadlines.
using Clock = std::chrono::steady_clock;

// Namespace for the llmcpp agent transport implementation.
namespace llmcpp
{

    // Store immutable options without starting a process.
    AgentSession::AgentSession(const data::DataGenerationOptions &opts)
        : m_opts(opts)
    {
        // Empty.
    }

    // Close any active agent process.
    AgentSession::~AgentSession()
    {
        stop(false);
    }

    // Resolve the configured or adjacent agent executable.
    std::string AgentSession::command() const
    {
        if (!m_opts.m_agent_command.empty()) {
            return m_opts.m_agent_command;
        }
        if (const char *env = std::getenv("LLMCPP_AGENT"); env && *env) {
            return env;
        }
        SmallString<256> path(sys::path::parent_path(m_opts.m_executable));
        sys::path::append(path, "llmcpp-agent");
        if (sys::fs::can_execute(path)) {
            return "'" + std::string(path) + "'";
        }
        return "llmcpp-agent";
    }

    // Start the external agent and complete the MCP handshake.
    bool AgentSession::start(std::string &error)
    {
        int sockets[2];
        if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) != 0) {
            error = std::string("socketpair: ") + strerror(errno);
            return false;
        }
        std::string cmd = "exec " + command();
        pid_t child = fork();
        if (child < 0) {
            error = std::string("fork: ") + strerror(errno);
            close(sockets[0]);
            close(sockets[1]);
            return false;
        }
        if (child == 0) {
            dup2(sockets[1], STDIN_FILENO);
            dup2(sockets[1], STDOUT_FILENO);
            setenv("LLMCPP_BACKEND", m_opts.m_backend.c_str(), 1);
            setenv("LLMCPP_VERBOSE", m_opts.m_verbose ? "1" : "0", 1);
            execl("/bin/sh", "sh", "-c", cmd.c_str(), (char *)nullptr);
            _exit(127);
        }
        close(sockets[1]);
        m_fd = sockets[0];
        m_pid = child;
        m_buffer.clear();

        auto deadline =
            Clock::now() + std::chrono::seconds(std::min(60U, m_opts.m_timeout_seconds));
        unsigned noToolCalls = 0;
        while (true) {
            std::optional<json::Value> msg = receive(deadline, error);
            if (!msg) {
                std::string exit = stop(true);
                error = "agent '" + command() + "' failed to start: " + error +
                        (exit.empty() ? "" : " (" + exit + ")");
                return false;
            }
            json::Object *obj = msg->getAsObject();
            if (!obj) {
                continue;
            }
            StringRef method = obj->getString("method").value_or("");
            if (method == "initialize") {
                if (json::Object *params = obj->getObject("params")) {
                    if (json::Object *info = params->getObject("clientInfo")) {
                        if (std::optional<StringRef> m = info->getString("model")) {
                            m_model = m->str();
                        }
                    }
                }
            }
            if (method == "notifications/initialized") {
                return true;
            }
            if (!handle_incoming(*obj, nullptr, noToolCalls, error)) {
                stop(true);
                return false;
            }
        }
    }

    // Close the transport and reap the child process.
    std::string AgentSession::stop(bool shouldKill)
    {
        if (m_pid <= 0) {
            return "";
        }
        if (m_fd >= 0) {
            close(m_fd);
            m_fd = -1;
        }
        auto waitFor = [&](std::chrono::milliseconds timeout, int &status) {
            auto deadline = Clock::now() + timeout;
            while (true) {
                pid_t r = waitpid(m_pid, &status, WNOHANG);
                if (r != 0 || Clock::now() >= deadline) {
                    return r;
                }
                usleep(20000);
            }
        };
        int status = 0;
        pid_t r = waitFor(std::chrono::milliseconds(shouldKill ? 0 : 3000), status);
        if (r == 0) {
            kill(m_pid, SIGTERM);
            r = waitFor(std::chrono::milliseconds(2000), status);
            if (r == 0) {
                kill(m_pid, SIGKILL);
                r = waitpid(m_pid, &status, 0);
            }
        }
        m_pid = -1;
        m_buffer.clear();
        if (r > 0 && WIFEXITED(status)) {
            return formatv("exit status {0}", WEXITSTATUS(status)).str();
        }
        if (r > 0 && WIFSIGNALED(status)) {
            return formatv("killed by signal {0}", WTERMSIG(status)).str();
        }
        return "";
    }

    // Serialize and send one JSON-RPC message.
    bool AgentSession::send(json::Value message, std::string &error)
    {
        agent_record(m_opts, "send", message);
        std::string line = formatv("{0}", message).str();
        line += '\n';
        size_t done = 0;
        while (done < line.size()) {
            ssize_t n = ::send(m_fd, line.data() + done, line.size() - done, MSG_NOSIGNAL);
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                error = std::string("write to agent failed: ") + strerror(errno);
                return false;
            }
            done += n;
        }
        return true;
    }

    // Receive and parse one newline-delimited JSON-RPC message.
    std::optional<json::Value> AgentSession::receive(Clock::time_point deadline, std::string &error)
    {
        m_timed_out = false;
        while (true) {
            if (Clock::now() >= deadline) {
                m_timed_out = true;
                error = "timed out";
                return std::nullopt;
            }
            size_t nl = m_buffer.find('\n');
            if (nl != std::string::npos) {
                std::string line = m_buffer.substr(0, nl);
                m_buffer.erase(0, nl + 1);
                if (StringRef(line).trim().empty()) {
                    continue;
                }
                Expected<json::Value> v = json::parse(line);
                if (!v) {
                    error = "invalid JSON from agent (" + toString(v.takeError()) +
                            "): " + line.substr(0, 200);
                    return std::nullopt;
                }
                agent_record(m_opts, "receive", *v);
                return std::move(*v);
            }

            auto now = Clock::now();
            if (now >= deadline) {
                m_timed_out = true;
                error = "timed out";
                return std::nullopt;
            }
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
            pollfd p{m_fd, POLLIN, 0};
            int r = poll(&p, 1, static_cast<int>(std::min<long long>(ms.count(), 1000)));
            if (r < 0 && errno != EINTR) {
                error = std::string("poll: ") + strerror(errno);
                return std::nullopt;
            }
            if (r <= 0) {
                continue;
            }

            char chunk[65536];
            ssize_t n = read(m_fd, chunk, sizeof chunk);
            if (n < 0) {
                if (errno == EINTR || errno == EAGAIN) {
                    continue;
                }
                error = std::string("read from agent failed: ") + strerror(errno);
                return std::nullopt;
            }
            if (n == 0) {
                error = "agent exited";
                return std::nullopt;
            }
            m_buffer.append(chunk, n);
        }
    }

    // Shorten verbose agent messages for progress logging.
    static std::string truncated(StringRef s, size_t max)
    {
        std::string line = s.split('\n').first.str();
        if (line.size() > max || line.size() < s.size()) {
            line = line.substr(0, max) + " ...";
        }
        return line;
    }

    // Service one incoming agent request or response.
    bool AgentSession::handle_incoming(const json::Object &msg, AgentToolHandler *tools,
                                       unsigned &toolCalls, std::string &error)
    {
        StringRef method = msg.getString("method").value_or("");
        const json::Value *id = msg.get("id");
        const json::Object *params = msg.getObject("params");

        if (!id) {
            if (method == "notifications/message" && m_opts.m_verbose && params) {
                if (const json::Value *data = params->get("data")) {
                    std::optional<StringRef> s = data->getAsString();
                    errs() << "llmc++: agent: " << (s ? s->str() : formatv("{0}", *data).str())
                           << "\n";
                }
            }
            return true;
        }

        json::Object response{{"jsonrpc", "2.0"}, {"id", *id}};
        if (method == "initialize") {
            std::string version = "2025-06-18";
            if (params) {
                if (std::optional<StringRef> v = params->getString("protocolVersion")) {
                    version = v->str();
                }
            }
            response["result"] = json::Object{
                {"protocolVersion", version},
                {"llmcppProtocolVersion", 1},
                {"llmcppCapabilities",
                 json::Array{"system_prompt", "model", "agent_config", "effective_settings"}},
                {"capabilities", json::Object{{"tools", json::Object{}}}},
                {"serverInfo", json::Object{{"name", "llmc++"}, {"version", LLMCPP_VERSION}}}};
        } else if (method == "tools/list") {
            response["result"] = json::Object{{"tools", tool_definitions()}};
        } else if (method == "ping") {
            response["result"] = json::Object{};
        } else if (method == "tools/call") {
            StringRef name = params ? params->getString("name").value_or("") : "";
            json::Object noArguments;
            const json::Object *arguments = params ? params->getObject("arguments") : nullptr;
            if (!arguments) {
                arguments = &noArguments;
            }

            data::DataToolResult r;
            if (!tools) {
                r = {"no function is being generated yet", true};
            } else if (++toolCalls > m_opts.m_max_tool_calls) {
                r = {formatv("tool call limit ({0}) reached; stop now", m_opts.m_max_tool_calls)
                         .str(),
                     true};
            } else {
                r = tools->call_tool(name, *arguments);
            }

            if (m_opts.m_verbose) {
                errs() << "llmc++: tool " << name << " "
                       << truncated(formatv("{0}", json::Value(json::Object(*arguments))).str(),
                                    160)
                       << "\n         -> " << (r.m_is_error ? "[error] " : "")
                       << truncated(r.m_text, 160) << "\n";
            }

            response["result"] = json::Object{
                {"content", json::Array{json::Object{{"type", "text"}, {"text", r.m_text}}}},
                {"isError", r.m_is_error}};
        } else {
            response["error"] =
                json::Object{{"code", -32601}, {"message", ("method not found: " + method).str()}};
        }
        return send(std::move(response), error);
    }

    // Run one complete generation exchange.
    bool AgentSession::generate(json::Object task, AgentToolHandler &tools,
                                data::DataAgentOutcome &result, std::string &error)
    {
        const char *externalAgent = std::getenv("LLMCPP_AGENT");
        if (m_opts.m_backend.empty() && m_opts.m_agent_command.empty() &&
            !(externalAgent && *externalAgent)) {
            error = "select an LLM backend with -fllm-backend=<backend> or LLMCPP_BACKEND";
            return false;
        }
        if (use_native_anthropic(m_opts)) {
            bool completed = generate_anthropic(m_opts, std::move(task), tools, result, error);
            if (!result.m_model.empty()) {
                m_model = result.m_model;
            }
            return completed;
        }
        if (use_native_openai(m_opts)) {
            bool completed = generate_openai(m_opts, std::move(task), tools, result, error);
            if (!result.m_model.empty()) {
                m_model = result.m_model;
            }
            return completed;
        }
        auto deadline = Clock::now() + std::chrono::seconds(m_opts.m_timeout_seconds);
        if (m_pid <= 0 && !start(error)) {
            return false;
        }

        std::string id = formatv("llmcpp-{0}", m_next_id++).str();
        if (!send(json::Object{{"jsonrpc", "2.0"},
                               {"id", id},
                               {"method", "llm/generate"},
                               {"params", std::move(task)}},
                  error)) {
            stop(true);
            return false;
        }

        while (true) {
            std::optional<json::Value> msg = receive(deadline, error);
            if (!msg) {
                bool wasTimeout = m_timed_out;
                std::string exit = stop(true);
                if (wasTimeout) {
                    error = formatv("agent timed out after {0}s", m_opts.m_timeout_seconds);
                } else if (!exit.empty()) {
                    error += " (" + exit + ")";
                }
                return false;
            }
            json::Object *obj = msg->getAsObject();
            if (!obj) {
                continue;
            }

            if (!obj->get("method")) {
                std::optional<StringRef> responseId = obj->getString("id");
                if (!responseId || *responseId != id) {
                    continue;
                }
                if (json::Object *err = obj->getObject("error")) {
                    result.m_status = "error";
                    result.m_message = err->getString("message").value_or("agent error").str();
                    return true;
                }
                if (json::Object *res = obj->getObject("result")) {
                    result.m_status = res->getString("status").value_or("ok").str();
                    result.m_message = res->getString("message").value_or("").str();
                    result.m_model = res->getString("model").value_or("").str();
                }
                if (!result.m_model.empty()) {
                    m_model = result.m_model;
                }
                return true;
            }

            if (!handle_incoming(*obj, &tools, result.m_tool_calls, error)) {
                stop(true);
                return false;
            }
        }
    }

}
