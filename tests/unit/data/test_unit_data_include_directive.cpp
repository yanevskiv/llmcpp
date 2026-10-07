/*
 * C++ file for include-directive record unit tests.
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

// Catch2 declarations for include checks.
#include <catch2/catch_test_macros.hpp>

// Project include-directive record under test.
#include "llmcpp/data/data_include_directive.h"

// Verify that an unobserved include carries no source location.
TEST_CASE("include directives start unset", "[unit][data_include_directive]")
{
    llmcpp::data::DataIncludeDirective include;
    CHECK(include.m_hash_loc.isInvalid());
    CHECK(include.m_spelled.empty());
    CHECK(include.m_path.empty());
    CHECK_FALSE(include.m_from_main_file);
}
