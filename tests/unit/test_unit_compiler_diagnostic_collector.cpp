/*
 * C++ file for compiler-diagnostic collector unit tests.
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

// Catch2 declarations for diagnostic checks.
#include <catch2/catch_test_macros.hpp>

// Project diagnostic collector under test.
#include "llmcpp/compiler_diagnostic_collector.h"

// Verify that the collector starts without retained diagnostics.
TEST_CASE("diagnostic collector starts empty", "[unit][compiler_diagnostic_collector]")
{
    llmcpp::CompilerDiagnosticCollector collector(4, 8, "candidate.cpp");
    CHECK(collector.m_errors == 0);
    CHECK(collector.m_warnings == 0);
    CHECK(collector.m_text.empty());
}
