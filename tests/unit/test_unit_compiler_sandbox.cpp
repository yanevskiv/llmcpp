/*
 * C++ file for compiler sandbox unit tests.
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

// Catch2 declarations for isolated compilation checks.
#include <catch2/catch_test_macros.hpp>

// Project shadow compiler under test.
#include "llmcpp/compiler_sandbox.h"

// Verify that isolated compilation accepts valid code and rejects invalid code.
TEST_CASE("compiler sandbox validates replacement source", "[unit][compiler_sandbox]")
{
    llmcpp::CompilerSandbox sandbox({"-x", "c++", "-std=c++20", "-fsyntax-only", "unit.cpp"},
                                    "unit.cpp", LLMCPP_PATH);
    std::string valid = "int answer() { return 42; }";
    llmcpp::data::DataCompilationResult accepted = sandbox.compile(valid, 0, valid.size(), false);
    CHECK(accepted.m_ok);
    CHECK(accepted.m_errors == 0);

    std::string invalid = "int answer() { return missing; }";
    llmcpp::data::DataCompilationResult rejected =
        sandbox.compile(invalid, 0, invalid.size(), false);
    CHECK_FALSE(rejected.m_ok);
    CHECK(rejected.m_errors > 0);
    CHECK(rejected.m_text.find("missing") != std::string::npos);
}
