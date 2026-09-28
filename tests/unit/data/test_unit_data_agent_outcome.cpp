/*
 * C++ file for agent-outcome record unit tests.
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

// Catch2 declarations for outcome checks.
#include <catch2/catch_test_macros.hpp>

// Project agent outcome record under test.
#include "llmcpp/data/data_agent_outcome.h"

// Verify that an untouched outcome reports no completed tool calls.
TEST_CASE("agent outcomes start without a reported result", "[unit][data_agent_outcome]")
{
    llmcpp::data::DataAgentOutcome outcome;
    CHECK(outcome.m_status.empty());
    CHECK(outcome.m_message.empty());
    CHECK(outcome.m_model.empty());
    CHECK(outcome.m_tool_calls == 0);
}
