/*
 * C++ file for compiler type probe finder unit tests.
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

// Catch2 declarations for probe checks.
#include <catch2/catch_test_macros.hpp>

// Project alias finder and isolated compiler under test.
#include "llmcpp/compiler_sandbox.h"
#include "llmcpp/compiler_type_probe_finder.h"

// Clang AST declarations inspected by the callback.
#include "clang/AST/ASTContext.h"

// Verify that the generated alias is selected among ordinary aliases.
TEST_CASE("type probe finder selects its reserved alias", "[unit][compiler_type_probe_finder]")
{
    llmcpp::CompilerSandbox sandbox({"-x", "c++", "-std=c++20", "-fsyntax-only", "unit.cpp"},
                                    "unit.cpp", LLMCPP_PATH);
    bool found = false;
    std::string source = "using Other = int; using __llmcpp_probe_type = long;";
    llmcpp::data::DataCompilationResult result = sandbox.compile(
        source, 0, source.size(), false, "", [&](clang::ASTContext &context, clang::Sema &) {
            llmcpp::CompilerTypeProbeFinder finder;
            finder.TraverseDecl(context.getTranslationUnitDecl());
            found = finder.m_found && finder.m_found->getName() == "__llmcpp_probe_type";
        });
    REQUIRE(result.m_ok);
    CHECK(found);
}
