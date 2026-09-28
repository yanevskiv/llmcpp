/*
 * C++ file for driver-options unit tests.
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

// Catch2 declarations for option checks.
#include <catch2/catch_test_macros.hpp>

// Project driver options under test.
#include "llmcpp/driver_options.h"

// Verify that source-output suffixes select generation without reaching Clang.
TEST_CASE("driver options recognize C++ source output", "[unit][driver_options]")
{
    llmcpp::DriverOptions opts("/tmp/llmc++");
    const char *args[] = {"--help", "input.cpp", "-o", "result.cpp"};
    std::string error;
    REQUIRE(opts.parse(args, error));
    CHECK(error.empty());
    CHECK(opts.wants_help());
    CHECK(opts.options().m_emit_source);
    CHECK(opts.output_path() == "result.cpp");
}

// Verify that driver arguments retain explicit language mode and resource path.
TEST_CASE("driver options add default Clang mode", "[unit][driver_options]")
{
    llmcpp::DriverOptions opts("/tmp/llmc++");
    const char *args[] = {"--help", "-E", "input.cpp"};
    std::string error;
    REQUIRE(opts.parse(args, error));
    CHECK(opts.wants_preprocess());
    std::vector<const char *> forwarded = opts.driver_arguments("llmc++", "/resource");
    REQUIRE(forwarded.size() >= 5);
    CHECK(std::string(forwarded[1]) == "--driver-mode=g++");
    CHECK(std::string(forwarded[2]) == "-resource-dir");
    CHECK(std::string(forwarded[3]) == "/resource");
}
