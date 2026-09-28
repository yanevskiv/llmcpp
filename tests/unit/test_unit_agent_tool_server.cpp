/*
 * C++ file for agent tool server unit tests.
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

// Catch2 declarations for tool dispatch checks.
#include <catch2/catch_test_macros.hpp>

// Project tool server and shared state under test.
#include "llmcpp/agent_tool_server.h"

// Clang compiler instance used as the context owner.
#include "clang/Frontend/CompilerInstance.h"

// Verify that tool dispatch reports unknown tools and missing arguments.
TEST_CASE("tool server validates requests before AST access", "[unit][agent_tool_server]")
{
    clang::CompilerInstance compiler;
    llmcpp::data::DataGenerationOptions opts;
    llmcpp::GenerationContext context(compiler, opts);
    llmcpp::data::DataGenerationTarget target;
    llmcpp::AgentToolServer server(context, target);
    CHECK_FALSE(server.accepted());
    llmcpp::data::DataToolResult unknown = server.call_tool("missing", {});
    CHECK(unknown.m_is_error);
    CHECK(unknown.m_text.find("unknown tool") != std::string::npos);
    llmcpp::data::DataToolResult missing = server.call_tool("describe_type", {});
    CHECK(missing.m_is_error);
    CHECK(missing.m_text.find("missing string argument") != std::string::npos);
}
