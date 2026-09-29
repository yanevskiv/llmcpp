/*
 * C++ file for agent-prompt unit tests.
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

// Catch2 declarations for prompt checks.
#include <catch2/catch_test_macros.hpp>

// Project prompt construction under test.
#include "llmcpp/agent_prompt.h"

// Verify that agent settings describe the effective generation limits.
TEST_CASE("agent settings expose generation limits", "[unit][agent_prompt]")
{
    llmcpp::data::DataGenerationOptions opts;
    opts.m_model = "local-model";
    opts.m_use_cache = false;
    opts.m_max_attempts = 2;
    opts.m_max_tool_calls = 5;
    opts.m_timeout_seconds = 30;
    llvm::json::Object settings = llmcpp::agent_generation_settings(opts);
    CHECK(settings.getString("model") == "local-model");
    CHECK(settings.getString("cache") == "disabled");
    CHECK(settings.getInteger("max_attempts") == 2);
    CHECK(settings.getInteger("max_tool_calls") == 5);
    CHECK(settings.getInteger("timeout_seconds") == 30);
    CHECK_FALSE(settings.getInteger("max_output_tokens"));

    opts.m_max_output_tokens = 128;
    settings = llmcpp::agent_generation_settings(opts);
    CHECK(settings.getInteger("max_output_tokens") == 128);
}

// Verify that an explicit agent command takes priority over a backend name.
TEST_CASE("agent identity prefers the command override", "[unit][agent_prompt]")
{
    llmcpp::data::DataGenerationOptions opts;
    opts.m_backend = "openai";
    opts.m_agent_command = "local-agent";
    CHECK(llmcpp::agent_identity(opts) == "local-agent");
}

// Verify that a task message names the target and its source location.
TEST_CASE("task message identifies the requested function", "[unit][agent_prompt]")
{
    llvm::json::Object task{{"name", "sort_items"}, {"location", "main.cpp:12"}};
    std::string message = llmcpp::agent_task_message(task);
    CHECK(message.find("sort_items") != std::string::npos);
    CHECK(message.find("main.cpp:12") != std::string::npos);
    CHECK(message.find("get_task") != std::string::npos);
}
