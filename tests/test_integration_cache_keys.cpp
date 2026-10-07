/*
 * C++ file for cache keys integration tests.
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
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// Namespace alias for test filesystem operations.
namespace fs = std::filesystem;

// Pin cached implementations independently of prompts and compilation context.
TEST_CASE("explicit cache keys pin generated bodies", "[integration][cache_keys]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("30_explicit_cache_key.cpp", "answer.cpp");
    auto result = work.mock("json/05_return_values.json",
                            {"--llm", "-fllm-no-cache", "-fllm-cache-dir=cache", "answer.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    fs::path cache = work.path() / "cache/abcdef0.cpp";
    REQUIRE(fs::exists(cache));
    std::string metadata = llmcpp::test::read_file(cache);
    llmcpp::test::check_contains(metadata, {"// key: abcdef0123\n", "return 42;"});
    work.write_fixture("31_offline_cache_key.cpp", "answer.cpp");
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
        work.write_fixture("24_answer.cpp", "answer.cpp");
        result = work.mock("json/05_return_values.json",
                           {"--llm", "-fllm-cache-dir=ordinary-cache", "answer.cpp"});
        REQUIRE(result.m_status == 0);
        std::string ordinary =
            llmcpp::test::read_file(fs::directory_iterator(work.path() / "ordinary-cache")->path());
        std::string key = ordinary.substr(ordinary.find("// key: ") + 8, 64);
        work.write_fixture("42_offline_computed_key.cpp", "answer.cpp",
                           {{"@KEY@", key.substr(0, 7)}});
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
        work.write_fixture("32_incompatible_cache_key.cpp", "answer.cpp");
        result = work.llmcpp({"--llm", "-fllm-cache-dir=cache", "answer.cpp"});
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"does not compile"});
    }
}

// Verify short hashes, prefix collisions, and stable metadata across cache hits.
TEST_CASE("cache hashes abbreviate without losing identity", "[integration][cache_keys]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("24_answer.cpp", "answer.cpp");
    auto result =
        work.mock("json/05_return_values.json", {"--llm", "-fllm-cache-dir=cache", "answer.cpp"});
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
        result = work.mock("json/05_return_values.json", args);
        INFO(result.m_err);
        REQUIRE(result.m_status == 0);
        CHECK(fs::exists(cache.parent_path() / (key.substr(0, 8) + ".cpp")));
        CHECK(llmcpp::test::read_file(cache.parent_path() / (other + ".cpp")) == conflicting);
        CHECK(llmcpp::test::read_file(work.path() / "answer.llm.cpp")
                  .find("// key: " + key.substr(0, 8) + "\n") != std::string::npos);
    }
    SECTION("new cache filenames honor explicit length")
    {
        result = work.mock("json/05_return_values.json", {"--llm", "-fllm-cache-dir=long-cache",
                                                          "-fllm-hash-abbrev=10", "answer.cpp"});
        REQUIRE(result.m_status == 0);
        REQUIRE(fs::exists(work.path() / "long-cache" / (key.substr(0, 10) + ".cpp")));
    }
    SECTION("an occupied prefix is not overwritten")
    {
        std::ofstream(cache) << "unrelated cache contents\n";
        result = work.mock("json/05_return_values.json",
                           {"--llm", "-fllm-regenerate", "-fllm-cache-dir=cache", "answer.cpp"});
        INFO(result.m_err);
        REQUIRE(result.m_status == 0);
        CHECK(llmcpp::test::read_file(cache) == "unrelated cache contents\n");
        CHECK(fs::exists(cache.parent_path() / (key.substr(0, 8) + ".cpp")));
    }
}

// Verify that changing backends cannot reuse a previously generated body.
TEST_CASE("cache separates selected backends", "[integration][cache_keys]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("24_answer.cpp", "cached.cpp");
    llmcpp::test::TestCommandResult generated =
        work.mock("json/05_return_values.json",
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
