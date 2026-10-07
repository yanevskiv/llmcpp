/*
 * C++ file for test text helper unit tests.
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

// Catch2 declarations for text-helper checks.
#include <catch2/catch_test_macros.hpp>

// Project test text helpers under test.
#include "llmcpp/test/test_text.h"

// Verify that shell quoting handles embedded single quotes.
TEST_CASE("test text quotes shell arguments", "[unit][test_text]")
{
    CHECK(llmcpp::test::shell_quote("two words") == "'two words'");
    CHECK(llmcpp::test::shell_quote("it's") == "'it'\\''s'");
    CHECK(llmcpp::test::count_occurrences("aaaa", "aa") == 2);
}
