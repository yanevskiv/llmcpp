/*
 * C++ file for generation entry point unit tests.
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

// Catch2 declarations for pass-entry checks.
#include <catch2/catch_test_macros.hpp>

// Project pass entry point under test.
#include "llmcpp/generation_entry_point.h"

// Verify that an incomplete frontend invocation leaves source unchanged.
TEST_CASE("generation entry rejects a missing source input", "[unit][generation_entry_point]")
{
    llmcpp::data::DataGenerationOptions opts;
    opts.m_executable = "/tmp/llmc++";
    const char *args[] = {"-x", "c++", "-fsyntax-only"};
    llmcpp::data::DataGenerationResult result = llmcpp::run_llm_pass(args, opts);
    CHECK(result.m_status == llmcpp::data::DataGenerationStatus::Unchanged);
    CHECK(result.m_output.empty());
}
