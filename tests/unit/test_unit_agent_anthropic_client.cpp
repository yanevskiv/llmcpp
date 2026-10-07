/*
 * C++ file for Anthropic client selection unit tests.
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

// Catch2 declarations for backend-selection checks.
#include <catch2/catch_test_macros.hpp>

// Project Anthropic backend selector under test.
#include "llmcpp/agent_anthropic_client.h"

// Verify that an external agent command overrides native Anthropic transport.
TEST_CASE("Anthropic client defers to an explicit agent", "[unit][agent_anthropic_client]")
{
    llmcpp::data::DataGenerationOptions opts;
    opts.m_backend = "anthropic";
    opts.m_agent_command = "local-agent";
    CHECK_FALSE(llmcpp::use_native_anthropic(opts));
    opts.m_agent_command.clear();
    opts.m_backend = "openai";
    CHECK_FALSE(llmcpp::use_native_anthropic(opts));
}
