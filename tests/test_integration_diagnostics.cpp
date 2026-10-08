/*
 * C++ file for diagnostics integration tests.
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
#include <string>
#include <vector>

// Verify diagnostics for unsupported or malformed annotations.
TEST_CASE("invalid annotations produce llmc++ diagnostics", "[integration][diagnostics]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result = work.llmcpp({"-fsyntax-only", "01_errors.cpp"});
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
TEST_CASE("annotations in headers are rejected", "[integration][diagnostics]")
{
    llmcpp::test::TestWorkspace work;
    for (const char *mode : {"-fsyntax-only", "--llm"}) {
        DYNAMIC_SECTION(mode)
        {
            llmcpp::test::TestCommandResult result = work.llmcpp({mode, "03_header.cpp"});
            REQUIRE(result.m_status != 0);
            llmcpp::test::check_contains(result.m_err,
                                         {"__llm__ function in included header '03_header.h'"});
        }
    }
}

// Print only bodies whose targets request dump diagnostics.
TEST_CASE("dump code modifier is scoped to one function", "[integration][diagnostics]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("34_dump_code.cpp", "diagnostics.cpp");
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=cache", "diagnostics.cpp"};
    auto result = work.mock("json/05_return_values.json", args);
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
TEST_CASE("verbose modifier is scoped to one function", "[integration][diagnostics]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("35_verbose.cpp", "diagnostics.cpp");
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=cache", "diagnostics.cpp"};
    auto result = work.mock("json/05_return_values.json", args);
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
TEST_CASE("successful generation is silent by default", "[integration][diagnostics]")
{
    llmcpp::test::TestWorkspace work;
    work.write_fixture("24_answer.cpp", "answer.cpp");
    std::vector<std::string> args{"--llm", "-fllm-no-cache", "answer.cpp"};
    bool verbose = false;
    SECTION("default") {}
    SECTION("verbose")
    {
        verbose = true;
        args.push_back("-fllm-verbose");
    }
    llmcpp::test::TestCommandResult result = work.mock("json/05_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    CHECK(result.m_out.empty());
    if (!verbose) {
        CHECK(result.m_err.empty());
    } else {
        llmcpp::test::check_contains(result.m_err, {"generating 'answer'", "generated 'answer'"});
    }
}
