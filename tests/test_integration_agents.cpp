/*
 * C++ file for agents integration tests.
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

// Catch2 declarations for integration tests.
#include <catch2/catch_test_macros.hpp>

// Test support for driver scenarios.
#include "llmcpp/test/test_fake_anthropic_server.h"
#include "llmcpp/test/test_fake_openai_server.h"
#include "llmcpp/test/test_text.h"
#include "llmcpp/test/test_workspace.h"

// Standard headers used by test scenarios.
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// Namespace alias for test filesystem operations.
namespace fs = std::filesystem;

// Verify diagnostics for an agent-declared generation failure.
TEST_CASE("agent failures are reported", "[integration][agents]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result =
        work.mock("json/02_failure.json", {"-fllm-no-cache", "02_failure.cpp", "-o", "failure"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(
        result.m_err, {"LLM failed to generate a body for 'f': mock gave up",
                       "last rejected attempt", "use of undeclared identifier 'not_declared'"});
}

// Launch a function's agent instead of the driver agent or native backend.
TEST_CASE("agent modifier overrides driver defaults", "[integration][agents]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("43_agent_command.cpp", "agent.cpp", {{"@AGENT@", MOCK_AGENT_PATH}});
    auto result = work.llmcpp(
        {"--llm", "-fllm-no-cache", "-fllm-backend=openai", "-fllm-agent=/nonexistent/agent",
         "agent.cpp"},
        {{"LLMCPP_AGENT", "/nonexistent/environment-agent"},
         {"LLMCPP_MOCK_SCRIPT", (work.path() / "json/05_return_values.json").string()}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "agent.llm.cpp"),
                                 {"return 42;", MOCK_AGENT_PATH});
    for (const char *options : {"agent(\"\")", "agent(\"   \")", "agent(2)"}) {
        work.write_fixture("37_empty_answer_options.cpp", "agent.cpp", {{"@OPTIONS@", options}});
        result = work.llmcpp({"-fllm-dump-context", "agent.cpp"});
        REQUIRE(result.m_status != 0);
    }
}

// Select a function's backend ahead of environment and command-line defaults.
TEST_CASE("backend modifier overrides driver defaults", "[integration][agents]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("33_backend_override.cpp", "backend.cpp");
    std::vector<std::string> args{"--llm", "-fllm-no-cache", "backend.cpp"};
    SECTION("no driver backend") {}
    SECTION("command line backend")
    {
        args.push_back("-fllm-backend=openai");
    }
    auto result = work.llmcpp(args, {{"LLMCPP_BACKEND", ""},
                                     {"LLMCPP_AGENT", ""},
                                     {"LLMCPP_CODEX", MOCK_AGENT_PATH},
                                     {"LLMCPP_EFFORT", "high"}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "backend.llm.cpp"),
                                 {"// model:", "void generated()"});
}

// Verify that credentials and installed programs never select a backend.
TEST_CASE("generation requires an explicit backend", "[integration][agents]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult missing =
        work.llmcpp({"-fllm-no-cache", "02_failure.cpp"}, {{"LLMCPP_BACKEND", ""},
                                                           {"LLMCPP_AGENT", ""},
                                                           {"ANTHROPIC_API_KEY", "test-key"},
                                                           {"OPENAI_API_KEY", "test-key"},
                                                           {"LLMCPP_CODEX", MOCK_AGENT_PATH},
                                                           {"LLMCPP_CLAUDE", MOCK_AGENT_PATH}});
    REQUIRE(missing.m_status != 0);
    llmcpp::test::check_contains(missing.m_err, {"select an LLM backend", "-fllm-backend"});
    for (const std::string &backend : {"auto", "unknown", ""}) {
        llmcpp::test::TestCommandResult invalid =
            work.llmcpp({"-fllm-backend=" + backend, "02_failure.cpp"});
        CHECK(invalid.m_status != 0);
    }
}

// Verify that Anthropic generation runs in-process without the Python agent.
TEST_CASE("native Anthropic client completes a compiler tool loop", "[integration][agents]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestFakeAnthropicServer server;
    llmcpp::test::TestCommandResult result =
        work.llmcpp({"-fllm-no-cache", "-fllm-max-output-tokens=768", "02_failure.cpp", "-o",
                     "native-anthropic"},
                    {{"LLMCPP_AGENT", ""},
                     {"LLMCPP_BACKEND", "anthropic"},
                     {"ANTHROPIC_API_KEY", "test-key"},
                     {"ANTHROPIC_BASE_URL", server.base_url()},
                     {"LLMCPP_MODEL", "requested-test-model"}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    CHECK(server.calls() == 3);
    CHECK(server.problem().empty());

    std::vector<std::string> requests = server.requests();
    REQUIRE(requests.size() == 3);
    llmcpp::test::check_contains(requests[0], {"\"model\":\"requested-test-model\"", "\"get_task\"",
                                               "\"messages\"", "\"system\"", "\"max_tokens\":768"});
    llmcpp::test::check_contains(
        requests[1], {"\"tool_use_id\":\"call-1\"", "\"tool_result\"", "Do something impossible."});
    llmcpp::test::check_contains(
        requests[2], {"\"tool_use_id\":\"call-2\"", "compiles without errors or warnings"});

    llmcpp::test::TestCommandResult run = work.run("./native-anthropic");
    REQUIRE(run.m_status == 0);
}

// Verify that OpenAI generation runs in-process without the Python agent.
TEST_CASE("native OpenAI client completes a compiler tool loop", "[integration][agents]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestFakeOpenAIServer server;
    std::vector<std::string> args{"-fllm-no-cache",
                                  "-fllm-max-output-tokens=768",
                                  "-fllm-backend=openai",
                                  "02_failure.cpp",
                                  "-o",
                                  "native-openai"};
    SECTION("native client") {}
    SECTION("bundled Python agent")
    {
        std::ofstream(work.path() / "openai.json") << "{}";
        args.push_back("-fllm-agent-config=openai.json");
    }
    llmcpp::test::TestCommandResult result =
        work.llmcpp(args, {{"LLMCPP_AGENT", ""},
                           {"LLMCPP_BACKEND", "anthropic"},
                           {"OPENAI_API_KEY", "test-key"},
                           {"OPENAI_BASE_URL", server.base_url()},
                           {"LLMCPP_MODEL", "requested-test-model"}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    CHECK(server.calls() == 3);
    CHECK(server.problem().empty());

    std::vector<std::string> requests = server.requests();
    REQUIRE(requests.size() == 3);
    llmcpp::test::check_contains(requests[0], {"\"model\"", "\"requested-test-model\"",
                                               "\"get_task\"", "\"instructions\"", "\"function\""});
    for (const std::string &request : requests) {
        CHECK(request.find("768") != std::string::npos);
    }
    llmcpp::test::check_contains(requests[1], {"\"previous_response_id\"", "\"response-1\"",
                                               "\"function_call_output\"", "call-1",
                                               "Do something impossible."});
    llmcpp::test::check_contains(requests[2], {"\"previous_response_id\"", "\"response-2\"",
                                               "call-2", "compiles without errors or warnings"});

    llmcpp::test::TestCommandResult run = work.run("./native-openai");
    REQUIRE(run.m_status == 0);
}

// Verify that a custom Python agent translates compiler tools for a model server.
TEST_CASE("custom chat agent completes a compiler tool loop", "[integration][agents]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestFakeOpenAIServer server;
    fs::path adapter = work.path() / "python/07_chat_agent.py";
    std::ofstream(work.path() / "chat.json")
        << "{\"base_url\":\"" << server.base_url() << "/v1\",\"model\":\"config-model\"}";
    llmcpp::test::TestCommandResult result = work.llmcpp(
        {"-fllm-no-cache", "-fllm-agent=python3 " + llmcpp::test::shell_quote(adapter.string()),
         "-fllm-agent-config=chat.json", "-fllm-model=local-model", "02_failure.cpp", "-o",
         "custom-agent"},
        {{"LOCAL_MODEL_API_KEY", "test-key"}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    CHECK(server.calls() == 3);
    CHECK(server.problem().empty());
    auto requests = server.requests();
    REQUIRE(requests.size() == 3);
    llmcpp::test::check_contains(requests[0], {"local-model", "system", "get_task"});
    CHECK(requests[0].find("config-model") == std::string::npos);
    llmcpp::test::check_contains(requests[1],
                                 {"Do something impossible.", "timeout_seconds", "max_attempts"});
    CHECK(work.run("./custom-agent").m_status == 0);
}

// Verify that the Codex CLI connects to the compiler through the MCP bridge.
TEST_CASE("Codex CLI completes a compiler tool loop", "[integration][agents]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "codex.json")
        << "{\"effort\":\"high\",\"executable\":\"" << MOCK_AGENT_PATH << "\"}";
    std::vector<std::string> args{"-fllm-no-cache", "02_failure.cpp", "-o", "codex"};
    std::string backend = "codex";
    SECTION("environment defaults") {}
    SECTION("command line overrides environment")
    {
        args.push_back("-fllm-backend=codex");
        backend = "claude";
    }
    SECTION("agent configuration")
    {
        args.push_back("-fllm-agent-config=codex.json");
    }
    llmcpp::test::TestCommandResult result = work.llmcpp(args, {{"ANTHROPIC_API_KEY", ""},
                                                                {"LLMCPP_AGENT", ""},
                                                                {"LLMCPP_BACKEND", backend},
                                                                {"LLMCPP_CODEX", MOCK_AGENT_PATH},
                                                                {"LLMCPP_EFFORT", "high"}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);

    llmcpp::test::TestCommandResult run = work.run("./codex");
    REQUIRE(run.m_status == 0);
}

// Verify diagnostics when the configured agent cannot start.
TEST_CASE("a missing agent is reported", "[integration][agents]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result =
        work.llmcpp({"-fllm-agent=/nonexistent/llmcpp-agent", "-fllm-no-cache", "02_failure.cpp",
                     "-o", "failure"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err, {"failed to start"});
}
