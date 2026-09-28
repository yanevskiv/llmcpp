/*
 * C++ file for driver integration tests.
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
#include <utility>
#include <vector>

// Verify that the compiler and bundled agent report their own release versions.
TEST_CASE("compiler and agent report independent versions", "[integration][driver]")
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
TEST_CASE("driver help includes generation options", "[integration][driver]")
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
TEST_CASE("environment generation defaults and overrides", "[integration][driver]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("08_environment_default.cpp", "env.cpp");
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
    work.write_fixture("09_environment_function_override.cpp", "env.cpp");
    result = work.llmcpp({"env.cpp", "-fllm-model=cli-model"}, environment);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_out, {"function-model", "512"});
    work.write_fixture("10_plain_main.cpp", "plain.cpp");
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
TEST_CASE("environment generation defaults validate values", "[integration][driver]")
{
    llmcpp::test::TestWorkspace work;
    for (const auto &[name, value] :
         {std::pair{"LLMCPP_OFFLINE", "maybe"}, std::pair{"LLMCPP_VERBOSE", "2"},
          std::pair{"LLMCPP_CONTEXT", "[1]"}, std::pair{"LLMCPP_APPEND_PROMPT", "["},
          std::pair{"LLMCPP_MAX_ATTEMPTS", "0"}, std::pair{"LLMCPP_MAX_TOOL_CALLS", "-1"},
          std::pair{"LLMCPP_MAX_OUTPUT_TOKENS", "0"}, std::pair{"LLMCPP_TIMEOUT", "no"},
          std::pair{"LLMCPP_HASH_ABBREV", "65"}, std::pair{"LLMCPP_CACHE_LIFETIME", "-1"}}) {
        auto result = work.llmcpp({"-fsyntax-only", "02_failure.cpp"}, {{name, value}});
        INFO(name);
        CHECK(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {name});
    }
    auto result = work.llmcpp({"-fllm-offline=maybe", "02_failure.cpp"});
    CHECK(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err, {"invalid boolean"});
    for (const char *name : {"LLMCPP_SYSTEM_PROMPT", "LLMCPP_APPEND_PROMPT", "LLMCPP_CONTEXT",
                             "LLMCPP_AGENT_CONFIG"}) {
        result = work.llmcpp({"-fsyntax-only", "02_failure.cpp"}, {{name, "missing.file"}});
        CHECK(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"missing.file"});
    }
}

// Verify compiler identification during preprocessing, generation, and compilation.
TEST_CASE("compiler identification macro is defined", "[integration][driver]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("11_compiler_macro_generated.cpp", "macro.cpp");
    llmcpp::test::TestCommandResult preprocess = work.llmcpp({"-E", "macro.cpp"});
    INFO(preprocess.m_err);
    REQUIRE(preprocess.m_status == 0);
    llmcpp::test::TestCommandResult compile =
        work.mock("json/04_options.json", {"-fsyntax-only", "-fllm-no-cache", "macro.cpp"});
    INFO(compile.m_err);
    REQUIRE(compile.m_status == 0);
    work.write_fixture("12_compiler_macro_plain.cpp", "plain.cpp");
    llmcpp::test::TestCommandResult plain = work.llmcpp({"-fsyntax-only", "plain.cpp"});
    INFO(plain.m_err);
    REQUIRE(plain.m_status == 0);
}

// Reject malformed policies before contacting any model.
TEST_CASE("invalid generation configuration is diagnosed", "[integration][driver]")
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
        work.write_fixture("38_empty_f_options.cpp", "invalid.cpp", {{"@OPTIONS@", options}});
        llmcpp::test::TestCommandResult result = work.llmcpp({"-fllm-dump-context", "invalid.cpp"});
        INFO(options);
        CHECK(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"error:"});
    }
    llmcpp::test::TestCommandResult missing =
        work.llmcpp({"-fllm-system-prompt=missing.md", "02_failure.cpp"});
    CHECK(missing.m_status != 0);
    llmcpp::test::check_contains(missing.m_err, {"cannot read system prompt"});
    std::ofstream(work.path() / "invalid.json") << "[]";
    llmcpp::test::TestCommandResult config =
        work.llmcpp({"-fllm-agent-config=invalid.json", "02_failure.cpp"});
    CHECK(config.m_status != 0);
    llmcpp::test::check_contains(config.m_err, {"agent configuration must be a JSON object"});
}

// Reject invalid reference files and output-token limits before contacting an agent.
TEST_CASE("reference and token options validate arguments", "[integration][driver]")
{
    llmcpp::test::TestWorkspace work;
    for (const std::string &modifier :
         {"context(\"missing.md\")", "context(\"\")", "context(3)", "max_output_tokens(0)",
          "max_output_tokens(-1)", "max_output_tokens(\"x\")", "cache_read_only(1)",
          "explain_cache(1)"}) {
        work.write_fixture("36_answer_options.cpp", "invalid.cpp", {{"@OPTIONS@", modifier}});
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
    work.write_fixture("13_invalid_context_encoding.cpp", "invalid.cpp");
    result = work.llmcpp({"--llm", "invalid.cpp"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err, {"not UTF-8"});
}

// Reject invalid lifetimes on the command line and individual targets.
TEST_CASE("cache lifetime requires nonnegative integers", "[integration][driver]")
{
    llmcpp::test::TestWorkspace work;
    for (const char *value : {"-1", "abc", "1.5", "4294967296", ""}) {
        auto result = work.llmcpp(
            {std::string("-fllm-cache-lifetime=") + value, "-fsyntax-only", "00_all_forms.cpp"});
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"invalid value for -fllm-cache-lifetime"});
    }
    for (const char *value : {"-1", "\"1\"", "1.5", "4294967296", ""}) {
        work.write_fixture("39_cache_lifetime_option.cpp", "invalid.cpp", {{"@LIFETIME@", value}});
        auto result = work.llmcpp({"-fllm-dump-context", "invalid.cpp"});
        REQUIRE(result.m_status != 0);
    }
}

// Reject hash lengths outside the digest size or nondecimal values.
TEST_CASE("hash abbreviation validates its length", "[integration][driver]")
{
    llmcpp::test::TestWorkspace work;
    for (const char *value : {"0", "65", "-1", "abc", ""}) {
        auto result = work.llmcpp(
            {std::string("-fllm-hash-abbrev=") + value, "-fsyntax-only", "00_all_forms.cpp"});
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"invalid value for -fllm-hash-abbrev"});
    }
}
