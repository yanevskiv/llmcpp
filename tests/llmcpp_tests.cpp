/*
 * C++ file for Catch2 integration coverage for llmc++.
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

// Catch2 declarations and integration-test support.
#include <catch2/catch_test_macros.hpp>

#include "llmcpp/test/test_fake_anthropic_server.h"
#include "llmcpp/test/test_fake_openai_server.h"
#include "llmcpp/test/test_text.h"
#include "llmcpp/test/test_workspace.h"

// Standard headers used by test scenarios.
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

// Namespace alias for test filesystem operations.
namespace fs = std::filesystem;

// Verify that the compiler and bundled agent report their own release versions.
TEST_CASE("compiler and agent report independent versions", "[options]")
{
    llmcpp::test::TestWorkspace work;
    auto compiler = work.llmcpp({"--version"}, {{"LLMCPP_TIMEOUT", "invalid"}});
    CHECK(compiler.m_status == 0);
    CHECK(compiler.m_out == "llmc++ 0.0.1\n");
    CHECK(compiler.m_err.empty());

    auto agent = work.run(LLMCPP_AGENT_PATH, {"--version"});
    CHECK(agent.m_status == 0);
    CHECK(agent.m_out == "llmcpp-agent 0.0.1\n");
    CHECK(agent.m_err.empty());
}

// Verify that driver help includes both Clang and llmcpp options without configuration.
TEST_CASE("driver help includes generation options", "[options]")
{
    llmcpp::test::TestWorkspace work;
    for (const char *help : {"--help", "-help", "--help-hidden"}) {
        llmcpp::test::TestCommandResult result =
            work.llmcpp({help, "--llm", "-fllm-system-prompt=missing.md"},
                        {{"LLMCPP_BACKEND", ""}, {"LLMCPP_AGENT", ""}});
        INFO(result.m_err);
        REQUIRE(result.m_status == 0);
        CHECK(result.m_err.empty());
        llmcpp::test::check_contains(
            result.m_out, {"USAGE:", "LLMCPP OPTIONS:", "--llm", "-fllm-backend=", "-fllm-no-cache",
                           "-fllm-transcript=", "-fllm-verbose",
                           "-fllm-cache-lifetime=", "-fllm-cache-salt=", "-fllm-regenerate",
                           "-fllm-context=", "-fllm-cache-read-only", "-fllm-explain-cache",
                           "-fllm-max-output-tokens=", "-fllm-dump-code", "-fllm-append-prompt="});
        CHECK(result.m_out.find("-fllm-force-regenerate") == std::string::npos);
        result = work.llmcpp({help}, {{"LLMCPP_TIMEOUT", "not-a-number"}});
        CHECK(result.m_status == 0);
    }
}

// Resolve environment defaults before command-line and function overrides.
TEST_CASE("environment generation defaults and overrides", "[options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "env.cpp") << "__llm__ int f() { Return one. }\n";
    std::ofstream(work.path() / "first.md") << "First reference.";
    std::ofstream(work.path() / "second.md") << "Second reference.";
    std::ofstream(work.path() / "system.md") << "Environment instructions.";
    std::ofstream(work.path() / "append.md") << "Additional instructions.";
    std::ofstream(work.path() / "extra.md") << "More instructions.";
    std::vector<std::pair<std::string, std::string>> environment = {
        {"LLMCPP_BACKEND", "codex"},           {"LLMCPP_MODEL", "environment-model"},
        {"LLMCPP_SYSTEM_PROMPT", "system.md"}, {"LLMCPP_APPEND_PROMPT", "append.md"},
        {"LLMCPP_CONTEXT", "[\"first.md\"]"},  {"LLMCPP_DUMP_CONTEXT", "true"},
        {"LLMCPP_MAX_ATTEMPTS", "7"},          {"LLMCPP_MAX_TOOL_CALLS", "8"},
        {"LLMCPP_MAX_OUTPUT_TOKENS", "256"},   {"LLMCPP_TIMEOUT", "12"}};
    auto result =
        work.llmcpp({"env.cpp", "-fllm-context=second.md", "-fllm-model=cli-model"}, environment);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_out,
                                 {"First reference.", "Second reference.", "cli-model", "256"});
    CHECK(result.m_out.find("environment-model") == std::string::npos);
    environment[3].second = "[\"append.md\", \"extra.md\"]";
    environment[4].second = "first.md";
    result = work.llmcpp({"env.cpp"}, environment);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_out, {"First reference."});
    CHECK(result.m_out.find("Second reference.") == std::string::npos);
    std::ofstream(work.path() / "env.cpp")
        << "__llm__(model(\"function-model\"), max_output_tokens(512)) int f() {}\n";
    result = work.llmcpp({"env.cpp", "-fllm-model=cli-model"}, environment);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_out, {"function-model", "512"});
    std::ofstream(work.path() / "plain.cpp") << "int main() {}\n";
    result = work.llmcpp({"plain.cpp", "-fsyntax-only", "-fllm-dump-context=false",
                          "-fllm-verbose=off", "-fllm-no-cache=0"},
                         {{"LLMCPP_DUMP_CONTEXT", "yes"},
                          {"LLMCPP_VERBOSE", "on"},
                          {"LLMCPP_NO_CACHE", "true"},
                          {"LLMCPP_BACKEND", ""}});
    REQUIRE(result.m_status == 0);
    CHECK(result.m_out.empty());
    CHECK(result.m_err.empty());
}

// Reject malformed environment defaults using the command-line validators.
TEST_CASE("environment generation defaults validate values", "[options]")
{
    llmcpp::test::TestWorkspace work;
    for (const auto &[name, value] :
         {std::pair{"LLMCPP_OFFLINE", "maybe"}, std::pair{"LLMCPP_VERBOSE", "2"},
          std::pair{"LLMCPP_CONTEXT", "[1]"}, std::pair{"LLMCPP_APPEND_PROMPT", "["},
          std::pair{"LLMCPP_MAX_ATTEMPTS", "0"}, std::pair{"LLMCPP_MAX_TOOL_CALLS", "-1"},
          std::pair{"LLMCPP_MAX_OUTPUT_TOKENS", "0"}, std::pair{"LLMCPP_TIMEOUT", "no"},
          std::pair{"LLMCPP_HASH_ABBREV", "65"}, std::pair{"LLMCPP_CACHE_LIFETIME", "-1"}}) {
        auto result =
            work.llmcpp({"-fsyntax-only", "test_integration_failure.cpp"}, {{name, value}});
        INFO(name);
        CHECK(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {name});
    }
    auto result = work.llmcpp({"-fllm-offline=maybe", "test_integration_failure.cpp"});
    CHECK(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err, {"invalid boolean"});
    for (const char *name : {"LLMCPP_SYSTEM_PROMPT", "LLMCPP_APPEND_PROMPT", "LLMCPP_CONTEXT",
                             "LLMCPP_AGENT_CONFIG"}) {
        result = work.llmcpp({"-fsyntax-only", "test_integration_failure.cpp"},
                             {{name, "missing.file"}});
        CHECK(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"missing.file"});
    }
}

// Exercise environment cache policy and custom-agent defaults through generation.
TEST_CASE("environment cache policies control generation", "[options][cache]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "env.cpp") << "__llm__ int f(int score) {}\n";
    std::vector<std::pair<std::string, std::string>> environment = {
        {"LLMCPP_AGENT", MOCK_AGENT_PATH},
        {"LLMCPP_BACKEND", ""},
        {"LLMCPP_MOCK_SCRIPT", (work.path() / "json/test_integration_options.json").string()},
        {"LLMCPP_CACHE_DIR", "env-cache"},
        {"LLMCPP_CACHE_SALT", "env-salt"},
        {"LLMCPP_CACHE_LIFETIME", "0"},
        {"LLMCPP_HASH_ABBREV", "10"},
        {"LLMCPP_CACHE_READ_ONLY", "true"},
        {"LLMCPP_EXPLAIN_CACHE", "yes"},
        {"LLMCPP_DUMP_CODE", "on"},
        {"LLMCPP_VERBOSE", "0"},
        {"LLMCPP_TRANSCRIPT", "events.jsonl"}};
    auto result = work.llmcpp({"--llm", "env.cpp"}, environment);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_err, {"return score;"});
    llmcpp::test::check_contains(result.m_err, {"cache is read-only"});
    CHECK_FALSE(fs::exists(work.path() / "env-cache"));
    CHECK(fs::exists(work.path() / "events.jsonl"));
    result = work.llmcpp({"--llm", "env.cpp", "-fllm-cache-read-only=false"}, environment);
    REQUIRE(result.m_status == 0);
    REQUIRE(fs::exists(work.path() / "env-cache"));
    environment.emplace_back("LLMCPP_OFFLINE", "true");
    environment.emplace_back("LLMCPP_REGENERATE", "true");
    result = work.llmcpp({"--llm", "env.cpp"}, environment);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_err, {"regeneration requested"});
    result = work.llmcpp({"--llm", "env.cpp", "-fllm-regenerate=false"}, environment);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_err, {"hit"});
}

// Verify diagnostics for unsupported or malformed annotations.
TEST_CASE("invalid annotations produce llmc++ diagnostics", "[diagnostics]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result =
        work.llmcpp({"-fsyntax-only", "test_integration_errors.cpp"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(
        result.m_err, {"preprocessor directives are not allowed in an __llm__ function body",
                       "__llm__ function 'declaration_only' must have a body containing the prompt",
                       "__llm__ function 'compile_time' cannot be constexpr",
                       "__llm__ function 'try_block' cannot have a function-try-block",
                       "__llm__ function 'S::S' cannot be defaulted or deleted",
                       "__llm__ cannot be used inside a macro expansion",
                       "__llm__ must be followed by a function definition or a lambda"});
}

// Verify that annotations in included headers are rejected.
TEST_CASE("annotations in headers are rejected", "[diagnostics]")
{
    llmcpp::test::TestWorkspace work;
    for (const char *mode : {"-fsyntax-only", "--llm"}) {
        DYNAMIC_SECTION(mode)
        {
            llmcpp::test::TestCommandResult result =
                work.llmcpp({mode, "test_integration_header.cpp"});
            REQUIRE(result.m_status != 0);
            llmcpp::test::check_contains(
                result.m_err, {"__llm__ function in included header 'test_integration_header.h'"});
        }
    }
}

// Verify that offline mode fails when no cached body exists.
TEST_CASE("offline mode requires cached bodies", "[cache]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result =
        work.llmcpp({"-fsyntax-only", "-fllm-offline", "-fllm-cache-dir=empty",
                     "test_integration_all_forms.cpp"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err,
                                 {"no cached body for __llm__ function 'sum' (-fllm-offline)"});
}

// Verify that plain prompt text reaches compiler-context output.
TEST_CASE("compiler context includes plain prompts", "[context]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result =
        work.llmcpp({"-fllm-dump-context", "test_integration_all_forms.cpp"});
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_out, {"\"signature\": \"void Counter::report() const\"",
                                                "\"prompt\": \"Store the sum of values in total.\"",
                                                "Balanced braces in prompts are fine: {",
                                                "\"name\": \"doubled\""});
}

// Verify target policy, prompt resolution, transport visibility, and cache metadata.
TEST_CASE("target options and prompt files reach the agent", "[generation][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "prompt.md") << "Replacement instructions.";
    std::ofstream(work.path() / "rules.md") << "Project rules.";
    std::ofstream(work.path() / "config.json")
        << R"json({"api_key":"secret-value","project":"scores"})json";
    llmcpp::test::TestCommandResult result =
        work.mock("json/test_integration_options.json",
                  {"--llm", "-fllm-cache-dir=cache", "-fllm-model=default-model",
                   "-fllm-max-attempts=5", "-fllm-timeout=10", "-fllm-system-prompt=prompt.md",
                   "-fllm-append-prompt=rules.md", "-fllm-agent-config=config.json",
                   "-fllm-transcript=trace.jsonl", "test_integration_options.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::string log = llmcpp::test::read_file(work.path() / "test_integration_options.log");
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
         "test_integration_options.cpp"});
    INFO(replay.m_err);
    REQUIRE(replay.m_status == 0);
}

// Verify that changes to visible headers and instructions invalidate reviewed bodies.
TEST_CASE("cache tracks context and system instructions", "[cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "cached.cpp")
        << "#include \"include/test_integration_context.h\"\n__llm__() int cached() { Return the "
           "score. }\n";
    llmcpp::test::TestCommandResult generated = work.mock(
        "json/test_integration_options.json", {"--llm", "-fllm-cache-dir=cache", "cached.cpp"});
    INFO(generated.m_err);
    REQUIRE(generated.m_status == 0);
    llmcpp::test::TestCommandResult offline =
        work.llmcpp({"--llm", "-fllm-offline", "-fllm-cache-dir=cache", "cached.cpp"});
    INFO(offline.m_err);
    REQUIRE(offline.m_status == 0);
    SECTION("header contents")
    {
        std::ofstream(work.path() / "include/test_integration_score.h")
            << "inline constexpr int score = 8;\n";
    }
    SECTION("system prompt")
    {
        std::ofstream(work.path() / "rules.md") << "Use a different implementation style.";
    }
    SECTION("metadata")
    {
        for (const auto &entry : fs::directory_iterator(work.path() / "cache")) {
            std::ofstream(entry.path())
                << "// llmcpp cache entry\n// model: mock\n// ---\nreturn 7;\n";
        }
    }
    std::vector<std::string> args{"--llm", "-fllm-offline", "-fllm-cache-dir=cache", "cached.cpp"};
    if (fs::exists(work.path() / "rules.md")) {
        args.push_back("-fllm-append-prompt=rules.md");
    }
    offline = work.llmcpp(args);
    CHECK(offline.m_status != 0);
    llmcpp::test::check_contains(offline.m_err, {"no cached body"});
}

// Accept bare modifiers, empty option lists, and configured modifiers together.
TEST_CASE("modifier parentheses are optional", "[generation][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "modifiers.cpp")
        << "#include \"include/test_integration_context.h\"\n"
        << "__llm__ int bare() { Return the score. }\n"
        << "__llm__() int empty() { Return the score. }\n"
        << "__llm__(max_attempts(2)) int configured() { Return the score. }\n";
    llmcpp::test::TestCommandResult result = work.mock(
        "json/test_integration_options.json", {"--llm", "-fllm-no-cache", "modifiers.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::string source = llmcpp::test::read_file(work.path() / "modifiers.llm.cpp");
    CHECK(source.find("__llm__") == std::string::npos);
    CHECK(llmcpp::test::count_occurrences(source, "return score;") == 3);
}

// Verify compiler identification during preprocessing, generation, and compilation.
TEST_CASE("compiler identification macro is defined", "[generation][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "macro.cpp") << "#if !defined(__LLMCPP__) || __LLMCPP__ != 1\n"
                                             << "#error missing compiler identification\n"
                                             << "#endif\n"
                                             << "#include \"include/test_integration_context.h\"\n"
                                             << "__llm__ int generated() { Return the score. }\n";
    llmcpp::test::TestCommandResult preprocess = work.llmcpp({"-E", "macro.cpp"});
    INFO(preprocess.m_err);
    REQUIRE(preprocess.m_status == 0);
    llmcpp::test::TestCommandResult compile = work.mock(
        "json/test_integration_options.json", {"-fsyntax-only", "-fllm-no-cache", "macro.cpp"});
    INFO(compile.m_err);
    REQUIRE(compile.m_status == 0);
    std::ofstream(work.path() / "plain.cpp")
        << "#ifndef __LLMCPP__\n#error missing compiler identification\n#endif\n"
        << "static_assert(__LLMCPP__ == 1);\n";
    llmcpp::test::TestCommandResult plain = work.llmcpp({"-fsyntax-only", "plain.cpp"});
    INFO(plain.m_err);
    REQUIRE(plain.m_status == 0);
}

// Replace system instructions for one target without leaking into other targets.
TEST_CASE("system prompt modifier is scoped to one function", "[generation][cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "target.md") << "Target-only instructions.\n";
    std::ofstream(work.path() / "default.md") << "Driver instructions.\n";
    std::ofstream(work.path() / "append.md") << "Appended driver instructions.\n";
    std::ofstream(work.path() / "prompts.cpp")
        << "__llm__(system_prompt(\"target.md\")) int answer() { Return 42. }\n"
        << "__llm__ int increment(int x) { Return x plus one. }\n";
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=cache",
                                  "-fllm-system-prompt=default.md", "-fllm-append-prompt=append.md",
                                  "prompts.cpp"};
    auto result = work.mock("json/test_integration_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::string log = llmcpp::test::read_file(work.path() / "test_integration_return_values.log");
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
        std::ofstream(work.path() / "prompts.cpp")
            << "__llm__(system_prompt(\"missing.md\")) int answer() {}\n";
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
TEST_CASE("function prompt additions configuration and transcripts", "[generation][cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "base.md") << "Target base.";
    std::ofstream(work.path() / "first.md") << "First addition.";
    std::ofstream(work.path() / "second.md") << "Second addition.";
    std::ofstream(work.path() / "default.md") << "Default instructions.";
    std::ofstream(work.path() / "default.json") << R"({"project":"default"})";
    std::ofstream(work.path() / "target.json") << R"({"project":"target","api_key":"secret"})";
    std::ofstream(work.path() / "local.cpp")
        << "__llm__(append_prompt(\"first.md\"), system_prompt(\"base.md\"), "
           "append_prompt(\"second.md\"), agent_config(\"target.json\"), "
           "transcript(\"target.jsonl\")) int answer() { Return 42. }\n"
        << "__llm__ int increment(int x) { Return x plus one. }\n";
    std::vector<std::string> args{"--llm",
                                  "-fllm-cache-dir=cache",
                                  "-fllm-system-prompt=default.md",
                                  "-fllm-agent-config=default.json",
                                  "-fllm-transcript=default.jsonl",
                                  "local.cpp"};
    auto result = work.mock("json/test_integration_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::string log = llmcpp::test::read_file(work.path() / "test_integration_return_values.log");
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
TEST_CASE("dump context modifier selects functions", "[options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "local.cpp")
        << "__llm__(dump_context) int answer() { Return 42. }\n"
        << "__llm__ int increment(int x) { Return x plus one. }\n";
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

// Verify default salts affect cache identity and function salts replace them.
TEST_CASE("command line cache salt supplies a default", "[cache][options]")
{
    llmcpp::test::TestWorkspace work;
    auto result = work.mock("json/test_integration_options.json",
                            {"--llm", "-fllm-cache-salt=driver", "test_integration_options.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    for (const auto &entry : fs::directory_iterator(work.path() / ".llmcache")) {
        llmcpp::test::check_contains(llmcpp::test::read_file(entry.path()),
                                     {"// cache_salt: \"reviewed\""});
    }
    std::ofstream(work.path() / "local.cpp") << "__llm__ int answer() { Return 42. }\n";
    std::vector<std::string> args{"--llm", "-fllm-cache-salt=driver", "local.cpp"};
    REQUIRE(work.mock("json/test_integration_return_values.json", args).m_status == 0);
    args.push_back("-fllm-offline");
    REQUIRE(work.llmcpp(args).m_status == 0);
    args[1] = "-fllm-cache-salt=changed";
    REQUIRE(work.llmcpp(args).m_status != 0);
    REQUIRE(work.mock("json/test_integration_return_values.json",
                      {"--llm", "-fllm-cache-salt=driver", "-fllm-no-cache",
                       "-fllm-cache-dir=unused", "local.cpp"})
                .m_status == 0);
    CHECK_FALSE(fs::exists(work.path() / "unused"));
    result = work.llmcpp({"-fllm-cache-salt=", "local.cpp"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err, {"requires a nonempty salt"});
}

// Diagnose malformed file-backed modifiers before contacting an agent.
TEST_CASE("file backed modifiers reject invalid arguments", "[diagnostics][options]")
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
        std::ofstream(work.path() / "local.cpp") << "__llm__(" << modifier << ") int answer() {}\n";
        auto result = work.llmcpp({"--llm", "local.cpp"});
        INFO(modifier);
        INFO(result.m_err);
        REQUIRE(result.m_status != 0);
        CHECK(result.m_err.find("select an LLM backend") == std::string::npos);
    }
}

// Enforce target limits even when command-line defaults allow more work.
TEST_CASE("target generation budgets are enforced", "[generation][options]")
{
    llmcpp::test::TestWorkspace work;
    SECTION("submission attempts")
    {
        std::ofstream(work.path() / "budget.cpp") << "__llm__(max_attempts(1)) int budget() {}\n";
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
        std::ofstream(work.path() / "budget.cpp") << "__llm__(timeout(1)) void budget() {}\n";
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
TEST_CASE("tool call modifier overrides driver defaults", "[generation][options]")
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
    std::ofstream(work.path() / "tools.cpp")
        << "__llm__(max_tool_calls(" << limit << ")) int answer() { Return 42. }\n";
    auto result =
        work.mock("json/test_integration_return_values.json",
                  {"--llm", "-fllm-no-cache",
                   "-fllm-max-tool-calls=" + std::string(allowed ? "1" : "10"), "tools.cpp"});
    INFO(result.m_err);
    CHECK((result.m_status == 0) == allowed);
    CHECK(llmcpp::test::read_file(work.path() / "test_integration_return_values.log")
              .find("\"max_tool_calls\": " + limit) != std::string::npos);
    if (!allowed) {
        llmcpp::test::check_contains(
            llmcpp::test::read_file(work.path() / "test_integration_return_values.log"),
            {"tool call limit"});
    }
}

// Reject malformed policies before contacting any model.
TEST_CASE("invalid generation configuration is diagnosed", "[options]")
{
    llmcpp::test::TestWorkspace work;
    for (const std::string &options : {"timeout(0)",
                                       "max_tool_calls(0)",
                                       "max_tool_calls(\"2\")",
                                       "max_tool_calls(-1)",
                                       "model(2)",
                                       "cache_salt(\"v1\"), no_cache",
                                       "cache(\"v1\")",
                                       "cache_salt(\"\")",
                                       "cache_salt(2)",
                                       "cache_salt(\"v1\"), cache_salt(\"v2\")",
                                       "timeout(1), timeout(2)",
                                       "unknown(1)",
                                       "offline, no_cache",
                                       "offline, offline",
                                       "offline(1)",
                                       "key(\"xyz\")",
                                       "key(\"abcdef\")",
                                       "key(2)",
                                       "key(\"abcdef0\"), no_cache",
                                       "backend(\"auto\")",
                                       "backend(\"claude-code\")",
                                       "backend(2)",
                                       "backend(\"\")"}) {
        std::ofstream(work.path() / "invalid.cpp") << "__llm__(" << options << ") int f() {}\n";
        llmcpp::test::TestCommandResult result = work.llmcpp({"-fllm-dump-context", "invalid.cpp"});
        INFO(options);
        CHECK(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"error:"});
    }
    llmcpp::test::TestCommandResult missing =
        work.llmcpp({"-fllm-system-prompt=missing.md", "test_integration_failure.cpp"});
    CHECK(missing.m_status != 0);
    llmcpp::test::check_contains(missing.m_err, {"cannot read system prompt"});
    std::ofstream(work.path() / "invalid.json") << "[]";
    llmcpp::test::TestCommandResult config =
        work.llmcpp({"-fllm-agent-config=invalid.json", "test_integration_failure.cpp"});
    CHECK(config.m_status != 0);
    llmcpp::test::check_contains(config.m_err, {"agent configuration must be a JSON object"});
}

// Verify generated source, native compilation, preprocessing, and caching.
TEST_CASE("generated sources compile and cache reproducibly", "[generation][cache]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult generate =
        work.mock("json/test_integration_all_forms.json",
                  {"--llm", "-fllm-cache-dir=cache", "test_integration_all_forms.cpp"});
    INFO(generate.m_err);
    REQUIRE(generate.m_status == 0);

    fs::path generated = work.path() / "test_integration_all_forms.llm.cpp";
    REQUIRE(fs::exists(generated));
    std::string source = llmcpp::test::read_file(generated);
    CHECK(source.find("__llm__") == std::string::npos);
    CHECK(llmcpp::test::count_occurrences(source, "// model: mock-model") == 8);
    CHECK(llmcpp::test::count_occurrences(source, "// prompt:") == 8);

    llmcpp::test::TestCommandResult build =
        work.llmcpp({"test_integration_all_forms.llm.cpp", "-o", "from-llm-cpp"});
    INFO(build.m_err);
    REQUIRE(build.m_status == 0);
    llmcpp::test::TestCommandResult run = work.run("./from-llm-cpp");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out ==
          llmcpp::test::read_file(work.path() / "test_integration_all_forms.expected"));

    llmcpp::test::TestCommandResult gxx =
        work.run("g++", {"-std=c++17", "test_integration_all_forms.llm.cpp", "-o", "with-gxx"});
    INFO(gxx.m_err);
    REQUIRE(gxx.m_status == 0);
    run = work.run("./with-gxx");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out ==
          llmcpp::test::read_file(work.path() / "test_integration_all_forms.expected"));

    llmcpp::test::TestCommandResult offline =
        work.llmcpp({"-fllm-offline", "-fllm-cache-dir=cache", "test_integration_all_forms.cpp",
                     "-o", "direct"});
    INFO(offline.m_err);
    REQUIRE(offline.m_status == 0);
    run = work.run("./direct");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out ==
          llmcpp::test::read_file(work.path() / "test_integration_all_forms.expected"));

    auto oldTime = fs::file_time_type::clock::now() - std::chrono::hours(24);
    fs::last_write_time(generated, oldTime);
    llmcpp::test::TestCommandResult regenerate = work.llmcpp(
        {"--llm", "-fllm-offline", "-fllm-cache-dir=cache", "test_integration_all_forms.cpp"});
    INFO(regenerate.m_err);
    REQUIRE(regenerate.m_status == 0);
    CHECK(fs::last_write_time(generated) == oldTime);

    llmcpp::test::TestCommandResult preprocess =
        work.llmcpp({"--llm", "-E", "-fllm-offline", "-fllm-cache-dir=cache",
                     "test_integration_all_forms.cpp"});
    INFO(preprocess.m_err);
    REQUIRE(preprocess.m_status == 0);
    llmcpp::test::check_contains(
        llmcpp::test::read_file(work.path() / "test_integration_all_forms.llm.ii"), {"++count;"});

    llmcpp::test::TestCommandResult multiple =
        work.llmcpp({"--llm", "test_integration_all_forms.cpp", "test_integration_tools.cpp", "-o",
                     "both.cpp"});
    REQUIRE(multiple.m_status != 0);
    llmcpp::test::check_contains(multiple.m_err,
                                 {"cannot specify -o when generating multiple output files"});
}

// Require cached bodies for individual targets without contacting an agent.
TEST_CASE("offline modifier requires a cached body", "[generation][cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "answer.cpp") << "__llm__(       ) int answer() { Return 42. }\n";
    auto result = work.mock("json/test_integration_return_values.json",
                            {"--llm", "-fllm-cache-dir=cache", "answer.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::ofstream(work.path() / "answer.cpp") << "__llm__(offline) int answer() { Return 42. }\n";
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=cache",
                                  "-fllm-agent=/nonexistent/agent", "answer.cpp"};
    SECTION("cache hit") {}
    SECTION("modifier overrides disabled cache")
    {
        args.push_back("-fllm-no-cache");
    }
    SECTION("cache miss")
    {
        args.push_back("-fllm-cache-dir=missing-cache");
    }
    result = work.llmcpp(args);
    INFO(result.m_err);
    if (args.back() == "-fllm-cache-dir=missing-cache") {
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"no cached body", "offline"});
        CHECK(result.m_err.find("failed to start") == std::string::npos);
    } else {
        REQUIRE(result.m_status == 0);
        llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "answer.llm.cpp"),
                                     {"return 42;"});
    }
}

// Attach reference files separately from instructions and track their contents.
TEST_CASE("reference context is additive and invalidates cache", "[cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "global.md") << "Global reference.";
    std::ofstream(work.path() / "local.md") << "Local reference.";
    std::ofstream(work.path() / "answer.cpp")
        << "__llm__(context(\"local.md\"), context(\"global.md\"), max_output_tokens(512)) "
           "int answer() { Return 42. }\n";
    std::vector<std::string> args{"--llm", "-fllm-context=global.md", "-fllm-cache-dir=cache",
                                  "answer.cpp"};
    auto result = work.llmcpp({"-fllm-dump-context", "-fllm-context=global.md",
                               "-fllm-max-output-tokens=256", "answer.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_out,
                                 {"references", "Global reference.", "Local reference.", "512"});
    result = work.mock("json/test_integration_return_values.json", args);
    REQUIRE(result.m_status == 0);
    args.push_back("-fllm-offline");
    result = work.llmcpp(args);
    REQUIRE(result.m_status == 0);
    std::ofstream(work.path() / "local.md") << "Changed reference.";
    result = work.llmcpp(args);
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err, {"no cached body"});
}

// Preserve cache entries while explaining hits, misses, and regeneration.
TEST_CASE("read only caching permits generation without writes", "[cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::string modifiers;
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=cache", "answer.cpp"};
    SECTION("command line")
    {
        args.push_back("-fllm-cache-read-only");
        args.push_back("-fllm-explain-cache");
    }
    SECTION("modifiers")
    {
        modifiers = "cache_read_only, explain_cache";
    }
    std::ofstream(work.path() / "answer.cpp")
        << "__llm__(" << modifiers << ") int answer() { Return 42. }\n";
    auto result = work.mock("json/test_integration_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_err, {"miss:", "not written: cache is read-only"});
    CHECK_FALSE(fs::exists(work.path() / "cache"));
    std::vector<std::string> writable{"--llm", "-fllm-cache-dir=cache", "answer.cpp"};
    std::ofstream(work.path() / "answer.cpp")
        << "__llm__(" << std::string(modifiers.size(), ' ') << ") int answer() { Return 42. }\n";
    result = work.mock("json/test_integration_return_values.json", writable);
    REQUIRE(result.m_status == 0);
    std::ofstream(work.path() / "answer.cpp")
        << "__llm__(" << modifiers << ") int answer() { Return 42. }\n";
    result = work.llmcpp(args);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_err, {"hit:"});
    args.push_back("-fllm-regenerate");
    std::map<fs::path, std::string> before;
    for (const auto &entry : fs::directory_iterator(work.path() / "cache")) {
        before[entry.path()] = llmcpp::test::read_file(entry.path());
    }
    result = work.mock("json/test_integration_return_values.json", args);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_err, {"bypass:", "not written:"});
    for (const auto &[path, contents] : before) {
        CHECK(llmcpp::test::read_file(path) == contents);
    }
}

// Reject invalid reference files and output-token limits before contacting an agent.
TEST_CASE("reference and token options validate arguments", "[options]")
{
    llmcpp::test::TestWorkspace work;
    for (const std::string &modifier :
         {"context(\"missing.md\")", "context(\"\")", "context(3)", "max_output_tokens(0)",
          "max_output_tokens(-1)", "max_output_tokens(\"x\")", "cache_read_only(1)",
          "explain_cache(1)"}) {
        std::ofstream(work.path() / "invalid.cpp")
            << "__llm__(" << modifier << ") int answer() { Return 42. }\n";
        auto result = work.llmcpp({"--llm", "invalid.cpp"});
        REQUIRE(result.m_status != 0);
        CHECK(result.m_err.find("select an LLM backend") == std::string::npos);
    }
    for (const char *value : {"0", "-1", "abc", ""}) {
        auto result = work.llmcpp({std::string("-fllm-max-output-tokens=") + value, "invalid.cpp"});
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"invalid value for -fllm-max-output-tokens"});
    }
    auto result = work.llmcpp({"-fllm-context=missing.md", "invalid.cpp"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err, {"cannot read context file"});
    std::ofstream(work.path() / "invalid.md", std::ios::binary) << '\xff';
    result = work.llmcpp({"-fllm-context=invalid.md", "invalid.cpp"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err, {"not UTF-8"});
    std::ofstream(work.path() / "invalid.cpp")
        << "__llm__(context(\"invalid.md\")) int answer() { Return 42. }\n";
    result = work.llmcpp({"--llm", "invalid.cpp"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err, {"not UTF-8"});
}

// Reject token limits that CLI adapters cannot enforce instead of ignoring them.
TEST_CASE("CLI backends reject explicit output token limits", "[options][agent]")
{
    llmcpp::test::TestWorkspace work;
    for (const std::string &backend : {"codex", "claude"}) {
        auto result = work.llmcpp({"--llm", "-fllm-no-cache", "-fllm-backend=" + backend,
                                   "-fllm-max-output-tokens=512", "test_integration_failure.cpp"},
                                  {{"LLMCPP_AGENT", ""},
                                   {"LLMCPP_CODEX", MOCK_AGENT_PATH},
                                   {"LLMCPP_CLAUDE", MOCK_AGENT_PATH}});
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"cannot enforce max_output_tokens"});
    }
}

// Force fresh bodies globally or per function, including in offline mode.
TEST_CASE("force regeneration overrides cached bodies and offline mode", "[cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::string modifiers;
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=cache", "answer.cpp"};
    SECTION("command line overrides offline modifier")
    {
        modifiers = "offline";
        args.push_back("-fllm-regenerate");
    }
    SECTION("modifier overrides command line offline")
    {
        modifiers = "regenerate";
        args.push_back("-fllm-offline");
    }
    SECTION("modifier overrides offline modifier")
    {
        modifiers = "offline, regenerate";
    }
    SECTION("command line overrides command line offline")
    {
        args.push_back("-fllm-offline");
        args.push_back("-fllm-regenerate");
    }
    std::ofstream(work.path() / "answer.cpp")
        << "__llm__(" << modifiers << ") int answer() { Return 42. }\n";
    auto result = work.mock("json/test_integration_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    for (const auto &entry : fs::directory_iterator(work.path() / "cache")) {
        std::string body = llmcpp::test::read_file(entry.path());
        size_t offset = body.find("return 42;");
        REQUIRE(offset != std::string::npos);
        body.replace(offset, 10, "return 41;");
        std::ofstream(entry.path()) << body;
    }
    result = work.mock("json/test_integration_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "answer.llm.cpp"),
                                 {"return 42;"});
    for (const auto &entry : fs::directory_iterator(work.path() / "cache")) {
        llmcpp::test::check_contains(llmcpp::test::read_file(entry.path()), {"return 42;"});
    }
}

// Expire cache entries by age without changing their computed identities.
TEST_CASE("cache lifetime expires old entries", "[generation][cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "answer.cpp") << "__llm__ int answer() { Return 42. }\n";
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=cache", "answer.cpp"};
    auto result = work.mock("json/test_integration_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    fs::path cache = fs::directory_iterator(work.path() / "cache")->path();
    auto oldTime = fs::file_time_type::clock::now() - std::chrono::hours(2);
    bool expired = false;
    bool regenerate = false;
    SECTION("fresh entry is reusable")
    {
        args.push_back("-fllm-cache-lifetime=3600");
    }
    SECTION("default has no expiry")
    {
        fs::last_write_time(cache, oldTime);
    }
    SECTION("zero disables expiry")
    {
        fs::last_write_time(cache, oldTime);
        args.push_back("-fllm-cache-lifetime=0");
    }
    SECTION("offline rejects expired entry")
    {
        fs::last_write_time(cache, oldTime);
        args.push_back("-fllm-cache-lifetime=3600");
        expired = true;
    }
    SECTION("online replaces expired entry")
    {
        fs::last_write_time(cache, oldTime);
        args.push_back("-fllm-cache-lifetime=3600");
        regenerate = true;
    }
    if (regenerate) {
        result = work.mock("json/test_integration_return_values.json", args);
        INFO(result.m_err);
        REQUIRE(result.m_status == 0);
        CHECK(fs::last_write_time(cache) > oldTime);
        CHECK(std::distance(fs::directory_iterator(cache.parent_path()),
                            fs::directory_iterator{}) == 1);
    } else {
        auto before = fs::last_write_time(cache);
        args.push_back("-fllm-offline");
        result = work.llmcpp(args);
        INFO(result.m_err);
        CHECK((result.m_status != 0) == expired);
        CHECK(fs::last_write_time(cache) == before);
        if (expired) {
            llmcpp::test::check_contains(result.m_err, {"no cached body"});
        }
    }
}

// Override driver lifetimes per function, including pinned offline cache entries.
TEST_CASE("cache lifetime modifier overrides driver defaults", "[generation][cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "answer.cpp")
        << "__llm__(key(\"abcdef0123\")) int answer() { Return 42. }\n";
    auto result = work.mock("json/test_integration_return_values.json",
                            {"--llm", "-fllm-cache-dir=cache", "answer.cpp"});
    REQUIRE(result.m_status == 0);
    fs::path cache = work.path() / "cache/abcdef0.cpp";
    auto oldTime = fs::file_time_type::clock::now() - std::chrono::hours(2);
    fs::last_write_time(cache, oldTime);
    std::string lifetime = "0";
    std::string driverLifetime = "1";
    bool expired = false;
    SECTION("zero overrides finite driver lifetime") {}
    SECTION("target permits longer lifetime")
    {
        lifetime = "10800";
    }
    SECTION("target expires a pinned entry despite unlimited driver lifetime")
    {
        lifetime = "3600";
        driverLifetime = "0";
        expired = true;
    }
    std::ofstream(work.path() / "answer.cpp")
        << "__llm__(offline, key(\"abcdef0123\"), cache_lifetime(" << lifetime
        << ")) int answer() { Return 42. }\n";
    result =
        work.llmcpp({"--llm", "-fllm-cache-dir=cache", "-fllm-cache-lifetime=" + driverLifetime,
                     "-fllm-agent=/nonexistent/agent", "answer.cpp"});
    INFO(result.m_err);
    CHECK((result.m_status != 0) == expired);
    CHECK(fs::last_write_time(cache) == oldTime);
    CHECK(result.m_err.find("failed to start") == std::string::npos);
}

// Reject invalid lifetimes on the command line and individual targets.
TEST_CASE("cache lifetime requires nonnegative integers", "[options]")
{
    llmcpp::test::TestWorkspace work;
    for (const char *value : {"-1", "abc", "1.5", "4294967296", ""}) {
        auto result = work.llmcpp({std::string("-fllm-cache-lifetime=") + value, "-fsyntax-only",
                                   "test_integration_all_forms.cpp"});
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"invalid value for -fllm-cache-lifetime"});
    }
    for (const char *value : {"-1", "\"1\"", "1.5", "4294967296", ""}) {
        std::ofstream(work.path() / "invalid.cpp")
            << "__llm__(cache_lifetime(" << value << ")) int answer() {}\n";
        auto result = work.llmcpp({"-fllm-dump-context", "invalid.cpp"});
        REQUIRE(result.m_status != 0);
    }
}

// Keep each function's cache entries in its selected directory.
TEST_CASE("cache directory modifier is scoped to one function", "[generation][cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "directories.cpp")
        << "__llm__(cache_dir(\"custom-cache\"), key(\"abcdef0123\")) int answer() { Return 42. }\n"
        << "__llm__ int increment(int x) { Return x plus one. }\n";
    fs::create_directory(work.path() / "custom-cache");
    std::ofstream(work.path() / "custom-cache/abcdef0.cpp") << "occupied prefix\n";
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=default-cache", "directories.cpp"};
    auto result = work.mock("json/test_integration_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    REQUIRE(fs::exists(work.path() / "custom-cache/abcdef01.cpp"));
    CHECK(llmcpp::test::read_file(work.path() / "custom-cache/abcdef0.cpp") == "occupied prefix\n");
    CHECK(std::distance(fs::directory_iterator(work.path() / "default-cache"),
                        fs::directory_iterator{}) == 1);
    args.push_back("-fllm-offline");
    result = work.llmcpp(args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::ofstream(work.path() / "directories.cpp") << "__llm__(cache_dir(\"\")) int answer() {}\n";
    result = work.llmcpp({"-fllm-dump-context", "directories.cpp"});
    REQUIRE(result.m_status != 0);
}

// Pin cached implementations independently of prompts and compilation context.
TEST_CASE("explicit cache keys pin generated bodies", "[generation][cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "answer.cpp")
        << "__llm__(key(\"ABCDEF0123\")) int answer() { Return 42. }\n";
    auto result = work.mock("json/test_integration_return_values.json",
                            {"--llm", "-fllm-no-cache", "-fllm-cache-dir=cache", "answer.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    fs::path cache = work.path() / "cache/abcdef0.cpp";
    REQUIRE(fs::exists(cache));
    std::string metadata = llmcpp::test::read_file(cache);
    llmcpp::test::check_contains(metadata, {"// key: abcdef0123\n", "return 42;"});
    std::ofstream(work.path() / "answer.cpp")
        << "__llm__(offline, key(\"abcdef0\")) int answer() { Return something else. }\n";
    SECTION("cache hit ignores changed inputs")
    {
        std::ofstream(work.path() / "rules.md") << "Different system instructions.\n";
        result = work.llmcpp({"--llm", "-fllm-cache-dir=cache", "-fllm-backend=claude",
                              "-fllm-system-prompt=rules.md", "answer.cpp"});
        INFO(result.m_err);
        REQUIRE(result.m_status == 0);
        CHECK(llmcpp::test::read_file(cache) == metadata);
        llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "answer.llm.cpp"),
                                     {"return 42;"});
    }
    SECTION("prefix selects an ordinary computed cache hash")
    {
        std::ofstream(work.path() / "answer.cpp") << "__llm__ int answer() { Return 42. }\n";
        result = work.mock("json/test_integration_return_values.json",
                           {"--llm", "-fllm-cache-dir=ordinary-cache", "answer.cpp"});
        REQUIRE(result.m_status == 0);
        std::string ordinary =
            llmcpp::test::read_file(fs::directory_iterator(work.path() / "ordinary-cache")->path());
        std::string key = ordinary.substr(ordinary.find("// key: ") + 8, 64);
        std::ofstream(work.path() / "answer.cpp") << "__llm__(offline, key(\"" << key.substr(0, 7)
                                                  << "\")) int answer() { A changed prompt. }\n";
        result = work.llmcpp({"--llm", "-fllm-cache-dir=ordinary-cache", "answer.cpp"});
        INFO(result.m_err);
        REQUIRE(result.m_status == 0);
        llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "answer.llm.cpp"),
                                     {"return 42;"});
    }
    SECTION("ambiguous prefix is rejected")
    {
        std::string other = metadata;
        other.replace(other.find("abcdef0123"), 10, "abcdef0456");
        std::ofstream(work.path() / "cache/abcdef0456.cpp") << other;
        result = work.llmcpp({"--llm", "-fllm-cache-dir=cache", "answer.cpp"});
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"ambiguous cache key"});
    }
    SECTION("an incompatible pinned body still fails compilation")
    {
        std::ofstream(work.path() / "answer.cpp")
            << "__llm__(offline, key(\"abcdef0\")) void answer() {}\n";
        result = work.llmcpp({"--llm", "-fllm-cache-dir=cache", "answer.cpp"});
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"does not compile"});
    }
}

// Verify short hashes, prefix collisions, and stable metadata across cache hits.
TEST_CASE("cache hashes abbreviate without losing identity", "[generation][cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "answer.cpp") << "__llm__ int answer() { Return 42. }\n";
    auto result = work.mock("json/test_integration_return_values.json",
                            {"--llm", "-fllm-cache-dir=cache", "answer.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    fs::path cache;
    for (const auto &entry : fs::directory_iterator(work.path() / "cache")) {
        cache = entry.path();
    }
    REQUIRE(cache.stem().string().size() == 7);
    std::string metadata = llmcpp::test::read_file(cache);
    size_t keyBegin = metadata.find("// key: ") + 8;
    std::string key = metadata.substr(keyBegin, 64);
    std::string original = llmcpp::test::read_file(work.path() / "answer.llm.cpp");
    CHECK(original.find("// key: " + key.substr(0, 7) + "\n") != std::string::npos);
    llmcpp::test::check_contains(original, {"// context:", "// policy:", "// prompt:", "// ---"});
    CHECK(original.find("// llmcpp: generated") == std::string::npos);
    std::vector<std::string> args{"--llm", "-fllm-offline", "-fllm-cache-dir=cache", "answer.cpp"};

    SECTION("default cache hit preserves metadata")
    {
        result = work.llmcpp(args);
        REQUIRE(result.m_status == 0);
        CHECK(llmcpp::test::read_file(work.path() / "answer.llm.cpp") == original);
    }
    SECTION("explicit abbreviation does not invalidate the cache")
    {
        args.push_back("-fllm-hash-abbrev=10");
        result = work.llmcpp(args);
        REQUIRE(result.m_status == 0);
        CHECK(llmcpp::test::read_file(work.path() / "answer.llm.cpp")
                  .find("// key: " + key.substr(0, 10) + "\n") != std::string::npos);
    }
    SECTION("legacy full-length filename remains readable")
    {
        fs::rename(cache, cache.parent_path() / (key + ".cpp"));
        result = work.llmcpp(args);
        REQUIRE(result.m_status == 0);
        CHECK(llmcpp::test::read_file(work.path() / "answer.llm.cpp") == original);
    }
    SECTION("ambiguous prefix expands source and new cache filename")
    {
        std::string other = key;
        other[7] = key[7] == 'a' ? 'b' : 'a';
        std::string conflicting = metadata;
        conflicting.replace(keyBegin, 64, other);
        std::ofstream(cache.parent_path() / (other + ".cpp")) << conflicting;
        args.erase(args.begin() + 1);
        args.push_back("-fllm-regenerate");
        result = work.mock("json/test_integration_return_values.json", args);
        INFO(result.m_err);
        REQUIRE(result.m_status == 0);
        CHECK(fs::exists(cache.parent_path() / (key.substr(0, 8) + ".cpp")));
        CHECK(llmcpp::test::read_file(cache.parent_path() / (other + ".cpp")) == conflicting);
        CHECK(llmcpp::test::read_file(work.path() / "answer.llm.cpp")
                  .find("// key: " + key.substr(0, 8) + "\n") != std::string::npos);
    }
    SECTION("new cache filenames honor explicit length")
    {
        result = work.mock(
            "json/test_integration_return_values.json",
            {"--llm", "-fllm-cache-dir=long-cache", "-fllm-hash-abbrev=10", "answer.cpp"});
        REQUIRE(result.m_status == 0);
        REQUIRE(fs::exists(work.path() / "long-cache" / (key.substr(0, 10) + ".cpp")));
    }
    SECTION("an occupied prefix is not overwritten")
    {
        std::ofstream(cache) << "unrelated cache contents\n";
        result = work.mock("json/test_integration_return_values.json",
                           {"--llm", "-fllm-regenerate", "-fllm-cache-dir=cache", "answer.cpp"});
        INFO(result.m_err);
        REQUIRE(result.m_status == 0);
        CHECK(llmcpp::test::read_file(cache) == "unrelated cache contents\n");
        CHECK(fs::exists(cache.parent_path() / (key.substr(0, 8) + ".cpp")));
    }
}

// Reject hash lengths outside the digest size or nondecimal values.
TEST_CASE("hash abbreviation validates its length", "[options]")
{
    llmcpp::test::TestWorkspace work;
    for (const char *value : {"0", "65", "-1", "abc", ""}) {
        auto result = work.llmcpp({std::string("-fllm-hash-abbrev=") + value, "-fsyntax-only",
                                   "test_integration_all_forms.cpp"});
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"invalid value for -fllm-hash-abbrev"});
    }
}

// Verify that C++ output suffixes select generated-source mode.
TEST_CASE("C++ output filenames imply source generation", "[generation]")
{
    llmcpp::test::TestWorkspace work;
    for (const std::string &output :
         {"main.llm.cpp", "anything.cpp", "anything.cc", "anything.cxx"}) {
        for (bool joined : {false, true}) {
            std::vector<std::string> args{"-fllm-no-cache", "test_integration_all_forms.cpp"};
            if (joined) {
                args.push_back("-o" + output);
            } else {
                args.push_back("-o");
                args.push_back(output);
            }
            llmcpp::test::TestCommandResult result =
                work.mock("json/test_integration_all_forms.json", args);
            INFO(result.m_err);
            REQUIRE(result.m_status == 0);
            std::string source = llmcpp::test::read_file(work.path() / output);
            CHECK(source.find("__llm__") == std::string::npos);
            CHECK(source.find("++count;") != std::string::npos);
        }
    }
    llmcpp::test::TestCommandResult multiple = work.llmcpp(
        {"test_integration_all_forms.cpp", "test_integration_tools.cpp", "-o", "both.cpp"});
    REQUIRE(multiple.m_status != 0);
    llmcpp::test::check_contains(multiple.m_err,
                                 {"cannot specify -o when generating multiple output files"});
}

// Verify semantic tools and candidate validation through the mock agent.
TEST_CASE("agent tools expose compiler context and validate bodies", "[tools]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result =
        work.mock("json/test_integration_tools.json",
                  {"-fllm-no-cache", "test_integration_tools.cpp", "-o", "tools"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);

    std::string log = llmcpp::test::read_file(work.path() / "test_integration_tools.log");
    llmcpp::test::check_contains(log, {"not accessible from here", "declared after this function",
                                       "\"size_bytes\": 4", "public: void deposit(int amount)",
                                       "'cents' is a private member of 'Account'",
                                       "\"writable\": true", "\"name\": \"hits\""});
    CHECK(llmcpp::test::count_occurrences(log, "[error] REJECTED") == 2);

    llmcpp::test::TestCommandResult run = work.run("./tools");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == "5\n1\n");
}

// Verify value returns, empty prompts, ignored comments, conversions, and an annotated main.
TEST_CASE("value-returning and empty targets infer behavior without body comments",
          "[generation][returns][comments]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult generate =
        work.mock("json/test_integration_return_values.json",
                  {"--llm", "-fllm-no-cache", "test_integration_return_values.cpp"});
    INFO(generate.m_err);
    REQUIRE(generate.m_status == 0);

    std::string log = llmcpp::test::read_file(work.path() / "test_integration_return_values.log");
    llmcpp::test::check_contains(log, {"\"return_type\": \"double\"", "\"return_type\": \"auto\"",
                                       "\"return_type\": \"deduced from the generated body\"",
                                       "\"prompt\": \"\"", "\"prompt\": \"Return x plus one.\""});
    CHECK(log.find("Return a deliberately wrong value") == std::string::npos);
    CHECK(log.find("Ignore the function name") == std::string::npos);
    CHECK(log.find("Return zero instead") == std::string::npos);
    CHECK(log.find("Make the program fail") == std::string::npos);

    fs::path generated = work.path() / "test_integration_return_values.llm.cpp";
    REQUIRE(fs::exists(generated));
    std::string source = llmcpp::test::read_file(generated);
    CHECK(source.find("__llm__") == std::string::npos);
    CHECK(source.find("Return a deliberately wrong value") == std::string::npos);
    CHECK(source.find("Ignore the function name") == std::string::npos);
    CHECK(source.find("Return zero instead") == std::string::npos);
    CHECK(source.find("Make the program fail") == std::string::npos);

    llmcpp::test::TestCommandResult build =
        work.run("g++", {"-std=c++17", generated.string(), "-o", "returns"});
    INFO(build.m_err);
    REQUIRE(build.m_status == 0);
    llmcpp::test::TestCommandResult run = work.run("./returns");
    REQUIRE(run.m_status == 0);
}

// Print only bodies whose targets request dump diagnostics.
TEST_CASE("dump code modifier is scoped to one function", "[generation][diagnostics]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "diagnostics.cpp")
        << "__llm__(dump_code) int answer() { Return 42. }\n"
        << "__llm__ int increment(int x) { Return x plus one. }\n";
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=cache", "diagnostics.cpp"};
    auto result = work.mock("json/test_integration_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_err, {"body of 'answer'", "return 42;"});
    CHECK(result.m_err.find("body of 'increment'") == std::string::npos);
    args.push_back("-fllm-offline");
    result = work.llmcpp(args);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_err, {"body of 'answer'", "return 42;"});
    CHECK(result.m_err.find("body of 'increment'") == std::string::npos);
}

// Scope progress and tool diagnostics to targets that request verbosity.
TEST_CASE("verbose modifier is scoped to one function", "[generation][diagnostics]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "diagnostics.cpp")
        << "__llm__(verbose) int answer() { Return 42. }\n"
        << "__llm__ int increment(int x) { Return x plus one. }\n";
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=cache", "diagnostics.cpp"};
    auto result = work.mock("json/test_integration_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_err, {"generating 'answer'", "generated 'answer'",
                                                "tool get_task", "tool submit"});
    CHECK(result.m_err.find("generating 'increment'") == std::string::npos);
    CHECK(result.m_err.find("generated 'increment'") == std::string::npos);
    args.push_back("-fllm-offline");
    result = work.llmcpp(args);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_err, {"'answer' from"});
    CHECK(result.m_err.find("'increment' from") == std::string::npos);
}

// Verify that successful generation is silent unless verbose output is requested.
TEST_CASE("successful generation is silent by default", "[generation][diagnostics]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "answer.cpp") << "__llm__ int answer() { Return 42. }\n";
    std::vector<std::string> args{"--llm", "-fllm-no-cache", "answer.cpp"};
    bool verbose = false;
    SECTION("default") {}
    SECTION("verbose")
    {
        verbose = true;
        args.push_back("-fllm-verbose");
    }
    llmcpp::test::TestCommandResult result =
        work.mock("json/test_integration_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    CHECK(result.m_out.empty());
    if (!verbose) {
        CHECK(result.m_err.empty());
    } else {
        llmcpp::test::check_contains(result.m_err, {"generating 'answer'", "generated 'answer'"});
    }
}

// Verify diagnostics for an agent-declared generation failure.
TEST_CASE("agent failures are reported", "[failures]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result =
        work.mock("json/test_integration_failure.json",
                  {"-fllm-no-cache", "test_integration_failure.cpp", "-o", "failure"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(
        result.m_err, {"LLM failed to generate a body for 'f': mock gave up",
                       "last rejected attempt", "use of undeclared identifier 'not_declared'"});
}

// Launch a function's agent instead of the driver agent or native backend.
TEST_CASE("agent modifier overrides driver defaults", "[generation][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "agent.cpp")
        << "__llm__(agent(\"" << MOCK_AGENT_PATH << "\")) int answer() { Return 42. }\n";
    auto result =
        work.llmcpp({"--llm", "-fllm-no-cache", "-fllm-backend=openai",
                     "-fllm-agent=/nonexistent/agent", "agent.cpp"},
                    {{"LLMCPP_AGENT", "/nonexistent/environment-agent"},
                     {"LLMCPP_MOCK_SCRIPT",
                      (work.path() / "json/test_integration_return_values.json").string()}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "agent.llm.cpp"),
                                 {"return 42;", MOCK_AGENT_PATH});
    for (const char *options : {"agent(\"\")", "agent(\"   \")", "agent(2)"}) {
        std::ofstream(work.path() / "agent.cpp") << "__llm__(" << options << ") int answer() {}\n";
        result = work.llmcpp({"-fllm-dump-context", "agent.cpp"});
        REQUIRE(result.m_status != 0);
    }
}

// Select a function's backend ahead of environment and command-line defaults.
TEST_CASE("backend modifier overrides driver defaults", "[generation][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "backend.cpp")
        << "__llm__(backend(\"codex\")) void generated() {}\n";
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
TEST_CASE("generation requires an explicit backend", "[generation][options]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult missing = work.llmcpp(
        {"-fllm-no-cache", "test_integration_failure.cpp"}, {{"LLMCPP_BACKEND", ""},
                                                             {"LLMCPP_AGENT", ""},
                                                             {"ANTHROPIC_API_KEY", "test-key"},
                                                             {"OPENAI_API_KEY", "test-key"},
                                                             {"LLMCPP_CODEX", MOCK_AGENT_PATH},
                                                             {"LLMCPP_CLAUDE", MOCK_AGENT_PATH}});
    REQUIRE(missing.m_status != 0);
    llmcpp::test::check_contains(missing.m_err, {"select an LLM backend", "-fllm-backend"});
    for (const std::string &backend : {"auto", "unknown", ""}) {
        llmcpp::test::TestCommandResult invalid =
            work.llmcpp({"-fllm-backend=" + backend, "test_integration_failure.cpp"});
        CHECK(invalid.m_status != 0);
    }
}

// Verify that changing backends cannot reuse a previously generated body.
TEST_CASE("cache separates selected backends", "[cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "cached.cpp") << "__llm__ int answer() { Return 42. }\n";
    llmcpp::test::TestCommandResult generated =
        work.mock("json/test_integration_return_values.json",
                  {"--llm", "-fllm-backend=codex", "-fllm-cache-dir=cache", "cached.cpp"});
    REQUIRE(generated.m_status == 0);
    llmcpp::test::TestCommandResult same = work.llmcpp(
        {"--llm", "-fllm-backend=codex", "-fllm-offline", "-fllm-cache-dir=cache", "cached.cpp"});
    REQUIRE(same.m_status == 0);
    llmcpp::test::TestCommandResult changed = work.llmcpp(
        {"--llm", "-fllm-backend=claude", "-fllm-offline", "-fllm-cache-dir=cache", "cached.cpp"});
    REQUIRE(changed.m_status != 0);
    llmcpp::test::check_contains(changed.m_err, {"no cached body"});
}

// Verify that Anthropic generation runs in-process without the Python agent.
TEST_CASE("native Anthropic client completes a compiler tool loop", "[generation][anthropic]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestFakeAnthropicServer server;
    llmcpp::test::TestCommandResult result =
        work.llmcpp({"-fllm-no-cache", "-fllm-max-output-tokens=768",
                     "test_integration_failure.cpp", "-o", "native-anthropic"},
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
TEST_CASE("native OpenAI client completes a compiler tool loop", "[generation][openai]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestFakeOpenAIServer server;
    std::vector<std::string> args{"-fllm-no-cache",
                                  "-fllm-max-output-tokens=768",
                                  "-fllm-backend=openai",
                                  "test_integration_failure.cpp",
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
TEST_CASE("custom chat agent completes a compiler tool loop", "[generation][agent]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestFakeOpenAIServer server;
    fs::path adapter = work.path() / "python/test_integration_chat_agent.py";
    std::ofstream(work.path() / "chat.json")
        << "{\"base_url\":\"" << server.base_url() << "/v1\",\"model\":\"config-model\"}";
    llmcpp::test::TestCommandResult result = work.llmcpp(
        {"-fllm-no-cache", "-fllm-agent=python3 " + llmcpp::test::shell_quote(adapter.string()),
         "-fllm-agent-config=chat.json", "-fllm-model=local-model", "test_integration_failure.cpp",
         "-o", "custom-agent"},
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
TEST_CASE("Codex CLI completes a compiler tool loop", "[generation][codex]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "codex.json")
        << "{\"effort\":\"high\",\"executable\":\"" << MOCK_AGENT_PATH << "\"}";
    std::vector<std::string> args{"-fllm-no-cache", "test_integration_failure.cpp", "-o", "codex"};
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
TEST_CASE("a missing agent is reported", "[failures]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result =
        work.llmcpp({"-fllm-agent=/nonexistent/llmcpp-agent", "-fllm-no-cache",
                     "test_integration_failure.cpp", "-o", "failure"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err, {"failed to start"});
}
