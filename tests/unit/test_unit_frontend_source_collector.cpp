/*
 * C++ file for frontend source collector unit tests.
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

// Catch2 declarations for source checks.
#include <catch2/catch_test_macros.hpp>

// Project source collector under test.
#include "llmcpp/compiler_sandbox.h"
#include "llmcpp/frontend_source_collector.h"

// Clang AST and token types for annotation and include callbacks.
#include "clang/AST/ASTContext.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/Token.h"

// Standard optional marker for an unresolved include.
#include <optional>

// Verify that annotation and include callbacks preserve source locations.
TEST_CASE("source collector records annotations and includes", "[unit][frontend_source_collector]")
{
    llmcpp::CompilerSandbox sandbox({"-x", "c++", "-std=c++20", "-fsyntax-only", "unit.cpp"},
                                    "unit.cpp", LLMCPP_PATH);
    std::vector<clang::SourceLocation> keywords;
    std::vector<llmcpp::data::DataIncludeDirective> includes;
    std::string source = "int value = 7;";
    llmcpp::data::DataCompilationResult result = sandbox.compile(
        source, 0, source.size(), false, "", [&](clang::ASTContext &context, clang::Sema &) {
            llmcpp::FrontendSourceCollector collector(context.getSourceManager(), keywords,
                                                      includes);
            clang::Token token;
            clang::MacroDefinition definition;
            collector.MacroExpands(token, definition, {}, nullptr);
            clang::IdentifierTable identifiers(context.getLangOpts());
            token.setIdentifierInfo(&identifiers.get("__llm__"));
            clang::SourceLocation location = context.getSourceManager().getLocForStartOfFile(
                context.getSourceManager().getMainFileID());
            token.setLocation(location);
            collector.MacroExpands(token, definition, {}, nullptr);
            collector.InclusionDirective(location, token, "vector", true, {}, std::nullopt, "", "",
                                         nullptr, false, clang::SrcMgr::C_User);
        });
    REQUIRE(result.m_ok);
    REQUIRE(keywords.size() == 1);
    CHECK(keywords.front().isValid());
    REQUIRE(includes.size() == 1);
    CHECK(includes.front().m_spelled == "<vector>");
    CHECK(includes.front().m_from_main_file);
}
