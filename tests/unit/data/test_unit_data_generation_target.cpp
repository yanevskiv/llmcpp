/*
 * C++ file for generation-target record unit tests.
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

// Catch2 declarations for target checks.
#include <catch2/catch_test_macros.hpp>

// Project generation target record under test.
#include "llmcpp/data/data_generation_target.h"

// Verify that a fresh target has no AST identity or generated body.
TEST_CASE("generation targets start unbound", "[unit][data_generation_target]")
{
    llmcpp::data::DataGenerationTarget target;
    CHECK(target.m_function == nullptr);
    CHECK(target.m_lambda == nullptr);
    CHECK(target.m_body == nullptr);
    CHECK(target.m_keyword_offset == 0);
    CHECK_FALSE(target.m_generated);
    CHECK_FALSE(target.m_agent_override);
    CHECK(target.m_key.empty());
    CHECK(target.m_code.empty());
    CHECK(target.m_options.m_use_cache);
}
