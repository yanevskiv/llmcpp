/*
 * C++ file for generation-options record unit tests.
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

// Catch2 declarations for data defaults.
#include <catch2/catch_test_macros.hpp>

// Project generation policy record under test.
#include "llmcpp/data/data_generation_options.h"

// Verify that an untouched record represents the documented default policy.
TEST_CASE("generation options default to cached sequential generation",
          "[unit][data_generation_options]")
{
    llmcpp::data::DataGenerationOptions opts;
    CHECK(opts.m_backend.empty());
    CHECK(opts.m_use_cache);
    CHECK_FALSE(opts.m_offline);
    CHECK_FALSE(opts.m_regenerate);
    CHECK_FALSE(opts.m_cache_read_only);
    CHECK(opts.m_hash_abbrev == 7);
    CHECK(opts.m_max_attempts == 4);
    CHECK(opts.m_max_tool_calls == 60);
    CHECK(opts.m_timeout_seconds == 600);
}
