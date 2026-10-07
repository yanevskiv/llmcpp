/*
 * C++ file for prompt-token record unit tests.
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

// Catch2 declarations for token checks.
#include <catch2/catch_test_macros.hpp>

// Project prompt-token record under test.
#include "llmcpp/data/data_prompt_token.h"

// Verify that a token records a half-open source range and directive flag.
TEST_CASE("prompt tokens retain source spans", "[unit][data_prompt_token]")
{
    llmcpp::data::DataPromptToken token{3, 10, true};
    CHECK(token.m_begin == 3);
    CHECK(token.m_end == 10);
    CHECK(token.m_directive);
}
