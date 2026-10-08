/*
 * C++ file for source-edit record unit tests.
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

// Catch2 declarations for source-edit checks.
#include <catch2/catch_test_macros.hpp>

// Project source-edit record and transformation under test.
#include "llmcpp/data/data_source_edit.h"
#include "llmcpp/source_text.h"

// Standard container for independent source edits.
#include <vector>

// Verify that reordered edits retain their resulting source offsets.
TEST_CASE("source edits track replacement offsets", "[unit][data_source_edit]")
{
    std::vector<llmcpp::data::DataSourceEdit> edits{{8, 11, "last"}, {0, 3, "first"}};
    CHECK(llmcpp::apply_edits("one two six", edits) == "first two last");
    REQUIRE(edits.size() == 2);
    CHECK(edits[0].m_begin == 0);
    CHECK(edits[0].m_new_begin == 0);
    CHECK(edits[1].m_begin == 8);
    CHECK(edits[1].m_new_begin == 10);
}
