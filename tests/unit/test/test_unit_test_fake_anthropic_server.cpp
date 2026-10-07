/*
 * C++ file for fake Anthropic server unit tests.
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

// Catch2 declarations for server checks.
#include <catch2/catch_test_macros.hpp>

// Project deterministic Anthropic server under test.
#include "llmcpp/test/test_fake_anthropic_server.h"

// Verify that the local server starts with no recorded requests.
TEST_CASE("fake Anthropic server starts empty", "[unit][test_fake_anthropic_server]")
{
    llmcpp::test::TestFakeAnthropicServer server;
    CHECK(server.base_url().find("http://127.0.0.1:") == 0);
    CHECK(server.calls() == 0);
    CHECK(server.requests().empty());
    CHECK(server.problem().empty());
}
