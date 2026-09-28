/*
 * C++ file for tool-result record unit tests.
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

// Catch2 declarations for tool checks.
#include <catch2/catch_test_macros.hpp>

// Project tool-result record under test.
#include "llmcpp/data/data_tool_result.h"

// Verify that a result only signals failure when its error flag is set.
TEST_CASE("tool results default to success", "[unit][data_tool_result]")
{
    llmcpp::data::DataToolResult result;
    CHECK(result.m_text.empty());
    CHECK_FALSE(result.m_is_error);
    result.m_text = "unknown tool";
    result.m_is_error = true;
    CHECK(result.m_is_error);
}
