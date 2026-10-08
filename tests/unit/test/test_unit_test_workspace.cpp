/*
 * C++ file for test workspace unit tests.
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

// Catch2 declarations for workspace checks.
#include <catch2/catch_test_macros.hpp>

// Project isolated workspace under test.
#include "llmcpp/test/test_workspace.h"

// Verify that workspace commands capture independent output streams.
TEST_CASE("test workspace captures child output", "[unit][test_workspace]")
{
    llmcpp::test::TestWorkspace workspace;
    REQUIRE(std::filesystem::exists(workspace.path()));
    llmcpp::test::TestCommandResult result =
        workspace.run("/bin/sh", {"-c", "printf out; printf err >&2"});
    CHECK(result.m_status == 0);
    CHECK(result.m_out == "out");
    CHECK(result.m_err == "err");
}
