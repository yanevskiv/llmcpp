/*
 * C++ file for agent tool handler unit tests.
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

// Catch2 declarations for dispatch checks.
#include <catch2/catch_test_macros.hpp>

// Project tool interface and implementation under test.
#include "llmcpp/agent_tool_handler.h"
#include "llmcpp/agent_tool_server.h"

// Clang compiler instance used as the context owner.
#include "clang/Frontend/CompilerInstance.h"

// Verify that the agent interface dispatches to the compiler tool server.
TEST_CASE("tool handler dispatches through its interface", "[unit][agent_tool_handler]")
{
    clang::CompilerInstance compiler;
    llmcpp::data::DataGenerationOptions opts;
    llmcpp::GenerationContext context(compiler, opts);
    llmcpp::data::DataGenerationTarget target;
    llmcpp::AgentToolServer server(context, target);
    llmcpp::AgentToolHandler &handler = server;
    llmcpp::data::DataToolResult result = handler.call_tool("absent", {});
    CHECK(result.m_is_error);
    CHECK(result.m_text == "unknown tool 'absent'");
}
