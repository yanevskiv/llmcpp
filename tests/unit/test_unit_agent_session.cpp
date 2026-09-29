/*
 * C++ file for agent-session unit tests.
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

// Catch2 declarations for session checks.
#include <catch2/catch_test_macros.hpp>

// Project agent session under test.
#include "llmcpp/agent_session.h"

// Verify that tool schemas expose the methods needed by the generation loop.
TEST_CASE("agent session publishes generation tools", "[unit][agent_session]")
{
    llvm::json::Array tools = llmcpp::tool_definitions();
    REQUIRE(tools.size() == 10);
    CHECK(tools.front().getAsObject()->getString("name") == "get_task");
    CHECK(tools.back().getAsObject()->getString("name") == "submit");
}

// Verify that a new session has no model before the handshake.
TEST_CASE("agent session starts without a model", "[unit][agent_session]")
{
    llmcpp::data::DataGenerationOptions opts;
    llmcpp::AgentSession session(opts);
    CHECK(session.model().empty());
}
