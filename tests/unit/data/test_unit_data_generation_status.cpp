/*
 * C++ file for generation-status unit tests.
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

// Catch2 declarations for status checks.
#include <catch2/catch_test_macros.hpp>

// Project generation status under test.
#include "llmcpp/data/data_generation_status.h"

// Verify that each pass outcome has its own status value.
TEST_CASE("generation statuses distinguish all pass outcomes", "[unit][data_generation_status]")
{
    using llmcpp::data::DataGenerationStatus;
    CHECK(DataGenerationStatus::Unchanged != DataGenerationStatus::Rewritten);
    CHECK(DataGenerationStatus::Rewritten != DataGenerationStatus::Failed);
    CHECK(DataGenerationStatus::Failed != DataGenerationStatus::DumpedContext);
}
