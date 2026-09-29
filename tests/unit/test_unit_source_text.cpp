/*
 * C++ file for source-text unit tests.
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

// Catch2 declarations for direct source-text checks.
#include <catch2/catch_test_macros.hpp>

// Project text transformations under test.
#include "llmcpp/source_text.h"

// Standard containers for source edits.
#include <vector>

// Verify that text normalization preserves meaningful internal spacing.
TEST_CASE("dedent removes shared indentation and outer blank lines", "[unit][source_text]")
{
    CHECK(llmcpp::dedent("\n    first  word  \n      second\t\n\n") == "first  word\n  second");
    CHECK(llmcpp::dedent(" \n\t\n").empty());
    CHECK(llmcpp::reindent("\n    first\n      second\n", "  ") == "  first\n    second\n");
}

// Verify that line helpers inspect only text before the requested offset.
TEST_CASE("line helpers identify indentation at source offsets", "[unit][source_text]")
{
    constexpr char source[] = "int value;\n\t  call();\n";
    CHECK(llmcpp::line_indent(source, 14) == "\t  ");
    CHECK(llmcpp::first_on_line(source, 14));
    CHECK_FALSE(llmcpp::first_on_line(source, 15));
    CHECK(llmcpp::collapse_whitespace("  alpha\t beta\n\n gamma  ") == "alpha beta gamma");
}

// Verify that hashing is stable and produces a full lowercase digest.
TEST_CASE("source hashes use SHA-256", "[unit][source_text]")
{
    CHECK(llmcpp::sha256_hex("abc") ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}
