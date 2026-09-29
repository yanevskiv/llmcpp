/*
 * C++ file for compiler inspection consumer unit tests.
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

// Catch2 declarations for inspection checks.
#include <catch2/catch_test_macros.hpp>

// Project inspection consumer and shadow compiler under test.
#include "llmcpp/compiler_inspection_consumer.h"
#include "llmcpp/compiler_sandbox.h"

// Verify that inspection does not run after a parsing error.
TEST_CASE("inspection consumer skips an invalid AST", "[unit][compiler_inspection_consumer]")
{
    llmcpp::CompilerSandbox sandbox({"-x", "c++", "-std=c++20", "-fsyntax-only", "unit.cpp"},
                                    "unit.cpp", LLMCPP_PATH);
    bool inspected = false;
    std::string source = "int value = ;";
    llmcpp::data::DataCompilationResult result = sandbox.compile(
        source, 0, source.size(), false, "", [&](clang::ASTContext &, clang::Sema &) {
            inspected = true;
        });
    CHECK_FALSE(result.m_ok);
    CHECK_FALSE(inspected);
}
