/*
 * C++ file for frontend header annotation guard unit tests.
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

// Catch2 declarations for annotation checks.
#include <catch2/catch_test_macros.hpp>

// Project annotation guard under test.
#include "llmcpp/frontend_header_annotation_guard.h"

// Clang compiler and token types for a non-annotation expansion.
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/Token.h"

// Verify that unrelated macro expansions are ignored.
TEST_CASE("header annotation guard ignores unrelated macros",
          "[unit][frontend_header_annotation_guard]")
{
    clang::CompilerInstance compiler;
    llmcpp::FrontendHeaderAnnotationGuard guard(compiler);
    clang::Token token;
    clang::MacroDefinition definition;
    guard.MacroExpands(token, definition, {}, nullptr);
    CHECK_FALSE(compiler.hasDiagnostics());
}
