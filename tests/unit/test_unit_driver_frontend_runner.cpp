/*
 * C++ file for driver frontend runner unit tests.
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

// Catch2 declarations for frontend checks.
#include <catch2/catch_test_macros.hpp>

// Project in-process frontend under test.
#include "llmcpp/driver_frontend_runner.h"

// Verify that generated input activates the in-process frontend path.
TEST_CASE("frontend runner detects rewritten inputs", "[unit][driver_frontend_runner]")
{
    llmcpp::DriverFrontendRunner::set_rewritten_input("unit.cpp", "int unit() { return 1; }");
    CHECK(llmcpp::DriverFrontendRunner::has_rewritten_inputs());
}
