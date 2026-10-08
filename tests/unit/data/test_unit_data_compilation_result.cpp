/*
 * C++ file for compilation-result record unit tests.
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

// Catch2 declarations for compilation checks.
#include <catch2/catch_test_macros.hpp>

// Project compilation result record under test.
#include "llmcpp/data/data_compilation_result.h"

// Verify that a result cannot imply success before compilation runs.
TEST_CASE("compilation results start unsuccessful", "[unit][data_compilation_result]")
{
    llmcpp::data::DataCompilationResult result;
    CHECK_FALSE(result.m_ok);
    CHECK(result.m_errors == 0);
    CHECK(result.m_warnings == 0);
    CHECK(result.m_text.empty());
}
