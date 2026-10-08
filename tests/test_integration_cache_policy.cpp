/*
 * C++ file for cache policy integration tests.
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
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

// Namespace alias for test filesystem operations.
namespace fs = std::filesystem;

// Exercise environment cache policy and custom-agent defaults through generation.
TEST_CASE("environment cache policies control generation", "[integration][cache_policy]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("22_environment_cache.cpp", "env.cpp");
    std::vector<std::pair<std::string, std::string>> environment = {
        {"LLMCPP_AGENT", MOCK_AGENT_PATH},
        {"LLMCPP_BACKEND", ""},
        {"LLMCPP_MOCK_SCRIPT", (work.path() / "json/04_options.json").string()},
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

// Verify that offline mode fails when no cached body exists.
TEST_CASE("offline mode requires cached bodies", "[integration][cache_policy]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result = work.llmcpp(
        {"-fsyntax-only", "-fllm-offline", "-fllm-cache-dir=empty", "00_all_forms.cpp"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err,
                                 {"no cached body for __llm__ function 'sum' (-fllm-offline)"});
}

// Verify that changes to visible headers and instructions invalidate reviewed bodies.
TEST_CASE("cache tracks context and system instructions", "[integration][cache_policy]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("23_cached_context.cpp", "cached.cpp");
    llmcpp::test::TestCommandResult generated =
        work.mock("json/04_options.json", {"--llm", "-fllm-cache-dir=cache", "cached.cpp"});
    INFO(generated.m_err);
    REQUIRE(generated.m_status == 0);
    llmcpp::test::TestCommandResult offline =
        work.llmcpp({"--llm", "-fllm-offline", "-fllm-cache-dir=cache", "cached.cpp"});
    INFO(offline.m_err);
    REQUIRE(offline.m_status == 0);
    SECTION("header contents")
    {
        std::ofstream(work.path() / "include/04_options_score.h")
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

// Verify default salts affect cache identity and function salts replace them.
TEST_CASE("command line cache salt supplies a default", "[integration][cache_policy]")
{
    llmcpp::test::TestWorkspace work;
    auto result =
        work.mock("json/04_options.json", {"--llm", "-fllm-cache-salt=driver", "04_options.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    for (const auto &entry : fs::directory_iterator(work.path() / ".llmcache")) {
        llmcpp::test::check_contains(llmcpp::test::read_file(entry.path()),
                                     {"// cache_salt: \"reviewed\""});
    }
    work.write_fixture("24_answer.cpp", "local.cpp");
    std::vector<std::string> args{"--llm", "-fllm-cache-salt=driver", "local.cpp"};
    REQUIRE(work.mock("json/05_return_values.json", args).m_status == 0);
    args.push_back("-fllm-offline");
    REQUIRE(work.llmcpp(args).m_status == 0);
    args[1] = "-fllm-cache-salt=changed";
    REQUIRE(work.llmcpp(args).m_status != 0);
    REQUIRE(work.mock("json/05_return_values.json",
                      {"--llm", "-fllm-cache-salt=driver", "-fllm-no-cache",
                       "-fllm-cache-dir=unused", "local.cpp"})
                .m_status == 0);
    CHECK_FALSE(fs::exists(work.path() / "unused"));
    result = work.llmcpp({"-fllm-cache-salt=", "local.cpp"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err, {"requires a nonempty salt"});
}

// Require cached bodies for individual targets without contacting an agent.
TEST_CASE("offline modifier requires a cached body", "[integration][cache_policy]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("25_offline_seed.cpp", "answer.cpp");
    auto result =
        work.mock("json/05_return_values.json", {"--llm", "-fllm-cache-dir=cache", "answer.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    work.write_fixture("26_offline_target.cpp", "answer.cpp");
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

// Preserve cache entries while explaining hits, misses, and regeneration.
TEST_CASE("read only caching permits generation without writes", "[integration][cache_policy]")
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
    work.write_fixture("36_answer_options.cpp", "answer.cpp", {{"@OPTIONS@", modifiers}});
    auto result = work.mock("json/05_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_err, {"miss:", "not written: cache is read-only"});
    CHECK_FALSE(fs::exists(work.path() / "cache"));
    std::vector<std::string> writable{"--llm", "-fllm-cache-dir=cache", "answer.cpp"};
    work.write_fixture("36_answer_options.cpp", "answer.cpp",
                       {{"@OPTIONS@", std::string(modifiers.size(), ' ')}});
    result = work.mock("json/05_return_values.json", writable);
    REQUIRE(result.m_status == 0);
    work.write_fixture("36_answer_options.cpp", "answer.cpp", {{"@OPTIONS@", modifiers}});
    result = work.llmcpp(args);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_err, {"hit:"});
    args.push_back("-fllm-regenerate");
    std::map<fs::path, std::string> before;
    for (const auto &entry : fs::directory_iterator(work.path() / "cache")) {
        before[entry.path()] = llmcpp::test::read_file(entry.path());
    }
    result = work.mock("json/05_return_values.json", args);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_err, {"bypass:", "not written:"});
    for (const auto &[path, contents] : before) {
        CHECK(llmcpp::test::read_file(path) == contents);
    }
}

// Force fresh bodies globally or per function, including in offline mode.
TEST_CASE("force regeneration overrides cached bodies and offline mode",
          "[integration][cache_policy]")
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
    work.write_fixture("36_answer_options.cpp", "answer.cpp", {{"@OPTIONS@", modifiers}});
    auto result = work.mock("json/05_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    for (const auto &entry : fs::directory_iterator(work.path() / "cache")) {
        std::string body = llmcpp::test::read_file(entry.path());
        size_t offset = body.find("return 42;");
        REQUIRE(offset != std::string::npos);
        body.replace(offset, 10, "return 41;");
        std::ofstream(entry.path()) << body;
    }
    result = work.mock("json/05_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "answer.llm.cpp"),
                                 {"return 42;"});
    for (const auto &entry : fs::directory_iterator(work.path() / "cache")) {
        llmcpp::test::check_contains(llmcpp::test::read_file(entry.path()), {"return 42;"});
    }
}

// Expire cache entries by age without changing their computed identities.
TEST_CASE("cache lifetime expires old entries", "[integration][cache_policy]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("24_answer.cpp", "answer.cpp");
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=cache", "answer.cpp"};
    auto result = work.mock("json/05_return_values.json", args);
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
        result = work.mock("json/05_return_values.json", args);
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
TEST_CASE("cache lifetime modifier overrides driver defaults", "[integration][cache_policy]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("27_cache_lifetime_seed.cpp", "answer.cpp");
    auto result =
        work.mock("json/05_return_values.json", {"--llm", "-fllm-cache-dir=cache", "answer.cpp"});
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
    work.write_fixture("41_offline_cache_lifetime.cpp", "answer.cpp", {{"@LIFETIME@", lifetime}});
    result =
        work.llmcpp({"--llm", "-fllm-cache-dir=cache", "-fllm-cache-lifetime=" + driverLifetime,
                     "-fllm-agent=/nonexistent/agent", "answer.cpp"});
    INFO(result.m_err);
    CHECK((result.m_status != 0) == expired);
    CHECK(fs::last_write_time(cache) == oldTime);
    CHECK(result.m_err.find("failed to start") == std::string::npos);
}

// Keep each function's cache entries in its selected directory.
TEST_CASE("cache directory modifier is scoped to one function", "[integration][cache_policy]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("28_cache_directory.cpp", "directories.cpp");
    fs::create_directory(work.path() / "custom-cache");
    std::ofstream(work.path() / "custom-cache/abcdef0.cpp") << "occupied prefix\n";
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=default-cache", "directories.cpp"};
    auto result = work.mock("json/05_return_values.json", args);
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
    work.write_fixture("29_invalid_cache_directory.cpp", "directories.cpp");
    result = work.llmcpp({"-fllm-dump-context", "directories.cpp"});
    REQUIRE(result.m_status != 0);
}
