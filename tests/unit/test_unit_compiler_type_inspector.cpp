/*
 * C++ file for compiler type inspector unit tests.
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

// Catch2 declarations for type-tool checks.
#include <catch2/catch_test_macros.hpp>

// Project type inspector and shared state under test.
#include "llmcpp/compiler_type_inspector.h"

// Clang compiler instance used as the context owner.
#include "clang/Frontend/CompilerInstance.h"

// Verify that type probes reject empty and statement-like input before compiling.
TEST_CASE("type inspector validates type spelling", "[unit][compiler_type_inspector]")
{
    clang::CompilerInstance compiler;
    llmcpp::data::DataGenerationOptions opts;
    llmcpp::GenerationContext context(compiler, opts);
    llmcpp::data::DataGenerationTarget target;
    llmcpp::CompilerTypeInspector inspector(context, target);
    CHECK(inspector.describe_type(" ").m_is_error);
    CHECK(inspector.list_members("int; return 0").m_is_error);
    CHECK(inspector.describe_type("struct { int x; }").m_is_error);
}
