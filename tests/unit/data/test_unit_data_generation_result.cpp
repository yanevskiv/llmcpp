/*
 * C++ file for generation-result record unit tests.
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

// Catch2 declarations for generation-result checks.
#include <catch2/catch_test_macros.hpp>

// Project generation result and status under test.
#include "llmcpp/data/data_generation_result.h"

// Verify that a pass starts without a rewrite or failure.
TEST_CASE("generation results start unchanged", "[unit][data_generation_result]")
{
    llmcpp::data::DataGenerationResult result;
    CHECK(result.m_status == llmcpp::data::DataGenerationStatus::Unchanged);
    CHECK(result.m_output.empty());
}
