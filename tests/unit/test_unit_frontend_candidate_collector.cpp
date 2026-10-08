/*
 * C++ file for frontend candidate collector unit tests.
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

// Catch2 declarations for AST visitor checks.
#include <catch2/catch_test_macros.hpp>

// Project candidate collector and isolated compiler under test.
#include "llmcpp/compiler_sandbox.h"
#include "llmcpp/frontend_candidate_collector.h"

// Clang AST used by the visitor.
#include "clang/AST/ASTContext.h"

// Verify that source collection excludes implicit code and template clones.
TEST_CASE("candidate collector visits source-written code only",
          "[unit][frontend_candidate_collector]")
{
    llmcpp::FrontendCandidateCollector collector;
    CHECK_FALSE(collector.shouldVisitTemplateInstantiations());
    CHECK_FALSE(collector.shouldVisitImplicitCode());
    CHECK(collector.m_functions.empty());
    CHECK(collector.m_lambdas.empty());
}

// Verify that the visitor finds written functions and excludes lambda call operators.
TEST_CASE("candidate collector records functions and lambdas",
          "[unit][frontend_candidate_collector]")
{
    llmcpp::CompilerSandbox sandbox({"-x", "c++", "-std=c++20", "-fsyntax-only", "unit.cpp"},
                                    "unit.cpp", LLMCPP_PATH);
    std::string source = "int f() { auto value = [] { return 2; }; return value(); }";
    unsigned functions = 0;
    unsigned lambdas = 0;
    llmcpp::data::DataCompilationResult result = sandbox.compile(
        source, 0, source.size(), false, "", [&](clang::ASTContext &context, clang::Sema &) {
            llmcpp::FrontendCandidateCollector collector;
            collector.TraverseDecl(context.getTranslationUnitDecl());
            functions = collector.m_functions.size();
            lambdas = collector.m_lambdas.size();
        });
    REQUIRE(result.m_ok);
    CHECK(functions == 1);
    CHECK(lambdas == 1);
}
