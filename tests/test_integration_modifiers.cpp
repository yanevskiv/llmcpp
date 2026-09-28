/*
 * C++ file for modifiers integration tests.
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

// Catch2 declarations for integration tests.
#include <catch2/catch_test_macros.hpp>

// Test support for driver scenarios.
#include "llmcpp/test/test_text.h"
#include "llmcpp/test/test_workspace.h"

// Standard headers used by test scenarios.
#include <fstream>
#include <string>

// Accept bare modifiers, empty option lists, and configured modifiers together.
TEST_CASE("modifier parentheses are optional", "[integration][modifiers]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("14_modifier_parentheses.cpp", "modifiers.cpp");
    llmcpp::test::TestCommandResult result =
        work.mock("json/04_options.json", {"--llm", "-fllm-no-cache", "modifiers.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::string source = llmcpp::test::read_file(work.path() / "modifiers.llm.cpp");
    CHECK(source.find("__llm__") == std::string::npos);
    CHECK(llmcpp::test::count_occurrences(source, "return score;") == 3);
}

// Diagnose malformed file-backed modifiers before contacting an agent.
TEST_CASE("file backed modifiers reject invalid arguments", "[integration][modifiers]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "invalid.json") << "[]";
    std::ofstream(work.path() / "broken.json") << "{";
    std::ofstream(work.path() / "invalid.md") << char(0xff);
    for (const char *modifier : {"append_prompt(\"missing.md\")", "append_prompt(\"invalid.md\")",
                                 "agent_config(\"missing.json\")", "agent_config(\"invalid.json\")",
                                 "agent_config(\"broken.json\")", "transcript(\"\")",
                                 "transcript(2)", "agent_config(\"\")", "append_prompt(\"\")",
                                 "dump_context(1)", "transcript(\"a\"), transcript(\"b\")"}) {
        work.write_fixture("37_empty_answer_options.cpp", "local.cpp", {{"@OPTIONS@", modifier}});
        auto result = work.llmcpp({"--llm", "local.cpp"});
        INFO(modifier);
        INFO(result.m_err);
        REQUIRE(result.m_status != 0);
        CHECK(result.m_err.find("select an LLM backend") == std::string::npos);
    }
}

// Enforce target limits even when command-line defaults allow more work.
TEST_CASE("target generation budgets are enforced", "[integration][modifiers]")
{
    llmcpp::test::TestWorkspace work;
    SECTION("submission attempts")
    {
        work.write_fixture("15_max_attempts_budget.cpp", "budget.cpp");
        std::ofstream(work.path() / "json/test_budget.json") << R"json({
            "functions": {"*": [
                {"tool":"submit","arguments":{"body":"return missing;"}},
                {"tool":"submit","arguments":{"body":"return 7;"}}
            ]}
        })json";
        llmcpp::test::TestCommandResult result =
            work.mock("json/test_budget.json",
                      {"--llm", "-fllm-no-cache", "-fllm-max-attempts=4", "budget.cpp"});
        CHECK(result.m_status != 0);
        llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "test_budget.log"),
                                     {"last allowed attempt"});
    }
    SECTION("generation timeout")
    {
        work.write_fixture("16_timeout_budget.cpp", "budget.cpp");
        std::ofstream(work.path() / "json/test_budget.json") << R"json({
            "functions": {"*": [{"sleep":3},{"tool":"submit","arguments":{"body":""}}]}
        })json";
        llmcpp::test::TestCommandResult result = work.mock(
            "json/test_budget.json", {"--llm", "-fllm-no-cache", "-fllm-timeout=10", "budget.cpp"});
        CHECK(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"agent timed out after 1s"});
    }
}

// Enforce target tool budgets independently of driver defaults.
TEST_CASE("tool call modifier overrides driver defaults", "[integration][modifiers]")
{
    llmcpp::test::TestWorkspace work;
    std::string limit = "3";
    bool allowed = true;
    SECTION("target expands the driver budget") {}
    SECTION("target restricts the driver budget")
    {
        limit = "1";
        allowed = false;
    }
    work.write_fixture("40_tool_call_limit.cpp", "tools.cpp", {{"@LIMIT@", limit}});
    auto result =
        work.mock("json/05_return_values.json",
                  {"--llm", "-fllm-no-cache",
                   "-fllm-max-tool-calls=" + std::string(allowed ? "1" : "10"), "tools.cpp"});
    INFO(result.m_err);
    CHECK((result.m_status == 0) == allowed);
    CHECK(llmcpp::test::read_file(work.path() / "05_return_values.log")
              .find("\"max_tool_calls\": " + limit) != std::string::npos);
    if (!allowed) {
        llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "05_return_values.log"),
                                     {"tool call limit"});
    }
}

// Reject token limits that CLI adapters cannot enforce instead of ignoring them.
TEST_CASE("CLI backends reject explicit output token limits", "[integration][modifiers]")
{
    llmcpp::test::TestWorkspace work;
    for (const std::string &backend : {"codex", "claude"}) {
        auto result = work.llmcpp({"--llm", "-fllm-no-cache", "-fllm-backend=" + backend,
                                   "-fllm-max-output-tokens=512", "02_failure.cpp"},
                                  {{"LLMCPP_AGENT", ""},
                                   {"LLMCPP_CODEX", MOCK_AGENT_PATH},
                                   {"LLMCPP_CLAUDE", MOCK_AGENT_PATH}});
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"cannot enforce max_output_tokens"});
    }
}
