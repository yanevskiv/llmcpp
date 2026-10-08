/*
 * C++ file for generation integration tests.
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
#include <string>
#include <vector>

// Namespace alias for test filesystem operations.
namespace fs = std::filesystem;

// Verify generated source, native compilation, preprocessing, and caching.
TEST_CASE("generated sources compile and cache reproducibly", "[integration][generation]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult generate =
        work.mock("json/00_all_forms.json", {"--llm", "-fllm-cache-dir=cache", "00_all_forms.cpp"});
    INFO(generate.m_err);
    REQUIRE(generate.m_status == 0);

    fs::path generated = work.path() / "00_all_forms.llm.cpp";
    REQUIRE(fs::exists(generated));
    std::string source = llmcpp::test::read_file(generated);
    CHECK(source.find("__llm__") == std::string::npos);
    CHECK(llmcpp::test::count_occurrences(source, "// model: mock-model") == 8);
    CHECK(llmcpp::test::count_occurrences(source, "// prompt:") == 8);

    llmcpp::test::TestCommandResult build =
        work.llmcpp({"00_all_forms.llm.cpp", "-o", "from-llm-cpp"});
    INFO(build.m_err);
    REQUIRE(build.m_status == 0);
    llmcpp::test::TestCommandResult run = work.run("./from-llm-cpp");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == llmcpp::test::read_file(work.path() / "expected/00_all_forms.expected"));

    llmcpp::test::TestCommandResult gxx =
        work.run("g++", {"-std=c++17", "00_all_forms.llm.cpp", "-o", "with-gxx"});
    INFO(gxx.m_err);
    REQUIRE(gxx.m_status == 0);
    run = work.run("./with-gxx");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == llmcpp::test::read_file(work.path() / "expected/00_all_forms.expected"));

    llmcpp::test::TestCommandResult offline =
        work.llmcpp({"-fllm-offline", "-fllm-cache-dir=cache", "00_all_forms.cpp", "-o", "direct"});
    INFO(offline.m_err);
    REQUIRE(offline.m_status == 0);
    run = work.run("./direct");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == llmcpp::test::read_file(work.path() / "expected/00_all_forms.expected"));

    auto oldTime = fs::file_time_type::clock::now() - std::chrono::hours(24);
    fs::last_write_time(generated, oldTime);
    llmcpp::test::TestCommandResult regenerate =
        work.llmcpp({"--llm", "-fllm-offline", "-fllm-cache-dir=cache", "00_all_forms.cpp"});
    INFO(regenerate.m_err);
    REQUIRE(regenerate.m_status == 0);
    CHECK(fs::last_write_time(generated) == oldTime);

    llmcpp::test::TestCommandResult preprocess =
        work.llmcpp({"--llm", "-E", "-fllm-offline", "-fllm-cache-dir=cache", "00_all_forms.cpp"});
    INFO(preprocess.m_err);
    REQUIRE(preprocess.m_status == 0);
    llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "00_all_forms.llm.ii"),
                                 {"++count;"});

    llmcpp::test::TestCommandResult multiple =
        work.llmcpp({"--llm", "00_all_forms.cpp", "06_tools.cpp", "-o", "both.cpp"});
    REQUIRE(multiple.m_status != 0);
    llmcpp::test::check_contains(multiple.m_err,
                                 {"cannot specify -o when generating multiple output files"});
}

// Verify that C++ output suffixes select generated-source mode.
TEST_CASE("C++ output filenames imply source generation", "[integration][generation]")
{
    llmcpp::test::TestWorkspace work;
    for (const std::string &output :
         {"main.llm.cpp", "anything.cpp", "anything.cc", "anything.cxx"}) {
        for (bool joined : {false, true}) {
            std::vector<std::string> args{"-fllm-no-cache", "00_all_forms.cpp"};
            if (joined) {
                args.push_back("-o" + output);
            } else {
                args.push_back("-o");
                args.push_back(output);
            }
            llmcpp::test::TestCommandResult result = work.mock("json/00_all_forms.json", args);
            INFO(result.m_err);
            REQUIRE(result.m_status == 0);
            std::string source = llmcpp::test::read_file(work.path() / output);
            CHECK(source.find("__llm__") == std::string::npos);
            CHECK(source.find("++count;") != std::string::npos);
        }
    }
    llmcpp::test::TestCommandResult multiple =
        work.llmcpp({"00_all_forms.cpp", "06_tools.cpp", "-o", "both.cpp"});
    REQUIRE(multiple.m_status != 0);
    llmcpp::test::check_contains(multiple.m_err,
                                 {"cannot specify -o when generating multiple output files"});
}

// Verify semantic tools and candidate validation through the mock agent.
TEST_CASE("agent tools expose compiler context and validate bodies", "[integration][generation]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result =
        work.mock("json/06_tools.json", {"-fllm-no-cache", "06_tools.cpp", "-o", "tools"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);

    std::string log = llmcpp::test::read_file(work.path() / "06_tools.log");
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
          "[integration][generation]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult generate = work.mock(
        "json/05_return_values.json", {"--llm", "-fllm-no-cache", "05_return_values.cpp"});
    INFO(generate.m_err);
    REQUIRE(generate.m_status == 0);

    std::string log = llmcpp::test::read_file(work.path() / "05_return_values.log");
    llmcpp::test::check_contains(log, {"\"return_type\": \"double\"", "\"return_type\": \"auto\"",
                                       "\"return_type\": \"deduced from the generated body\"",
                                       "\"prompt\": \"\"", "\"prompt\": \"Return x plus one.\""});
    CHECK(log.find("Return a deliberately wrong value") == std::string::npos);
    CHECK(log.find("Ignore the function name") == std::string::npos);
    CHECK(log.find("Return zero instead") == std::string::npos);
    CHECK(log.find("Make the program fail") == std::string::npos);

    fs::path generated = work.path() / "05_return_values.llm.cpp";
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
