/*
 * C++ file for generation-pass unit tests.
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

// Catch2 declarations for pass checks.
#include <catch2/catch_test_macros.hpp>

// Project generation pass under test.
#include "llmcpp/generation_pass.h"

// Clang compiler instance used as the pass owner.
#include "clang/Frontend/CompilerInstance.h"

// Verify that a fresh pass exposes one shared target collection.
TEST_CASE("generation pass exposes shared translation-unit state", "[unit][generation_pass]")
{
    clang::CompilerInstance compiler;
    llmcpp::data::DataGenerationOptions opts;
    llmcpp::data::DataGenerationResult result;
    llmcpp::GenerationPass pass(compiler, opts, {}, "int f() { return 0; }", result);
    CHECK(pass.keyword_locations().empty());
    CHECK(pass.state().m_targets.empty());
    CHECK(&pass.state().m_opts == &opts);
}
