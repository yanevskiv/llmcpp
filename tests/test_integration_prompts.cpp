/*
 * C++ file for prompts integration tests.
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
#include "llmcpp/test/test_text.h"
#include "llmcpp/test/test_workspace.h"

// Standard headers used by test scenarios.
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// Namespace alias for test filesystem operations.
namespace fs = std::filesystem;

// Verify that plain prompt text reaches compiler-context output.
TEST_CASE("compiler context includes plain prompts", "[integration][prompts]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result =
        work.llmcpp({"-fllm-dump-context", "00_all_forms.cpp"});
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_out, {"\"signature\": \"void Counter::report() const\"",
                                                "\"prompt\": \"Store the sum of values in total.\"",
                                                "Balanced braces in prompts are fine: {",
                                                "\"name\": \"doubled\""});
}

// Verify target policy, prompt resolution, transport visibility, and cache metadata.
TEST_CASE("target options and prompt files reach the agent", "[integration][prompts]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "prompt.md") << "Replacement instructions.";
    std::ofstream(work.path() / "rules.md") << "Project rules.";
    std::ofstream(work.path() / "config.json")
        << R"json({"api_key":"secret-value","project":"scores"})json";
    llmcpp::test::TestCommandResult result = work.mock(
        "json/04_options.json",
        {"--llm", "-fllm-cache-dir=cache", "-fllm-model=default-model", "-fllm-max-attempts=5",
         "-fllm-timeout=10", "-fllm-system-prompt=prompt.md", "-fllm-append-prompt=rules.md",
         "-fllm-agent-config=config.json", "-fllm-transcript=trace.jsonl", "04_options.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::string log = llmcpp::test::read_file(work.path() / "04_options.log");
    llmcpp::test::check_contains(log, {"Replacement instructions.", "Project rules.",
                                       "target-model", "default-model", "\"max_attempts\": 12",
                                       "\"timeout_seconds\": 120", "\"cache\": \"disabled\"",
                                       "\"protocol_version\": 1", "secret-value"});
    std::string transcript = llmcpp::test::read_file(work.path() / "trace.jsonl");
    CHECK(transcript.find("secret-value") == std::string::npos);
    llmcpp::test::check_contains(transcript, {"[redacted]", "get_task", "try_compile", "submit"});
    unsigned entries = 0;
    for (const auto &entry : fs::directory_iterator(work.path() / "cache")) {
        ++entries;
        std::string cache = llmcpp::test::read_file(entry.path());
        llmcpp::test::check_contains(cache, {"// version: llmcpp-cache-3",
                                             "// context:", "// system_prompt:", "// agent:",
                                             "// cache_salt: \"reviewed\""});
        CHECK(entry.path().stem().string().size() == 7);
    }
    CHECK(entries == 1);
    llmcpp::test::TestCommandResult replay = work.llmcpp(
        {"--llm", "-fllm-cache-dir=cache", "-fllm-regenerate", "-fllm-model=default-model",
         "-fllm-max-attempts=5", "-fllm-timeout=10", "-fllm-system-prompt=prompt.md",
         "-fllm-append-prompt=rules.md", "-fllm-agent-config=config.json",
         "-fllm-agent=" + (fs::path(LLMCPP_PATH).parent_path() / "llmcpp-agent").string() +
             " --replay trace.jsonl",
         "04_options.cpp"});
    INFO(replay.m_err);
    REQUIRE(replay.m_status == 0);
}

// Replace system instructions for one target without leaking into other targets.
TEST_CASE("system prompt modifier is scoped to one function", "[integration][prompts]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "target.md") << "Target-only instructions.\n";
    std::ofstream(work.path() / "default.md") << "Driver instructions.\n";
    std::ofstream(work.path() / "append.md") << "Appended driver instructions.\n";
    work.write_fixture("17_system_prompt.cpp", "prompts.cpp");
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=cache",
                                  "-fllm-system-prompt=default.md", "-fllm-append-prompt=append.md",
                                  "prompts.cpp"};
    auto result = work.mock("json/05_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::string log = llmcpp::test::read_file(work.path() / "05_return_values.log");
    size_t split = log.find("== increment\n");
    REQUIRE(split != std::string::npos);
    std::string targetLog = log.substr(0, split);
    std::string defaultLog = log.substr(split);
    CHECK(targetLog.find("Target-only instructions.") != std::string::npos);
    CHECK(targetLog.find("Driver instructions.") == std::string::npos);
    CHECK(targetLog.find("Appended driver instructions.") == std::string::npos);
    llmcpp::test::check_contains(defaultLog,
                                 {"Driver instructions.", "Appended driver instructions."});
    CHECK(defaultLog.find("Target-only instructions.") == std::string::npos);
    args.push_back("-fllm-offline");
    SECTION("same prompt reuses cache")
    {
        result = work.llmcpp(args);
        REQUIRE(result.m_status == 0);
    }
    SECTION("changed prompt invalidates cache")
    {
        std::ofstream(work.path() / "target.md") << "Changed target instructions.\n";
        result = work.llmcpp(args);
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"no cached body", "answer"});
    }
    SECTION("missing prompt is diagnosed")
    {
        work.write_fixture("18_missing_system_prompt.cpp", "prompts.cpp");
        result = work.llmcpp(args);
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"cannot read system prompt", "missing.md"});
    }
    SECTION("invalid UTF-8 is diagnosed")
    {
        std::ofstream(work.path() / "target.md") << char(0xff);
        result = work.llmcpp(args);
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"not UTF-8"});
    }
}

// Verify file-backed modifiers override defaults without affecting other functions.
TEST_CASE("function prompt additions configuration and transcripts", "[integration][prompts]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "base.md") << "Target base.";
    std::ofstream(work.path() / "first.md") << "First addition.";
    std::ofstream(work.path() / "second.md") << "Second addition.";
    std::ofstream(work.path() / "default.md") << "Default instructions.";
    std::ofstream(work.path() / "default.json") << R"({"project":"default"})";
    std::ofstream(work.path() / "target.json") << R"({"project":"target","api_key":"secret"})";
    work.write_fixture("19_prompt_additions.cpp", "local.cpp");
    std::vector<std::string> args{"--llm",
                                  "-fllm-cache-dir=cache",
                                  "-fllm-system-prompt=default.md",
                                  "-fllm-agent-config=default.json",
                                  "-fllm-transcript=default.jsonl",
                                  "local.cpp"};
    auto result = work.mock("json/05_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::string log = llmcpp::test::read_file(work.path() / "05_return_values.log");
    size_t split = log.find("== increment\n");
    REQUIRE(split != std::string::npos);
    std::string targetLog = log.substr(0, split);
    std::string defaultLog = log.substr(split);
    llmcpp::test::check_contains(targetLog, {"Target base.", "First addition.", "Second addition.",
                                             "\"project\": \"target\""});
    CHECK(targetLog.find("Target base.") < targetLog.find("First addition."));
    CHECK(targetLog.find("First addition.") < targetLog.find("Second addition."));
    CHECK(targetLog.find("Default instructions.") == std::string::npos);
    llmcpp::test::check_contains(defaultLog, {"Default instructions.", "\"project\": \"default\""});
    CHECK(defaultLog.find("First addition.") == std::string::npos);
    std::string trace = llmcpp::test::read_file(work.path() / "target.jsonl");
    llmcpp::test::check_contains(trace, {"answer", "[redacted]", "submit"});
    CHECK(trace.find("secret") == std::string::npos);
    CHECK(trace.find("increment") == std::string::npos);
    CHECK(llmcpp::test::read_file(work.path() / "default.jsonl").find("answer") ==
          std::string::npos);
    args.push_back("-fllm-offline");
    REQUIRE(work.llmcpp(args).m_status == 0);
    SECTION("appended instructions invalidate cache")
    {
        std::ofstream(work.path() / "first.md") << "Changed addition.";
        REQUIRE(work.llmcpp(args).m_status != 0);
    }
    SECTION("configuration invalidates cache")
    {
        std::ofstream(work.path() / "target.json") << R"({"project":"changed"})";
        REQUIRE(work.llmcpp(args).m_status != 0);
    }
}

// Verify selective context dumps never launch an agent or produce compiled output.
TEST_CASE("dump context modifier selects functions", "[integration][prompts]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("20_dump_context.cpp", "local.cpp");
    auto result = work.llmcpp({"-fllm-agent=nonexistent-command", "local.cpp", "-o", "program"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    CHECK(result.m_out.find("answer") != std::string::npos);
    CHECK(result.m_out.find("\"name\": \"increment\"") == std::string::npos);
    CHECK_FALSE(fs::exists(work.path() / "program"));
    result = work.llmcpp({"--llm", "local.cpp", "-o", "generated.cpp"});
    REQUIRE(result.m_status == 0);
    CHECK_FALSE(fs::exists(work.path() / "generated.cpp"));
    result = work.llmcpp({"-fllm-dump-context", "local.cpp"});
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_out, {"\"name\": \"answer\"", "\"name\": \"increment\""});
}

// Attach reference files separately from instructions and track their contents.
TEST_CASE("reference context is additive and invalidates cache", "[integration][prompts]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "global.md") << "Global reference.";
    std::ofstream(work.path() / "local.md") << "Local reference.";
    work.write_fixture("21_reference_context.cpp", "answer.cpp");
    std::vector<std::string> args{"--llm", "-fllm-context=global.md", "-fllm-cache-dir=cache",
                                  "answer.cpp"};
    auto result = work.llmcpp({"-fllm-dump-context", "-fllm-context=global.md",
                               "-fllm-max-output-tokens=256", "answer.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_out,
                                 {"references", "Global reference.", "Local reference.", "512"});
    result = work.mock("json/05_return_values.json", args);
    REQUIRE(result.m_status == 0);
    args.push_back("-fllm-offline");
    result = work.llmcpp(args);
    REQUIRE(result.m_status == 0);
    std::ofstream(work.path() / "local.md") << "Changed reference.";
    result = work.llmcpp(args);
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err, {"no cached body"});
}
