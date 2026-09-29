/*
 * C++ file for test command result unit tests.
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

// Catch2 declarations for process-result checks.
#include <catch2/catch_test_macros.hpp>

// Project child-command result record under test.
#include "llmcpp/test/test_command_result.h"

// Verify that process status and both output streams remain distinct.
TEST_CASE("command result preserves process output", "[unit][test_command_result]")
{
    llmcpp::test::TestCommandResult result{2, "stdout", "stderr"};
    CHECK(result.m_status == 2);
    CHECK(result.m_out == "stdout");
    CHECK(result.m_err == "stderr");
}
