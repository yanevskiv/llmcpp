/*
 * C++ file for generation-context unit tests.
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

// Catch2 declarations for source-shadow checks.
#include <catch2/catch_test_macros.hpp>

// Project generation context under test.
#include "llmcpp/generation_context.h"

// Clang compiler instance used as the context owner.
#include "clang/Frontend/CompilerInstance.h"

// Verify that a candidate replaces only its own body in shadow source.
TEST_CASE("generation context installs a candidate body", "[unit][generation_context]")
{
    clang::CompilerInstance compiler;
    llmcpp::data::DataGenerationOptions opts;
    llmcpp::GenerationContext context(compiler, opts);
    context.m_source = "int f() { return 0; }";
    context.m_targets.emplace_back();
    llmcpp::data::DataGenerationTarget &target = context.m_targets.back();
    target.m_l_brace = 8;
    target.m_r_brace = 20;
    unsigned begin = 0;
    unsigned end = 0;
    std::string source = context.shadow_source(target, "return 7;", begin, end);
    CHECK(source == "int f() {\nreturn 7;\n}");
    CHECK(source.substr(begin, end - begin) == "return 7;");
}
