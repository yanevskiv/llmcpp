/*
 * C++ file for driver application unit tests.
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

// Catch2 declarations for driver checks.
#include <catch2/catch_test_macros.hpp>

// Project Clang driver wrapper under test.
#include "llmcpp/driver_app.h"

// Verify that the driver handles its version command without a source file.
TEST_CASE("driver app handles its version command", "[unit][driver_app]")
{
    char executable[] = "llmc++";
    char version[] = "--version";
    char *args[] = {executable, version};
    llmcpp::DriverApp app(2, args);
    CHECK(app.run() == 0);
}
