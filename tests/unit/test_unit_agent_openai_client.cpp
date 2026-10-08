/*
 * C++ file for OpenAI client selection unit tests.
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

// Catch2 declarations for backend-selection checks.
#include <catch2/catch_test_macros.hpp>

// Project OpenAI backend selector under test.
#include "llmcpp/agent_openai_client.h"

// Verify that an agent configuration overrides native OpenAI transport.
TEST_CASE("OpenAI client defers to an agent configuration", "[unit][agent_openai_client]")
{
    llmcpp::data::DataGenerationOptions opts;
    opts.m_backend = "openai";
    opts.m_agent_config_file = "agent.json";
    CHECK_FALSE(llmcpp::use_native_openai(opts));
    opts.m_agent_config_file.clear();
    opts.m_backend = "anthropic";
    CHECK_FALSE(llmcpp::use_native_openai(opts));
}
